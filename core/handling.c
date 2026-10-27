#include "game.h"
#include "handling.h"
#include <math.h>
#include <string.h>
void pg_handling_reset(pg_game *g){uint32_t gestures=g->handling.gestures;int last_gesture=g->handling.last_gesture,last_recipient=g->handling.last_recipient;
    uint64_t cooldown=g->handling.cooldown_us,cosmetic=g->handling.cosmetic_us;
    memset(&g->handling,0,sizeof g->handling);
    g->handling.cooldown_us=cooldown;
    g->handling.cosmetic_us=cosmetic;
    g->handling.context=-1;g->handling.gestures=gestures;g->handling.last_gesture=last_gesture;g->handling.last_recipient=last_recipient;
    }
static void route_event(pg_game *g,bool shake){
    pg_handling *h=&g->handling;
    ++h->gestures;
    h->last_gesture=shake?1:2;
    h->last_recipient=0;
    if(g->standby)return;
    pg_user_activity(g,PG_ACTIVITY_GESTURE);
    if(g->screen==PG_ECHO){if(shake&&g->echo.stage==PG_ECHO_INPUT){h->last_recipient=2;g->echo.shake_cooldown=.8f;
            pg_echo_gesture(g,PG_SHAKE);
            }return;
        }
    if(g->sleeping||g->pet.phase==PG_DEAD||g->screen==PG_TILT||g->screen==PG_BALANCE)return;
    bool allowed=g->screen==PG_HOME||g->screen==PG_WASH||(g->screen==PG_TTT&&(g->ttt.finished||g->ttt.turn==1))||(g->screen==PG_RHYTHM&&g->rhythm.finished);
    if(!allowed||h->last_us<h->cosmetic_us)return;
    h->cosmetic_us=h->last_us+3000000;
    h->last_recipient=1;
    if(g->pet.phase!=PG_ALIVE){g->petting=1;
        return;
        }
    pg_react(g,shake?PG_EMOTE_FRIGHT:PG_EMOTE_WOBBLE,shake?PG_SOUND_GRUMBLE:PG_SOUND_NONE);
}
void pg_handling_event(pg_game *g,bool shake){route_event(g,shake);pg_trace_event(g,1,shake?1:2);}
void pg_handling_sample(pg_game *g,const pg_imu_sample *s){
    if(!s)return;
    pg_handling *h=&g->handling;
    int context=(int)g->screen*16+(g->screen==PG_ECHO?(int)g->echo.stage:0)+(g->standby?1024:0)+(g->sleeping?2048:0);
    if(context!=h->context){pg_handling_reset(g);
        h->context=context;
        }
    bool av=s->accel_valid,gv=s->gyro_valid;
    for(int i=0;i<3;++i){if(av&&(!isfinite(s->accel[i])||fabsf(s->accel[i])>16))av=false;
        if(gv&&(!isfinite(s->gyro[i])||fabsf(s->gyro[i])>2000))gv=false;
        }
    if((!av&&!gv)||(h->initialized&&(s->at_us<=h->last_us||s->at_us-h->last_us>200000))){pg_handling_reset(g);
        h->last_us=s->at_us;
        return;
        }
    float dt=h->initialized?(float)(s->at_us-h->last_us)/1000000:0;
    h->last_us=s->at_us;
    h->initialized=true;
    bool shake=false,swirl=false;
    if(gv){float spin=-s->gyro[2];
        int sign=spin>=0?1:-1;
        if(fabsf(spin)>=45){if(!h->spin_at||sign!=h->spin_sign){h->spin_at=s->at_us;
                h->spin_sign=sign;
                h->spin_angle=0;
                }h->spin_angle+=fabsf(spin)*dt;
            if(s->at_us-h->spin_at>=300000&&h->spin_angle>=25)swirl=true;
            }
        else{h->spin_at=0;
            h->spin_angle=0;
            }}
    if(av){
        if(!h->gravity_ready){memcpy(h->gravity,s->accel,sizeof h->gravity);
            h->gravity_ready=true;
            return;
            }
        float linear[3];
        int axis=0;
        float a=dt/(.35f+dt);
        for(int i=0;i<3;++i){h->gravity[i]+=a*(s->accel[i]-h->gravity[i]);
            linear[i]=s->accel[i]-h->gravity[i];
            if(fabsf(linear[i])>fabsf(linear[axis]))axis=i;
            }
        if(h->shake_at&&s->at_us-h->shake_at>700000){h->impulses=0;
            h->shake_at=0;
            }
        if(fabsf(linear[axis])>=.35f&&(!h->shake_at||s->at_us-h->shake_at>=60000)){
            int sign=linear[axis]>0?1:-1;
            if(!h->impulses||axis!=h->shake_axis){h->impulses=1;
                h->shake_axis=axis;
                h->shake_sign=sign;
                h->shake_at=s->at_us;
                }
            else if(sign!=h->shake_sign){++h->impulses;
                h->shake_sign=sign;
                h->shake_at=s->at_us;
                }
            shake=h->impulses>=3;
            }
        float mag=hypotf(linear[0],linear[1]);
        if(mag>=.18f&&mag<=1.5f){float angle=atan2f(linear[1],linear[0])*57.2957795f;
            if(!h->circle_ready||s->at_us-h->circle_at>1500000){h->circle_ready=true;
                h->circle_at=s->at_us;
                h->circle_angle=h->backtrack=0;
                h->circle_samples=0;
                h->last_circle=angle;
                }
            float change=remainderf(angle-h->last_circle,360);
            h->last_circle=angle;
            if(h->circle_angle*change<0)h->backtrack+=fabsf(change);
            h->circle_angle+=change;
            ++h->circle_samples;
            if(s->at_us-h->circle_at>=250000&&h->circle_samples>=4&&fabsf(h->circle_angle)>=90&&h->backtrack<=20)swirl=true;
        }else if(h->circle_ready&&s->at_us-h->circle_at>150000)h->circle_ready=false;
    }
    if((shake||swirl)&&s->at_us>=h->cooldown_us){h->cooldown_us=s->at_us+800000;
        h->impulses=0;
        h->shake_at=h->spin_at=0;
        h->circle_ready=false;
        pg_handling_event(g,shake);
        }
}
