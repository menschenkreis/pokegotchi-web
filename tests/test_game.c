#include "game.h"
#include "brain.h"
#include "draw.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int checks;
#define CHECK(x) do{++checks;if(!(x)){fprintf(stderr,"FAIL %s:%d: %s\n",__FILE__,__LINE__,#x);exit(1);}}while(0)
static void alive(pg_game *g,uint32_t seed){pg_init(g,seed);g->pet.phase=PG_ALIVE;g->pet.elapsed=0;g->pet.age=0;g->pet.incubation=PG_EGG_SECONDS;}
static void step(pg_game *g,double seconds){while(seconds>0){double n=fmin(seconds,1.0/30);pg_tick(g,n);seconds-=n;}}

static void egg_tests(void){
    pg_game a,b;pg_init(&a,1);CHECK(a.pet.phase==PG_EGG);
    unsigned species=0;
    for(uint32_t i=1;i<100;++i){pg_init(&b,i);CHECK(b.pet.species<3);species|=1u<<b.pet.species;}
    CHECK(species==7);
    uint16_t *p=malloc(466*466*2),*q=malloc(466*466*2);CHECK(p&&q);
    pg_canvas cp={p,466,466},cq={q,466,466};b=a;b.pet.species=(a.pet.species+1)%3;strcpy(b.pet.name,"SECRET");
    pg_render(&cp,&a);pg_render(&cq,&b);CHECK(memcmp(p,q,466*466*2)==0);
    a.screen=b.screen=PG_CARD;pg_render(&cp,&a);pg_render(&cq,&b);CHECK(memcmp(p,q,466*466*2)==0);
    free(p);free(q);
    pg_init(&a,4);pg_egg_care(&a,1,2);CHECK(a.pet.egg_visits==1);
    for(int i=0;i<100;++i)pg_egg_care(&a,0,0);CHECK(a.pet.egg_visits==1);
    pg_advance_care(&a,31,false);pg_egg_care(&a,0,0);CHECK(a.pet.egg_visits==2);
    pg_advance_care(&a,31,false);pg_egg_care(&a,0,0);CHECK(a.pet.egg_visits==3);
    a.pet.sheltered=true;pg_advance_care(&a,1800,false);CHECK(a.pet.incubation==PG_EGG_SECONDS);
    a.screen=PG_CARE;step(&a,1);CHECK(a.pet.phase==PG_EGG);
    a.screen=PG_HOME;step(&a,0.1);CHECK(a.pet.phase==PG_HATCHING);
    uint32_t original=a.pet.species;step(&a,6);CHECK(a.pet.phase==PG_ALIVE);CHECK(a.pet.species==original);
    pg_init(&a,9);pg_advance_care(&a,30*PG_DAY,true);CHECK(a.pet.phase==PG_EGG);CHECK(a.pet.health==100);
    pg_init(&a,9);a.pet.sheltered=true;pg_advance_care(&a,1800,true);CHECK(a.pet.incubation>600&&a.pet.incubation<800);
    CHECK(a.pet.phase==PG_EGG);
}
static void care_tests(void){
    pg_game g,a,b;alive(&g,22);pg_advance_care(&g,24*PG_HOUR,true);
    CHECK(g.pet.phase==PG_ALIVE);CHECK(fabsf(g.pet.satiety)<0.001);CHECK(fabsf(g.pet.health-100)<0.001);
    pg_advance_care(&g,12*PG_HOUR,true);CHECK(fabsf(g.pet.health-50)<0.01);
    pg_advance_care(&g,11.9*PG_HOUR,true);CHECK(g.pet.phase==PG_ALIVE);
    pg_advance_care(&g,0.2*PG_HOUR,true);CHECK(g.pet.phase==PG_DEAD);CHECK(g.screen==PG_MEMORIAL);
    alive(&g,22);pg_advance_care(&g,40*PG_HOUR,true);CHECK(g.pet.health<34);
    pg_feed(&g,0);CHECK(g.pet.satiety==100);CHECK(g.pet.phase==PG_ALIVE);
    pg_advance_care(&g,6*PG_HOUR,true);CHECK(g.pet.health==100);
    g.pet.clean=g.pet.affection=0;pg_advance_care(&g,PG_HOUR,false);CHECK(g.pet.phase==PG_ALIVE);
    alive(&a,25);b=a;pg_advance_care(&a,40*PG_HOUR,true);
    for(int i=0;i<2400;++i)pg_advance_care(&b,60,true);
    CHECK(fabsf(a.pet.health-b.pet.health)<0.03);CHECK(fabsf(a.pet.satiety-b.pet.satiety)<0.03);
    alive(&g,20);double before=g.pet.elapsed;pg_advance_care(&g,NAN,false);pg_advance_care(&g,-5,false);CHECK(g.pet.elapsed==before);
    /* Regular feeding indefinitely supports life; growth is not a death timer. */
    alive(&g,44);
    for(int i=0;i<90;++i){g.since_food=10;pg_feed(&g,i%3);pg_advance_care(&g,12*PG_HOUR,true);}
    CHECK(g.pet.phase==PG_ALIVE);CHECK(g.stage==PG_ADULT);
    g.pet.health=0.01f;g.pet.satiety=0;pg_advance_care(&g,60,false);CHECK(g.pet.phase==PG_DEAD);
    uint32_t id=g.pet.identity;g.pet.plant_style=2;pg_new_egg(&g);CHECK(g.pet.identity==id);
    g.pet.death_acknowledged=true;pg_new_egg(&g);CHECK(g.pet.phase==PG_EGG);CHECK(g.pet.identity!=id);
    CHECK(g.pet.legacy_count==1&&g.pet.plant_style==2&&g.pet.generation==2);
    CHECK(g.pet.legacy_name[0]!=0);
}
static void save_tests(void){
    pg_game g;alive(&g,42);pg_feed(&g,1);pg_pet_gently(&g);pg_advance_care(&g,42,false);
    uint8_t data[PG_SAVE_MAX],copy[PG_SAVE_MAX];pg_pet out;uint32_t seq=0;
    size_t n=pg_save_encode(&g.pet,123,data,sizeof data);CHECK(n>100&&n<PG_SAVE_MAX);
    CHECK(pg_save_decode(&out,&seq,data,n));CHECK(seq==123);CHECK(out.identity==g.pet.identity);
    CHECK(out.species==g.pet.species);CHECK(out.meals==g.pet.meals);CHECK(out.elapsed==g.pet.elapsed);
    CHECK(out.food_preference[1]==g.pet.food_preference[1]);
    /* Version-one saves from the first emulator remain readable. */
    memcpy(copy,data,n);size_t old_len=n-56;
    uint32_t words[3]={1,(uint32_t)(old_len-20),0};
    for(int j=0;j<4;++j){copy[4+j]=(uint8_t)(words[0]>>(j*8));copy[12+j]=(uint8_t)(words[1]>>(j*8));}
    words[2]=pg_crc32(copy,old_len-4);
    for(int j=0;j<4;++j)copy[old_len-4+j]=(uint8_t)(words[2]>>(j*8));
    CHECK(pg_save_decode(&out,&seq,copy,old_len));CHECK(out.identity==g.pet.identity);
    CHECK(out.games_played[3]==0&&out.confidence[3]==40);
    memcpy(copy,data,n);old_len=n-40;words[0]=2;words[1]=(uint32_t)(old_len-20);
    for(int j=0;j<4;++j){copy[4+j]=(uint8_t)(words[0]>>(j*8));copy[12+j]=(uint8_t)(words[1]>>(j*8));}
    words[2]=pg_crc32(copy,old_len-4);
    for(int j=0;j<4;++j)copy[old_len-4+j]=(uint8_t)(words[2]>>(j*8));
    CHECK(pg_save_decode(&out,&seq,copy,old_len));CHECK(out.confidence[3]==40);
    g.pet.games_played[3]=8;g.pet.games_won[3]=7;g.pet.confidence[3]=89;
    n=pg_save_encode(&g.pet,123,data,sizeof data);
    CHECK(pg_save_decode(&out,&seq,data,n)&&out.games_played[3]==8&&out.games_won[3]==7&&out.confidence[3]==89);
    memcpy(copy,data,n);old_len=n-28;words[0]=3;words[1]=(uint32_t)(old_len-20);
    for(int j=0;j<4;++j){copy[4+j]=(uint8_t)(words[0]>>(j*8));copy[12+j]=(uint8_t)(words[1]>>(j*8));}
    words[2]=pg_crc32(copy,old_len-4);for(int j=0;j<4;++j)copy[old_len-4+j]=(uint8_t)(words[2]>>(j*8));
    CHECK(pg_save_decode(&out,&seq,copy,old_len)&&out.brightness==65&&out.games_played[3]==8&&out.confidence[5]==40);
    for(size_t i=0;i<n;++i)CHECK(!pg_save_decode(&out,&seq,data,i));
    for(size_t i=0;i<n;++i){memcpy(copy,data,n);copy[i]^=0x80;CHECK(!pg_save_decode(&out,&seq,copy,n));}
    CHECK(!pg_save_encode(&g.pet,0,data,16));
    g.pet.muted=true;n=pg_save_encode(&g.pet,124,data,sizeof data);
    CHECK(pg_save_decode(&out,&seq,data,n)&&out.muted);
    pg_pet bad=g.pet;bad.health=NAN;CHECK(!pg_save_valid_pet(&bad));
    bad=g.pet;bad.species=3;CHECK(!pg_save_valid_pet(&bad));
    bad=g.pet;bad.events[0].at=bad.elapsed+1;CHECK(!pg_save_valid_pet(&bad));
    pg_game restored;pg_restore(&restored,&g.pet);CHECK(restored.pet.elapsed==g.pet.elapsed);CHECK(restored.pet.health==g.pet.health);
    pg_restore(&restored,&g.pet);CHECK(restored.pet.elapsed==g.pet.elapsed); /* cold boot never invents time */
}
static void echo_ready(pg_game *g,pg_gesture action){
    g->echo.sequence[g->echo.passed]=(uint8_t)action;pg_echo_replay(g);
    step(g,0.9);CHECK(g->echo.stage==PG_ECHO_INPUT&&g->echo.timer>2.8f);
}
static void gesture(pg_game *g,pg_gesture action){
    if(action==PG_SHAKE){pg_echo_shake_key(g);return;}
    pg_pointer(g,0,210,238);step(g,0.12);
    if(action==PG_SWIPE)pg_pointer(g,1,295,238);
    pg_pointer(g,2,action==PG_SWIPE?295:210,238);
}
static void echo_tests(void){
    pg_game g;alive(&g,81);g.muted=true;pg_open_game(&g,3);
    CHECK(g.screen==PG_ECHO&&g.echo.stage==PG_ECHO_CUE);
    unsigned kinds=0;for(int i=0;i<5;++i)kinds|=1u<<g.echo.sequence[i];CHECK(kinds==7);
    pg_echo_gesture(&g,g.echo.prompt);CHECK(g.echo.passed==0);
    /* A pointer begun before GO cannot turn into a late pass. */
    pg_pointer(&g,0,233,238);step(&g,0.9);pg_pointer(&g,2,233,238);CHECK(g.echo.passed==0);
    g.echo.prompt=PG_TAP;pg_pointer(&g,0,233,238);step(&g,0.6);pg_pointer(&g,2,233,238);
    CHECK(g.echo.passed==0&&g.echo.stage==PG_ECHO_INPUT);
    /* Returning to origin after a drag is not a tap. */
    pg_pointer(&g,0,233,238);pg_pointer(&g,1,270,238);pg_pointer(&g,2,233,238);CHECK(g.echo.passed==0);
    pg_echo_gesture(&g,PG_SWIPE);CHECK(g.echo.stage==PG_ECHO_RETRY);CHECK(g.echo.passed==0);
    uint32_t round=g.echo.round;step(&g,1.1);CHECK(g.echo.round==round+1&&g.echo.stage==PG_ECHO_CUE);
    echo_ready(&g,PG_SWIPE);gesture(&g,PG_TAP);CHECK(g.echo.stage==PG_ECHO_RETRY);
    echo_ready(&g,PG_SWIPE);gesture(&g,PG_SWIPE);CHECK(g.echo.stage==PG_ECHO_PASS&&g.echo.passed==1);
    pg_pointer(&g,2,295,238);pg_echo_gesture(&g,PG_SWIPE);CHECK(g.echo.passed==1);
    pg_echo_next(&g);echo_ready(&g,PG_TAP);gesture(&g,PG_TAP);CHECK(g.echo.passed==2);
    pg_echo_next(&g);echo_ready(&g,PG_SHAKE);
    /* One resting tilt and a slow orientation change are not alternating impulses. */
    for(int i=0;i<50;++i){pg_echo_accel(&g,0,0,1,0.01f);step(&g,0.01);}
    for(int i=0;i<50;++i){float a=(float)i/50;pg_echo_accel(&g,sinf(a),0,cosf(a),0.01f);step(&g,0.01);}
    CHECK(g.echo.passed==2&&g.echo.stage==PG_ECHO_INPUT);
    for(int i=0;i<3;++i){step(&g,0.08);pg_echo_accel(&g,i%2?-0.9f:0.9f,0,1,0.01f);}
    CHECK(g.echo.passed==3&&g.echo.stage==PG_ECHO_PASS);
    pg_echo_accel(&g,-1,0,1,0.01f);CHECK(g.echo.passed==3);
    pg_echo_next(&g);echo_ready(&g,PG_TAP);step(&g,3.1);CHECK(g.echo.stage==PG_ECHO_RETRY&&g.echo.passed==3);
    echo_ready(&g,PG_TAP);gesture(&g,PG_TAP);pg_echo_next(&g);echo_ready(&g,PG_SHAKE);gesture(&g,PG_SHAKE);
    CHECK(g.echo.stage==PG_ECHO_DONE&&g.echo.passed==5&&g.pet.games_played[3]==1&&g.pet.games_won[3]==1);
    pg_echo_gesture(&g,PG_SHAKE);CHECK(g.pet.games_played[3]==1);CHECK(g.pet.health==100);
    /* Playback callbacks are bound to session, round AND stage. */
    g.muted=false;g.echo_audio=true;pg_open_game(&g,3);
    uint32_t session=g.echo.session;round=g.echo.round;
    pg_echo_replay(&g);pg_echo_audio_done(&g,session,round,PG_ECHO_CUE);CHECK(g.echo.stage==PG_ECHO_CUE);
    round=g.echo.round;pg_echo_audio_done(&g,session,round,PG_ECHO_CUE);CHECK(g.echo.stage==PG_ECHO_GO);
    pg_echo_audio_done(&g,session,round,PG_ECHO_CUE);CHECK(g.echo.stage==PG_ECHO_GO);
    pg_echo_audio_done(&g,session,round,PG_ECHO_GO);CHECK(g.echo.stage==PG_ECHO_INPUT&&g.echo.timer==3);
    pg_back(&g);pg_echo_audio_done(&g,session,round,PG_ECHO_GO);CHECK(g.screen==PG_GAMES&&g.echo.stage==PG_ECHO_OFF);
    pg_open_game(&g,3);pg_echo_audio_done(&g,session,round,PG_ECHO_GO);CHECK(g.echo.stage==PG_ECHO_CUE);
    g.muted=true;step(&g,0.1);CHECK(!g.echo.audio_wait);step(&g,0.8);CHECK(g.echo.stage==PG_ECHO_INPUT);
    pg_sleep(&g,true);CHECK(g.echo.stage==PG_ECHO_OFF&&g.screen==PG_HOME);
    pg_sleep(&g,false);g.muted=false;g.echo_audio=true;pg_open_game(&g,3);step(&g,2.1);
    CHECK(!g.echo_audio&&g.echo.stage==PG_ECHO_CUE);step(&g,0.9);CHECK(g.echo.stage==PG_ECHO_INPUT);
    /* The four games retain full-size rows and separate navigation. */
    pg_back(&g);CHECK(g.games_page==1);pg_tap(&g,125,405);CHECK(g.games_page==0&&g.screen==PG_GAMES);
    pg_tap(&g,339,405);CHECK(g.games_page==1);pg_tap(&g,233,208);CHECK(g.screen==PG_ECHO);
    pg_back(&g);pg_tap(&g,233,410);CHECK(g.screen==PG_HOME);
    pg_open_game(&g,1);pg_pointer(&g,0,97,244);step(&g,0.2);CHECK(g.tilt.vx<0);
    pg_pointer(&g,2,97,244);CHECK(g.tilt_x==0&&g.tilt_y==0);
}
static unsigned ttt_positions;
static void explore_ttt(int8_t b[9],int turn){
    ++ttt_positions;if(pg_ttt_winner(b))return;
    int m=pg_ttt_best(b,turn);CHECK(m>=0&&m<9&&!b[m]);
    for(int i=0;i<9;++i)if(!b[i]){b[i]=(int8_t)turn;explore_ttt(b,-turn);b[i]=0;}
}
static void game_tests(void){
    int8_t board[9]={0};explore_ttt(board,1);CHECK(ttt_positions>500000);
    /* Perfect opponents must draw; neither can make an illegal move. */
    for(int turn=1;!pg_ttt_winner(board);turn=-turn){int m=pg_ttt_best(board,turn);CHECK(m>=0&&!board[m]);board[m]=(int8_t)turn;}
    CHECK(pg_ttt_winner(board)==2);
    pg_game g;alive(&g,99);pg_open_game(&g,0);CHECK(pg_ttt_move(&g,4));CHECK(!pg_ttt_move(&g,4));
    CHECK(!pg_ttt_move(&g,0));step(&g,1);CHECK(g.ttt.turn==1||g.ttt.finished);
    uint32_t games=g.pet.games_played[0];pg_back(&g);CHECK(g.pet.games_played[0]==games);
    pg_open_game(&g,1);g.tilt_x=1;step(&g,32);CHECK(g.tilt.finished);CHECK(g.pet.games_played[1]==1);
    CHECK(hypotf(g.tilt.x-233,g.tilt.y-244)<=103.01f);
    pg_open_game(&g,2);CHECK(g.rhythm.showing);pg_rhythm_tap(&g,0);CHECK(g.rhythm.cursor==0);
    step(&g,4);CHECK(!g.rhythm.showing);step(&g,0.5);
    int note=g.rhythm.notes[0];pg_rhythm_tap(&g,(note+1)%3);CHECK(g.rhythm.finished);CHECK(g.pet.games_played[2]==1);
    CHECK(g.pet.health==100);
    g.muted=true;g.sound=PG_SOUND_NONE;pg_rhythm_start(&g);step(&g,0.8);CHECK(g.sound==PG_SOUND_NONE);
    while(!g.rhythm.finished){
        while(g.rhythm.showing||g.rhythm.timer>0)step(&g,0.05);
        int length=g.rhythm.length;
        for(int i=0;i<length;++i){pg_rhythm_tap(&g,g.rhythm.notes[i]);step(&g,0.2);}
    }
    CHECK(g.rhythm.length==10&&g.rhythm.score==9&&g.pet.games_won[2]==1);
    CHECK(g.sound==PG_SOUND_NONE);
}
static void input_and_memory_tests(void){
    pg_game g;pg_init(&g,15);pg_pointer(&g,0,233,252);step(&g,1);pg_pointer(&g,2,233,252);
    CHECK(g.pet.warmth>58);CHECK(g.screen==PG_HOME);CHECK(g.pet.egg_visits==1);
    pg_pointer(&g,0,233,252);step(&g,65);pg_pointer(&g,2,233,252);CHECK(g.pet.egg_visits==1);
    pg_sleep(&g,true);pg_pointer(&g,0,233,252);pg_pointer(&g,2,233,252);CHECK(!g.sleeping);CHECK(g.screen==PG_HOME);
    pg_pointer(&g,0,193,383);step(&g,0.8);pg_pointer(&g,2,193,383);CHECK(g.screen==PG_CARE);
    alive(&g,15);float trust=g.pet.trust;pg_pet_gently(&g);CHECK(g.pet.trust>trust);
    float trust2=g.pet.trust;pg_pet_gently(&g);CHECK(g.pet.trust==trust2);
    for(int i=0;i<10;++i){g.since_food=10;g.pet.satiety=50;pg_feed(&g,2);}
    CHECK(pg_favorite_food(&g)==2);
    char text[384];pg_brain_encode(&g,text,sizeof text);CHECK(strstr(text,"favorite food"));
    uint32_t rev=g.revision;CHECK(!pg_accept_goal(&g,g.pet.identity+1,rev,PG_PLAY,0.7f,100));
    CHECK(!pg_accept_goal(&g,g.pet.identity,rev-1,PG_PLAY,0.7f,100));
    g.screen=PG_HOME;CHECK(pg_accept_goal(&g,g.pet.identity,rev,PG_PLAY,0.7f,100));
    step(&g,10);float bored=g.boredom,goal_time=g.goal_time;
    CHECK(pg_accept_goal(&g,g.pet.identity,rev,PG_PLAY,0.7f,100));
    CHECK(g.boredom==bored&&g.goal_time==goal_time);
    CHECK(pg_accept_goal(&g,g.pet.identity,rev,PG_INSPECT,0.7f,100));
    step(&g,8);CHECK(pg_accept_goal(&g,g.pet.identity,rev,PG_INSPECT,0.7f,100));
    step(&g,8);CHECK(g.pet.milestones&1u);
    g.player_idle=100;CHECK(!pg_goal_feasible(&g,PG_INVITE));
    for(int i=0;i<80;++i)pg_event_add(&g,PG_EV_PETTED,0);
    uint8_t data[PG_SAVE_MAX];pg_pet out;size_t n=pg_save_encode(&g.pet,0,data,sizeof data);
    CHECK(n>0&&pg_save_decode(&out,NULL,data,n));CHECK(out.event_count==g.pet.event_count);
}
static void render_tests(void){
    uint16_t *pixels=malloc(466*466*2);CHECK(pixels);pg_canvas c={pixels,466,466};pg_game g;alive(&g,76);
    for(int s=PG_HOME;s<PG_SCREEN_COUNT;++s){g.screen=(pg_screen)s;pg_render(&c,&g);CHECK(pixels[0]==pg_rgb(0x07151c));}
    uint32_t hashes[3];g.screen=PG_HOME;
    for(int i=0;i<3;++i){g.pet.species=(uint32_t)i;pg_render(&c,&g);hashes[i]=pg_crc32(pixels,466*466*2);}
    CHECK(hashes[0]!=hashes[1]&&hashes[1]!=hashes[2]&&hashes[0]!=hashes[2]);free(pixels);
}
int main(void){
    egg_tests();care_tests();save_tests();game_tests();echo_tests();input_and_memory_tests();render_tests();
    printf("PASS %d checks; %u legal tic-tac-toe positions; egg, care, persistence, games, input, rendering\n",checks,ttt_positions);
    return 0;
}
