#include "game.h"
#include "draw.h"
#include "layout.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define CHECK(x) do{if(!(x)){fprintf(stderr,"FAIL grove:%d %s\n",__LINE__,#x);exit(1);}++checks;}while(0)
static unsigned checks;
static void step(pg_game *g,float seconds){for(int i=0;i<(int)(seconds*30);++i)pg_tick(g,1.0/30);}
static void lamp(pg_game *g){pg_pointer(g,0,119,222);pg_pointer(g,2,119,222);}
static void retained_pose(void){
    static uint16_t a[466*466],b[466*466];pg_canvas ca={a,466,466},cb={b,466,466};
    for(unsigned species=0;species<3;++species)for(int phase=0;phase<3;++phase){
        pg_game g,r;pg_init(&g,42);g.pet.phase=PG_ALIVE;g.pet.species=species;g.x=300;
        pg_sleep(&g,true);step(&g,phase==0?1:phase==1?4:8);
        pg_sleep_pose pose;pg_sleep_pose_capture(&g,&pose);unsigned events=g.pet.event_count;
        pg_restore(&r,&g.pet);CHECK(pg_sleep_pose_restore(&r,&pose));
        CHECK(r.sleeping&&r.goal==PG_REST&&r.x==g.x&&r.y==g.y&&r.sleep_settle==g.sleep_settle);
        CHECK(r.pet.event_count==events&&r.sound==PG_SOUND_NONE&&r.pet.rng==g.pet.rng);
        if(phase==0){float x=r.x;pg_tick(&r,.1);CHECK(r.x<x&&r.sleep_settle==0);}
        if(phase==2){
            pg_render(&ca,&g);pg_render(&cb,&r);CHECK(!memcmp(a,b,sizeof a));
            pg_tick(&r,.1);CHECK(r.x==195&&r.y==282&&r.sleep_settle==3&&r.emote==PG_EMOTE_NONE);
        }
        pg_sleep_pose bad=pose;bad.settle+=.01f;pg_game before=r;
        CHECK(!pg_sleep_pose_restore(&r,&bad)&&!memcmp(&before,&r,sizeof r));
        bad=pose;bad.x=NAN;bad.crc=pg_crc32(&bad,offsetof(pg_sleep_pose,crc));CHECK(!pg_sleep_pose_restore(&r,&bad));
        pg_sleep(&g,false);pg_sleep_pose_capture(&g,&pose);CHECK(pg_sleep_pose_restore(&r,&pose)&&!r.sleeping&&r.sleep_settle==0);
    }
}
int main(void){
    retained_pose();
    for(unsigned species=0;species<3;++species){
        /* Bedtime travels at a bounded speed, stays awake while walking, and
           only starts the tuck timer after arriving. Wake cancels immediately. */
        pg_game walk;pg_init(&walk,42);walk.pet.phase=PG_ALIVE;walk.pet.species=species;
        walk.x=310;walk.y=294;walk.blink=100;
        unsigned bed_rng=walk.pet.rng;pg_sleep(&walk,true);
        CHECK(walk.x==310&&walk.y==294&&walk.target_x==195&&walk.sleep_settle==0);
        pg_tick(&walk,.1);CHECK(walk.x<310&&walk.x>306&&walk.sleep_settle==0&&walk.facing<0);
        float wx=walk.x,wy=walk.y;pg_standby(&walk,true);step(&walk,2);
        CHECK(walk.x==wx&&walk.y==wy&&walk.sleep_settle==0);
        pg_standby(&walk,false);step(&walk,5);
        CHECK(walk.x==195&&walk.y==282&&walk.sleep_settle>1&&walk.pet.rng==bed_rng);
        float settled=walk.sleep_settle,emote_left=walk.emote_time;
        pg_emote sleeping_emote=walk.emote;
        pg_standby(&walk,true);step(&walk,1);
        CHECK(walk.sleep_settle==settled&&walk.emote==sleeping_emote&&walk.emote_time==emote_left);
        pg_standby(&walk,false);pg_sleep(&walk,false);CHECK(walk.sleep_settle==0&&!walk.sleeping);
        walk.x=290;pg_sleep(&walk,true);pg_tick(&walk,.1);wx=walk.x;pg_sleep(&walk,false);
        CHECK(walk.x==wx&&walk.target_x==wx&&walk.sleep_settle==0);
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
            CHECK(g.standby&&g.anim==anim&&(night?g.pet.energy>=energy:g.pet.energy<=energy));
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
    /* Removed menu night actions cannot put a living pet to bed. */
    pg_game menu;pg_init(&menu,8);menu.pet.phase=PG_ALIVE;
    menu.screen=PG_CARE;pg_tap(&menu,233,318);CHECK(!menu.sleeping&&menu.screen==PG_CARE);
    menu.screen=PG_SETTINGS;menu.utilities.settings_page=0;pg_tap(&menu,233,293);
    CHECK(!menu.sleeping&&menu.screen==PG_SETTINGS);
    /* The enlarged rocker and outer pane work with no HUD; drag remains safe. */
    menu.screen=PG_HOME;menu.hud_seconds=0;
    pg_pointer(&menu,0,119,264);CHECK(menu.lamp_touch&&!menu.sleeping);
    pg_pointer(&menu,2,119,264);CHECK(menu.sleeping);
    pg_tap(&menu,149,215);CHECK(!menu.sleeping);
    pg_pointer(&menu,0,119,264);pg_pointer(&menu,2,170,264);CHECK(!menu.sleeping&&!menu.lamp_touch);
    CHECK(pg_ui_hit(pg_ui_lamp,119,264)&&pg_ui_hit(pg_ui_lamp,149,215));
    pg_game egg;pg_init(&egg,8);lamp(&egg);step(&egg,4);CHECK(egg.sleeping&&egg.pet.phase==PG_EGG&&egg.emote==PG_EMOTE_NONE);lamp(&egg);CHECK(!egg.sleeping);
    printf("PASS %u living grove checks: visible night, independent PWR standby, scene-preserving wake, touch lock, lamp tap/cancel, nest, wander, mute, render purity, RNG isolation, egg privacy\n",checks);
}
