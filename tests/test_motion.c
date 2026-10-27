#include "motion.h"
#include "game.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define PI 3.14159265358979323846f
static unsigned checks;
#define CHECK(x) do{if(!(x)){fprintf(stderr,"FAIL motion:%d %s\n",__LINE__,#x);exit(1);}++checks;}while(0)
static void sample(pg_motion *m,float x,float y,float z,int n){
    for(int i=0;i<n;++i)CHECK(pg_motion_sample(m,x,y,z,0,0,0,.02f));
}
static void center(pg_motion *m,float x,float y,float z){
    pg_motion_recenter(m);sample(m,x,y,z,60);CHECK(m->centered&&m->calibration_left==0&&m->x==0&&m->y==0);
}
static void directions(void){
    const float angle=.35f,c=cosf(angle),s=sinf(angle);
    pg_motion m;
    /* Named physical poses, not the implementation's equations: lower the
       right edge, lower the left edge, lower bottom edge, lower top edge. */
    for(int upright=0;upright<2;++upright){
        center(&m,upright?1:0,0,upright?0:1);
        sample(&m,upright?c:0,s,upright?0:c,30);CHECK(m.x>.8f&&fabsf(m.y)<.01f);
        center(&m,upright?1:0,0,upright?0:1);
        sample(&m,upright?c:0,-s,upright?0:c,30);CHECK(m.x<-.8f&&fabsf(m.y)<.01f);
        center(&m,upright?1:0,0,upright?0:1);
        sample(&m,upright?c:s,0,upright?-s:c,30);CHECK(m.y>.8f&&fabsf(m.x)<.01f);
        center(&m,upright?1:0,0,upright?0:1);
        sample(&m,upright?c:-s,0,upright?s:c,30);CHECK(m.y<-.8f&&fabsf(m.x)<.01f);
    }
    /* A comfortable recline stays centered, irrespective of the boot pose. */
    center(&m,.7f,.1f,.7f);sample(&m,.7f,.1f,.7f,300);CHECK(m.x==0&&m.y==0);
    center(&m,0,0,1);sample(&m,.7f,.1f,.7f,30);CHECK(m.y>.8f);
    pg_motion_recenter(&m);sample(&m,.7f,.1f,.7f,60);CHECK(m.x==0&&m.y==0);
    /* Pitch near +/-pi must cross smoothly, rather than reverse at the wrap. */
    center(&m,sinf(PI-.1f),0,cosf(PI-.1f));sample(&m,sinf(-PI+.1f),0,cosf(-PI+.1f),60);
    CHECK(m.y>.3f&&m.y<.6f);
}
static void gyro_and_failures(void){
    pg_motion m;center(&m,0,0,1);
    CHECK(pg_motion_sample(&m,0,0,1,90,0,0,.04f));CHECK(m.x>0&&fabsf(m.y)<.01f);
    center(&m,0,0,1);CHECK(pg_motion_sample(&m,0,0,1,0,-90,0,.04f));CHECK(m.y>0&&fabsf(m.x)<.01f);
    center(&m,1,0,0);CHECK(pg_motion_sample(&m,1,0,0,0,0,-90,.04f));CHECK(m.x>0);
    /* Bias calibration, noise and missing gyro: gravity fallback still works. */
    pg_motion_recenter(&m);
    for(int i=0;i<60;++i)CHECK(pg_motion_sample(&m,0,0,1,2,-3,1,.02f));
    for(int i=0;i<1000;++i){float noise=(i%2?.006f:-.006f);CHECK(pg_motion_sample(&m,noise,-noise,1,2,-3,1,.02f));CHECK(m.x==0&&m.y==0);}
    CHECK(pg_motion_sample(&m,0,.35f,.94f,NAN,NAN,NAN,.04f));
    for(int i=0;i<50;++i)CHECK(pg_motion_sample(&m,0,.35f,.94f,NAN,NAN,NAN,.02f));CHECK(m.x>.8f);
    pg_motion before=m;
    CHECK(!pg_motion_sample(&m,NAN,0,1,0,0,0,.02f));CHECK(!memcmp(&before,&m,sizeof m));
    CHECK(!pg_motion_sample(&m,0,0,0,0,0,0,.02f));CHECK(!pg_motion_sample(&m,0,0,8,0,0,0,.02f));
    CHECK(!pg_motion_sample(&m,0,0,1,0,0,0,1));CHECK(!pg_motion_sample(NULL,0,0,1,0,0,0,.02f));
}
static void game_centering(void){
    pg_game g;pg_init(&g,7);g.pet.phase=PG_ALIVE;pg_open_game(&g,1);
    unsigned serial=g.motion_serial;CHECK(serial>0&&g.tilt.ready>0);g.tilt_x=1;
    for(int i=0;i<30;++i)pg_tick(&g,.02f);CHECK(g.tilt.seconds==30&&g.tilt.x==233);
    for(int i=0;i<35;++i)pg_tick(&g,.02f);CHECK(g.tilt.seconds<30&&g.tilt.x>233);
    g.tilt.score=3;float remaining=g.tilt.seconds;
    pg_tap(&g,233,65);CHECK(g.motion_serial==serial+1&&g.tilt.vx==0&&g.tilt.vy==0&&g.tilt.score==3&&g.tilt.seconds==remaining);
    pg_pointer(&g,0,97,244);pg_tick(&g,.1f);CHECK(g.tilt.ready==0&&g.tilt.vx<0);pg_pointer(&g,2,97,244);
    CHECK(g.tilt_x==0&&g.muted);
    pg_open_game(&g,4);serial=g.motion_serial;pg_balance_go(&g);CHECK(g.motion_serial==serial+1);
}
int main(void){directions();gyro_and_failures();game_centering();printf("PASS %u motion checks: C-board axes, flat/upright/reclined holding, gyro prediction/bias, noise/fallback, invalid samples, per-round/retry centering\n",checks);}
