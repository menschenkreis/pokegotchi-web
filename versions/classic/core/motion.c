#include "motion.h"
#include <math.h>
#include <string.h>
#define PI 3.14159265358979323846f
static float clamp(float x,float lo,float hi){return x<lo?lo:x>hi?hi:x;}
static void normalize(float v[3]){
    float n=sqrtf(v[0]*v[0]+v[1]*v[1]+v[2]*v[2]);
    if(n>0)for(int i=0;i<3;++i)v[i]/=n;
}
static float roll(const float g[3]){return atan2f(g[1],hypotf(g[0],g[2]));}
static float pitch(const float g[3]){return atan2f(g[0],g[2]);}
static float control(float angle){
    angle=remainderf(angle,2*PI);
    const float dead=0.035f,full=0.40f; /* ~2 degree rest zone, ~23 full scale */
    if(fabsf(angle)<=dead)return 0;
    return copysignf(clamp((fabsf(angle)-dead)/(full-dead),0,1),angle);
}
void pg_motion_recenter(pg_motion *m){
    if(!m)return;
    memset(m,0,sizeof *m);m->calibration_left=1;
}
bool pg_motion_sample(pg_motion *m,float ax,float ay,float az,
                      float gx,float gy,float gz,float dt){
    if(!m||!isfinite(dt)||dt<=0||dt>.2f||!isfinite(ax)||!isfinite(ay)||!isfinite(az))return false;
    float magnitude=sqrtf(ax*ax+ay*ay+az*az);
    if(magnitude<.5f||magnitude>1.8f)return false;
    float accel[3]={ax/magnitude,ay/magnitude,az/magnitude};
    bool gyro=isfinite(gx)&&isfinite(gy)&&isfinite(gz)&&fabsf(gx)<2000&&fabsf(gy)<2000&&fabsf(gz)<2000;
    if(!m->gravity_ready){memcpy(m->gravity,accel,sizeof accel);m->gravity_ready=true;}
    if(m->calibration_left>0){
        float duration=fminf(dt,m->calibration_left);
        for(int i=0;i<3;++i)m->anchor[i]+=accel[i]*duration;
        if(gyro){m->gyro_sum[0]+=gx*duration;m->gyro_sum[1]+=gy*duration;m->gyro_sum[2]+=gz*duration;m->calibration_time+=duration;++m->gyro_samples;}
        m->calibration_left=fmaxf(0,m->calibration_left-duration);++m->samples;
        memcpy(m->gravity,accel,sizeof accel);m->x=m->y=0;
        if(m->calibration_left==0){
            normalize(m->anchor);m->neutral_roll=roll(m->anchor);m->neutral_pitch=pitch(m->anchor);
            if(m->calibration_time>0)for(int i=0;i<3;++i)m->gyro_bias[i]=m->gyro_sum[i]/m->calibration_time;
            m->centered=true;
        }
        return true;
    }
    /* Gravity in a rotating sensor frame evolves as gravity CROSS angular rate.
       Keep all three axes: a fixed -gyroY/-gyroX shortcut fails upright. */
    float predicted[3];memcpy(predicted,m->gravity,sizeof predicted);
    if(gyro){
        float wx=(gx-m->gyro_bias[0])*PI/180,wy=(gy-m->gyro_bias[1])*PI/180,wz=(gz-m->gyro_bias[2])*PI/180;
        predicted[0]+=(m->gravity[1]*wz-m->gravity[2]*wy)*dt;
        predicted[1]+=(m->gravity[2]*wx-m->gravity[0]*wz)*dt;
        predicted[2]+=(m->gravity[0]*wy-m->gravity[1]*wx)*dt;
    }
    float retain=gyro?expf(-8*dt):expf(-12*dt);
    for(int i=0;i<3;++i)m->gravity[i]=predicted[i]*retain+accel[i]*(1-retain);
    normalize(m->gravity);
    m->x=control(roll(m->gravity)-m->neutral_roll);
    m->y=control(pitch(m->gravity)-m->neutral_pitch);
    return true;
}
