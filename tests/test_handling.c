#include "game.h"
#include "layout.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#define CHECK(x) do{if(!(x)){fprintf(stderr,"handling:%d %s\n",__LINE__,#x);exit(1);}}while(0)
static void sample(pg_game *g,uint64_t t,float x,float z){pg_imu_sample s={.at_us=t,.accel={x,0,1},.gyro={0,0,z},.accel_valid=true,.gyro_valid=true};pg_handling_sample(g,&s);}
int main(void){
    pg_game g;pg_init(&g,4);g.pet.phase=PG_ALIVE;uint32_t rng=g.pet.rng;sample(&g,10000,0,0);for(int i=2;i<20;++i)sample(&g,(uint64_t)i*10000,.01f,0);CHECK(g.handling.gestures==0);
    sample(&g,200000,.9f,0);sample(&g,280000,-.9f,0);sample(&g,360000,.9f,0);CHECK(g.emote==PG_EMOTE_FRIGHT&&g.handling.gestures==1&&g.pet.rng==rng);sample(&g,400000,-.9f,0);CHECK(g.handling.gestures==1);
    pg_init(&g,4);g.pet.phase=PG_ALIVE;for(int i=1;i<=30;++i)sample(&g,(uint64_t)i*20000,0,100);CHECK(g.emote==PG_EMOTE_WOBBLE&&g.handling.gestures==1);
    pg_init(&g,4);g.pet.phase=PG_ALIVE;sample(&g,10000,0,0);for(int i=1;i<=30;++i){float a=i*.16f;pg_imu_sample s={.at_us=10000+(uint64_t)i*40000,.accel={.3f*cosf(a),.3f*sinf(a),1},.accel_valid=true};pg_handling_sample(&g,&s);}CHECK(g.handling.last_gesture==2);
    /* Alternating tilt/raw gyro may animate handling, but cannot provide egg care. */
    pg_game still;pg_init(&g,4);still=g;
    for(int i=1;i<=600;++i){
        sample(&g,(uint64_t)i*20000,i%2?.9f:-.9f,100);
        g.tilt_x=(i/50)%2?.4f:-.4f;
        pg_tick(&g,.02f);pg_tick(&still,.02f);
    }
    CHECK(g.pet.warmth==still.pet.warmth&&g.pet.comfort==still.pet.comfort);
    CHECK(g.pet.egg_visits==0&&g.warmth_time==0&&g.pet.incubation==still.pet.incubation);
    /* Hourglass replaces the removed rocking hint; touch warmth still works. */
    g.hud_seconds=8;pg_tap(&g,pg_ui_dock[2][0],pg_ui_dock[2][1]);CHECK(g.screen==PG_CARD);
    g.screen=PG_HOME;g.hud_seconds=8;float before=g.pet.warmth;
    pg_tap(&g,pg_ui_dock[0][0],pg_ui_dock[0][1]);CHECK(g.pet.warmth>before&&g.pet.egg_visits==1);
    printf("PASS shared raw gesture routing, no carrying noise and RNG isolation\n");
}
