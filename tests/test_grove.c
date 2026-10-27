#include "game.h"
#include "draw.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define CHECK(x) do{if(!(x)){fprintf(stderr,"FAIL grove:%d %s\n",__LINE__,#x);exit(1);}++checks;}while(0)
static unsigned checks;
static void step(pg_game *g,float seconds){for(int i=0;i<(int)(seconds*30);++i)pg_tick(g,1.0/30);}
static void lamp(pg_game *g){pg_pointer(g,0,119,222);pg_pointer(g,2,119,222);}
int main(void){
    for(unsigned species=0;species<3;++species){
        pg_game g;pg_init(&g,42);g.pet.phase=PG_ALIVE;g.pet.species=species;g.goal=PG_EXPLORE;g.hud_seconds=0;
        unsigned events=g.pet.event_count;lamp(&g);CHECK(g.sleeping&&g.emote==PG_EMOTE_YAWN);CHECK(g.pet.event_count==events+1);
        step(&g,20);CHECK(g.emote==PG_EMOTE_NONE&&fabsf(g.x-195)<1&&fabsf(g.y-282)<1);
        CHECK(g.sleeping&&!g.standby&&g.pet.brightness==65);
        uint16_t night_pixels[466*466];pg_canvas night_canvas={night_pixels,466,466};
        pg_render(&night_canvas,&g);unsigned visible=0;
        for(unsigned i=0;i<466*466;++i)visible+=night_pixels[i]!=0;
        CHECK(visible>100000); /* Night remains visible beyond the standby grace. */
        for(unsigned night=0;night<2;++night){
            pg_sleep(&g,night!=0);unsigned count=g.pet.event_count;
            pg_standby(&g,true);CHECK(g.standby&&g.sleeping==(night!=0));
            float anim=g.anim,energy=g.pet.energy;step(&g,20);
            CHECK(g.standby&&g.anim==anim&&g.pet.energy>=energy);
            lamp(&g);pg_tap(&g,233,233);pg_back(&g);pg_clap(&g);pg_sleep(&g,!night);
            CHECK(g.standby&&g.sleeping==(night!=0)&&g.pet.event_count==count);
            pg_render(&night_canvas,&g);visible=0;
            for(unsigned i=0;i<466*466;++i)visible+=night_pixels[i]!=0;
            CHECK(visible==0);
            pg_standby(&g,false);CHECK(!g.standby&&g.sleeping==(night!=0));
            CHECK(g.sound==PG_SOUND_NONE&&g.pet.event_count==count);
            pg_render(&night_canvas,&g);visible=0;
            for(unsigned i=0;i<466*466;++i)visible+=night_pixels[i]!=0;
            CHECK(visible>100000);
        }
        float x=g.x;step(&g,3);CHECK(fabsf(g.x-x)<1);lamp(&g);CHECK(!g.sleeping&&g.goal==PG_EXPLORE&&g.emote==PG_EMOTE_GREET);
        step(&g,5);CHECK(fabsf(g.x-195)>5);CHECK(g.target_x>=178&&g.target_x<=288&&g.target_y>=278&&g.target_y<=300);
        unsigned reactions=g.performance_serial;step(&g,30);CHECK(g.performance_serial>reactions&&g.sound==PG_SOUND_NONE);
        g.goal=PG_ATTENTION;g.ambient_wait=0;g.emote_time=0;step(&g,1);CHECK(g.emote==PG_EMOTE_GREET);
        /* A drag off the lamp cancels without toggling; the next tap still works. */
        pg_pointer(&g,0,119,222);pg_pointer(&g,2,0,0);CHECK(!g.sleeping&&!g.lamp_touch);lamp(&g);CHECK(g.sleeping);
        pg_sleep(&g,false);g.hud_seconds=0;pg_tap(&g,119,222);CHECK(g.sleeping);pg_tap(&g,119,222);CHECK(!g.sleeping);
        g.screen=PG_FOOD;pg_tap(&g,119,222);CHECK(!g.sleeping);
        g.screen=PG_HOME;g.goal=PG_REST;g.emote_time=0;g.ambient_wait=0;g.target_x=g.x;g.target_y=g.y;step(&g,2);CHECK(g.emote==PG_EMOTE_NONE);
        /* Rendering must not mutate state, care, or any random sequence. */
        uint16_t pixels[466*466];pg_canvas canvas={pixels,466,466};pg_game before=g;pg_render(&canvas,&g);CHECK(!memcmp(&before,&g,sizeof g));
        g.goal=PG_EXPLORE;g.ambient_wait=0;g.emote_time=0;g.blink=100;unsigned rng=g.pet.rng;step(&g,1);CHECK(g.pet.rng==rng);
    }
    pg_game egg;pg_init(&egg,8);lamp(&egg);step(&egg,4);CHECK(egg.sleeping&&egg.pet.phase==PG_EGG&&egg.emote==PG_EMOTE_NONE);lamp(&egg);CHECK(!egg.sleeping);
    printf("PASS %u living grove checks: visible night, independent PWR standby, scene-preserving wake, touch lock, lamp tap/cancel, nest, wander, mute, render purity, RNG isolation, egg privacy\n",checks);
}
