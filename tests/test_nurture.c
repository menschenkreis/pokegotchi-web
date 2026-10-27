#include "game.h"
#include "layout.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#define CHECK(x) do{if(!(x)){fprintf(stderr,"FAIL nurture:%d %s\n",__LINE__,#x);exit(1);}++checks;}while(0)
static unsigned checks;
static void step(pg_game *g,float seconds){while(seconds>0){float dt=fminf(seconds,.02f);pg_tick(g,dt);seconds-=dt;}}
static void pet(pg_game *g,unsigned seed){pg_init(g,seed);g->pet.phase=PG_ALIVE;}
static void quiet_and_reveal(void){
    pg_game g,r;pg_init(&g,3);CHECK(g.muted&&g.pet.muted&&g.sound==PG_SOUND_NONE);
    g.pet.muted=false;pg_restore(&r,&g.pet);CHECK(r.muted&&r.pet.muted&&r.sound==PG_SOUND_NONE);
    step(&g,9);CHECK(g.hud_seconds==0);pg_tap(&g,351,77);CHECK(g.screen==PG_HOME&&g.hud_seconds>0);
    pg_tap(&g,351,77);CHECK(g.screen==PG_SETTINGS);
    pg_back(&g);step(&g,9);CHECK(g.hud_seconds==0);
    unsigned visits=g.pet.egg_visits;float warmth=g.pet.warmth;
    pg_pointer(&g,0,233,250);step(&g,2);pg_pointer(&g,2,233,250);
    CHECK(g.screen==PG_HOME&&g.pet.egg_visits==visits&&g.pet.warmth<=warmth);
    g.pet.warmth=100;for(int i=0;i<20;++i)pg_egg_care(&g,12,4);CHECK(g.pet.warmth==72);
    g.pet.warmth=15;for(int i=0;i<20;++i)pg_egg_care(&g,12,4);CHECK(g.pet.warmth==72);
    pg_tap(&g,351,77);uint8_t old=g.pet.brightness;
    for(int i=0;i<5;++i){pg_tap(&g,233,144);CHECK(g.pet.brightness>=20&&g.pet.brightness<=100);}CHECK(g.pet.brightness==old);
    pg_tap(&g,289,359);CHECK(g.screen==PG_DEBUG);unsigned identity=g.pet.identity;
    pg_tap(&g,233,271);CHECK(g.debug_reset_armed&&g.pet.identity==identity);
    pg_back(&g);CHECK(!g.debug_reset_armed&&g.pet.identity==identity);
    g.screen=PG_DEBUG;g.brain_ready=true;pg_tap(&g,233,271);pg_tap(&g,233,271);
    CHECK(g.pet.identity!=identity&&g.pet.phase==PG_EGG&&g.brain_ready&&g.muted&&g.pet.brightness==old);
    g.screen=PG_DEBUG;identity=g.pet.identity;unsigned species=g.pet.species;
    pg_tap(&g,233,183);CHECK(g.pet.phase==PG_HATCHING&&g.screen==PG_HOME&&g.pet.identity==identity);
    step(&g,6);CHECK(g.pet.phase==PG_ALIVE&&g.pet.species==species&&g.muted&&g.sound==PG_SOUND_NONE);
}
static void balancing(void){
    for(unsigned seed=1;seed<=40;++seed){pg_game g;pet(&g,seed);pg_open_game(&g,4);
        CHECK(g.screen==PG_BALANCE&&!g.balance.started);step(&g,2);CHECK(g.balance.seconds==20);
        pg_tap(&g,233,363);CHECK(g.balance.started);float before=g.balance.seconds;step(&g,.5);CHECK(g.balance.seconds==before);
        for(int i=0;i<1500&&!g.balance.finished;++i){g.tilt_x=pg_clamp(-g.balance.angle*2-g.balance.velocity,-.7,.7);pg_tick(&g,.02);}
        CHECK(g.balance.finished&&g.balance.steady>=12&&g.pet.games_played[4]==1&&g.pet.games_won[4]==1);
        step(&g,2);CHECK(g.pet.games_played[4]==1&&g.pet.health==100);
        pg_tap(&g,233,363);CHECK(!g.balance.started&&!g.balance.finished&&g.balance.steady==0);
        pg_balance_go(&g);g.balance.ready=0;
        for(int i=0;i<1100&&!g.balance.finished;++i){g.tilt_x=1;pg_tick(&g,.02);}
        CHECK(g.balance.finished&&g.balance.rescues>0&&g.pet.health==100&&g.pet.games_played[4]==2);
        unsigned played=g.pet.games_played[4];pg_tap(&g,233,363);pg_back(&g);step(&g,5);CHECK(g.pet.games_played[4]==played);
    }
}
static void bathing(void){
    for(unsigned seed=1;seed<=40;++seed){pg_game g;pet(&g,seed);g.pet.clean=20;pg_open_game(&g,5);
        CHECK(g.screen==PG_WASH&&g.wash.total>60&&g.wash.left==g.wash.total);
        unsigned total=g.wash.total;pg_pointer(&g,0,180,210);pg_pointer(&g,1,320,210);CHECK(g.wash.left==total);
        step(&g,1);pg_pointer(&g,0,233,230);pg_pointer(&g,2,233,230);CHECK(g.wash.left==total);
        pg_pointer(&g,0,140,180);pg_pointer(&g,1,330,180);CHECK(g.wash.left<total&&g.pet.clean<21);
        unsigned left=g.wash.left;pg_pointer(&g,1,0,0);pg_pointer(&g,1,233,310);CHECK(!g.wash.stroke&&g.wash.left==left);
        for(int y=130;y<=380;y+=45){pg_pointer(&g,0,110,(float)y);pg_pointer(&g,1,356,(float)y);pg_pointer(&g,2,356,(float)y);}
        CHECK(g.wash.left==0&&g.wash.finished&&g.pet.games_played[5]==1&&g.pet.games_won[5]==1&&g.pet.clean==100);
        pg_pointer(&g,1,233,250);step(&g,1);CHECK(g.pet.games_played[5]==1&&g.sound==PG_SOUND_NONE);
        pg_tap(&g,233,364);CHECK(!g.wash.finished&&g.wash.left==total);
        unsigned played=g.pet.games_played[5];pg_sleep(&g,true);CHECK(!g.wash.stroke&&g.screen==PG_HOME);
        pg_sleep(&g,false);step(&g,1);CHECK(g.pet.games_played[5]==played);
    }
}
static void settings_save(void){
    pg_game g,r;pet(&g,7);g.pet.brightness=85;g.pet.games_played[4]=9;g.pet.games_won[4]=7;g.pet.confidence[5]=83;
    uint8_t bytes[PG_SAVE_MAX];pg_pet out;size_t n=pg_save_encode(&g.pet,17,bytes,sizeof bytes);
    CHECK(n>0&&n<2048&&pg_save_decode(&out,NULL,bytes,n));
    CHECK(out.brightness==85&&out.games_played[4]==9&&out.games_won[4]==7&&out.confidence[5]==83);
    out.muted=false;pg_restore(&r,&out);CHECK(r.muted&&r.pet.brightness==85);
    out.brightness=0;CHECK(!pg_save_valid_pet(&out));out.brightness=85;out.confidence[5]=NAN;CHECK(!pg_save_valid_pet(&out));
}
int main(void){quiet_and_reveal();balancing();bathing();settings_save();printf("PASS %u nursery checks: silent startup, first-tap reveal, gentle care, debug confirmation, balance win/rescue/retry, swipe completion/cancellation, v4 settings\n",checks);}
