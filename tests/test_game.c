#include "game.h"
#include "tutorial_life.h"
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
    CHECK(PG_EGG_SECONDS==420);
    a.pet.sheltered=true;
    pg_advance_care(&a,PG_EGG_SECONDS-a.pet.incubation-1,false);
    CHECK(a.pet.incubation==PG_EGG_SECONDS-1);step(&a,0.1);CHECK(a.pet.phase==PG_EGG);
    pg_advance_care(&a,1,false);CHECK(a.pet.incubation==PG_EGG_SECONDS);
    a.screen=PG_CARE;step(&a,1);CHECK(a.pet.phase==PG_EGG);
    a.screen=PG_HOME;step(&a,0.1);CHECK(a.pet.phase==PG_HATCHING);
    uint32_t original=a.pet.species;step(&a,6);CHECK(a.pet.phase==PG_ALIVE);CHECK(a.pet.species==original);
    pg_init(&a,9);pg_advance_care(&a,30*PG_DAY,true);CHECK(a.pet.phase==PG_EGG);CHECK(a.pet.health==100);
    pg_init(&a,9);a.pet.sheltered=true;pg_advance_care(&a,180,true);CHECK(a.pet.incubation==180);
    CHECK(a.pet.phase==PG_EGG);
    /* Cold/unsheltered, sleep, chunking and save/load all retain real-time progress. */
    pg_init(&a,9);b=a;pg_advance_care(&a,PG_EGG_SECONDS,true);
    for(int i=0;i<7;++i)pg_advance_care(&b,60,false);
    CHECK(a.pet.incubation==PG_EGG_SECONDS&&b.pet.incubation==PG_EGG_SECONDS);
    step(&a,0.1);CHECK(a.pet.phase==PG_EGG); /* three visits still required */
    pg_init(&a,9);pg_advance_care(&a,123.5,true);
    uint8_t save[PG_SAVE_MAX];pg_pet saved;size_t len=pg_save_encode(&a.pet,17,save,sizeof save);
    CHECK(len&&pg_save_decode(&saved,NULL,save,len));pg_restore(&b,&saved);CHECK(b.pet.incubation==123.5);
    pg_advance_care(&b,PG_EGG_SECONDS-123.5,true);CHECK(b.pet.incubation==PG_EGG_SECONDS);
    /* Existing six-hour records, including already-hatched pets, stay loadable. */
    for(int phase=PG_EGG;phase<=PG_DEAD;++phase){
        pg_init(&a,9);a.pet.phase=(pg_phase)phase;a.pet.elapsed=6*PG_HOUR;
        a.pet.incubation=6*PG_HOUR;a.pet.egg_visits=3;
        len=pg_save_encode(&a.pet,18,save,sizeof save);
        CHECK(len&&pg_save_decode(&saved,NULL,save,len));
        CHECK(saved.identity==a.pet.identity&&saved.phase==a.pet.phase&&saved.incubation==6*PG_HOUR);
        pg_tutorial_life life,decoded;
        CHECK(pg_tutorial_life_from_pet(&life,&saved,0,PG_TUTORIAL_LANDED));
        len=pg_tutorial_life_encode_v7(&life,19,save,sizeof save);
        CHECK(len&&pg_tutorial_life_decode(&decoded,NULL,save,len));
        pg_restore(&b,&decoded.pet);CHECK(b.pet.phase==a.pet.phase);
        if(phase==PG_EGG){step(&b,0.1);CHECK(b.pet.phase==PG_HATCHING);}
    }
}
static void care_tests(void){
    pg_game g,a,b;alive(&g,22);
    pg_advance_care(&g,12*PG_HOUR,false);CHECK(g.pet.neglect_active&&g.pet.neglect_us==0);
    pg_advance_care(&g,36*PG_HOUR-1,false);CHECK(g.pet.phase==PG_ALIVE);
    pg_advance_care(&g,1,false);CHECK(g.pet.phase==PG_DEAD&&g.screen==PG_MEMORIAL);
    alive(&a,25);b=a;pg_advance_care(&a,40*PG_HOUR,false);
    for(int i=0;i<2400;++i)pg_advance_care(&b,60,false);
    CHECK(a.pet.neglect_us==b.pet.neglect_us);CHECK(a.pet.care_food==b.pet.care_food);
    g=a;g.since_food=100;pg_feed(&g,0);pg_clean(&g);CHECK(g.pet.neglect_active);
    pg_advance_care(&g,4.75,true);CHECK(!g.pet.neglect_active&&g.pet.energy>=95);
    alive(&g,11);g.pet.energy=0;g.pet.clean=0;g.pet.satiety=0;pg_advance_care(&g,1,false);
    g.since_food=100;pg_feed(&g,1);pg_clean(&g);CHECK(g.pet.neglect_active);
    g.standby=true;float energy=g.pet.energy;pg_tick(&g,1);CHECK(g.pet.energy<=energy);
    g.sleeping=true;pg_tick(&g,5);CHECK(g.pet.energy==100&&!g.pet.neglect_active);
    alive(&g,33);pg_advance_care(&g,20*PG_HOUR,false);
    pg_tutorial_life life,decoded;uint8_t bytes[PG_SAVE_MAX];
    CHECK(pg_tutorial_life_from_pet(&life,&g.pet,0,PG_TUTORIAL_LANDED));
    CHECK(pg_tutorial_life_encode_v7(&life,7,bytes,sizeof bytes)>0);
    CHECK(pg_tutorial_life_decode(&decoded,NULL,bytes,728));
    CHECK(decoded.pet.neglect_active&&decoded.pet.neglect_us==g.pet.neglect_us);
    pg_restore(&b,&decoded.pet);pg_advance_care(&b,28*PG_HOUR,false);CHECK(b.pet.phase==PG_DEAD);
    alive(&g,20);double before=g.pet.elapsed;pg_advance_care(&g,NAN,false);pg_advance_care(&g,-5,false);CHECK(g.pet.elapsed==before);
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
static void echo_tests(void){
    pg_game g;alive(&g,81);pg_open_game(&g,3);CHECK(g.echo.stage==PG_ECHO_CUE&&g.echo.attempts==3);
    pg_echo_gesture(&g,g.echo.prompt);CHECK(g.echo.score==0);pg_pointer(&g,0,233,238);step(&g,.9);pg_pointer(&g,2,233,238);CHECK(g.echo.score==0);
    CHECK(g.echo.stage==PG_ECHO_INPUT);pg_echo_gesture(&g,g.echo.prompt);CHECK(g.echo.score==1&&g.echo.stage==PG_ECHO_PASS);step(&g,.5);CHECK(g.echo.stage==PG_ECHO_CUE);
    pg_back(&g);CHECK(g.screen==PG_GAMES&&g.echo.stage==PG_ECHO_OFF);pg_tap(&g,153,394);CHECK(g.games_page==0);pg_tap(&g,313,394);CHECK(g.games_page==1);pg_tap(&g,313,394);CHECK(g.games_page==1);pg_tap(&g,233,180);CHECK(g.screen==PG_ECHO);pg_back(&g);CHECK(g.games_page==1);
    pg_open_game(&g,1);pg_pointer(&g,0,97,244);step(&g,.2);CHECK(g.tilt.vx<0);pg_pointer(&g,2,97,244);CHECK(g.tilt_x==0&&g.tilt_y==0);
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
    CHECK(g.tilt.fell&&g.tilt.vx==0&&g.tilt.vy==0);
    for(int direction=0;direction<8;++direction){
        pg_open_game(&g,1);g.tilt.ready=0;g.tilt.score=7;
        g.tilt_x=cosf(direction*3.14159265f/4);g.tilt_y=sinf(direction*3.14159265f/4);
        step(&g,4);CHECK(g.tilt.finished&&g.tilt.fell&&g.tilt.seconds>20);
        unsigned played=g.pet.games_played[1];step(&g,2);CHECK(g.pet.games_played[1]==played);
        pg_tap(&g,233,360);CHECK(!g.tilt.finished&&!g.tilt.fell&&g.tilt.score==0);
    }
    pg_open_game(&g,1);g.tilt.ready=0;g.tilt.score=5;g.tilt.seconds=.01f;step(&g,.02);CHECK(g.tilt.finished&&!g.tilt.fell);

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
static void letter_swipe(pg_game *g,int direction){
    pg_pointer(g,0,233,230);pg_pointer(g,2,233,direction>0?180:280);
}
static void naming_tests(void){
    pg_game g,r;pg_init(&g,42);char proposed[16];memcpy(proposed,g.pet.name,16);
    g.pet.phase=PG_HATCHING;step(&g,6);
    CHECK(g.pet.phase==PG_ALIVE&&g.screen==PG_NAME_EDIT&&g.pet.naming_pending);
    CHECK(!strcmp(g.name_edit,proposed)&&!strcmp(g.pet.name,proposed));
    uint8_t data[PG_SAVE_MAX];pg_pet pet;size_t n=pg_save_encode(&g.pet,7,data,sizeof data);
    CHECK(n&&pg_save_decode(&pet,NULL,data,n));pg_restore(&r,&pet);
    CHECK(r.screen==PG_NAME_EDIT&&r.naming_hatch&&!strcmp(r.name_edit,proposed));
    /* Keep discards edits and accepts the original proposed name. */
    letter_swipe(&g,1);pg_tap(&g,177,359);
    CHECK(g.screen==PG_HOME&&!g.pet.naming_pending&&!strcmp(g.pet.name,proposed));
    g.screen=PG_CARD;pg_tap(&g,233,95);CHECK(g.screen==PG_NAME_EDIT&&!g.naming_hatch);
    unsigned rng=g.pet.rng,identity=g.pet.identity;uint32_t events=g.pet.event_count;
    for(int i=0;i<16;++i)pg_tap(&g,233,300);
    pg_tap(&g,289,359);CHECK(g.screen==PG_NAME_EDIT&&!g.name_edit[0]);
    for(int i=0;i<26;++i){letter_swipe(&g,1);CHECK(g.name_edit[0]=='A'+i);}
    letter_swipe(&g,1);CHECK(g.name_edit[0]==' ');
    pg_tap(&g,289,359);CHECK(g.screen==PG_NAME_EDIT&&!g.name_edit[0]&&g.name_cursor==0);
    letter_swipe(&g,-1);CHECK(g.name_edit[0]=='Z');
    pg_tap(&g,233,300);CHECK(!g.name_edit[0]);
    for(int i=0;i<16;++i){letter_swipe(&g,1);pg_tap(&g,344,237);}
    CHECK(strlen(g.name_edit)==PG_NAME_MAX&&!strcmp(g.pet.name,proposed)&&g.name_cursor==PG_NAME_MAX-1);
    pg_tap(&g,233,405);CHECK(g.screen==PG_NAME_EDIT); /* no invisible Back target */
    pg_back(&g);CHECK(g.screen==PG_CARD&&!strcmp(g.pet.name,proposed));
    pg_tap(&g,233,95);for(int i=0;i<16;++i)pg_tap(&g,233,300);
    const int letters[]={12,21,13,9}; /* L U M I */
    for(int i=0;i<4;++i){for(int j=0;j<letters[i];++j)letter_swipe(&g,1);pg_tap(&g,344,237);}
    CHECK(!strcmp(g.name_edit,"LUMI"));
    pg_tap(&g,122,237);CHECK(g.name_cursor==3);letter_swipe(&g,-1);CHECK(!strcmp(g.name_edit,"LUMH"));
    letter_swipe(&g,1);pg_tap(&g,344,237);
    pg_standby(&g,true);letter_swipe(&g,1);CHECK(!strcmp(g.name_edit,"LUMI"));pg_standby(&g,false);
    pg_tap(&g,289,359);CHECK(g.screen==PG_CARD&&!strcmp(g.pet.name,"LUMI")&&g.dirty);
    CHECK(g.pet.rng==rng&&g.pet.identity==identity&&g.pet.event_count==events);
    n=pg_save_encode(&g.pet,8,data,sizeof data);CHECK(n&&pg_save_decode(&pet,NULL,data,n));pg_restore(&r,&pet);
    CHECK(!strcmp(r.pet.name,"LUMI")&&r.screen==PG_HOME&&!r.pet.naming_pending);
    /* v4 saves migrate without prompting an already living creature. */
    data[4]=4;uint32_t crc=pg_crc32(data,n-4);for(int i=0;i<4;++i)data[n-4+i]=(uint8_t)(crc>>(8*i));
    CHECK(pg_save_decode(&pet,NULL,data,n)&&!pet.naming_pending&&!strcmp(pet.name,"LUMI"));
    /* Older long names load intact; edit is capped, cancel preserves the original. */
    strcpy(g.pet.name,"ABCDEFGHIJKLMNO");pg_name_begin(&g);
    CHECK(!strcmp(g.name_edit,"ABCDEF"));pg_back(&g);CHECK(!strcmp(g.pet.name,"ABCDEFGHIJKLMNO"));
    pg_name_begin(&g);pg_tap(&g,289,359);CHECK(!strcmp(g.pet.name,"ABCDEF"));
    n=pg_save_encode(&g.pet,9,data,sizeof data);CHECK(n&&pg_save_decode(&pet,NULL,data,n)&&strlen(pet.name)==6);
    pg_init(&g,42);g.screen=PG_CARD;pg_tap(&g,233,95);CHECK(g.screen==PG_CARD); /* egg privacy */
}
static void render_tests(void){
    uint16_t *pixels=malloc(466*466*2);CHECK(pixels);pg_canvas c={pixels,466,466};pg_game g;alive(&g,76);
    for(int s=PG_HOME;s<PG_SCREEN_COUNT;++s){g.screen=(pg_screen)s;pg_render(&c,&g);CHECK(pixels[0]==pg_rgb(0x07151c));}
    g.screen=PG_NAME_EDIT;strcpy(g.name_edit,"A");g.name_cursor=0;pg_render(&c,&g);
    unsigned letter_pixels=0;for(int y=190;y<246;++y)for(int x=213;x<253;++x)if(pixels[y*466+x]==pg_rgb(0xf8edcf))++letter_pixels;
    CHECK(letter_pixels>500); /* The selected letter must actually render at a supported font scale. */
    uint32_t hashes[3];g.screen=PG_HOME;
    for(int i=0;i<3;++i){g.pet.species=(uint32_t)i;pg_render(&c,&g);hashes[i]=pg_crc32(pixels,466*466*2);}
    CHECK(hashes[0]!=hashes[1]&&hashes[1]!=hashes[2]&&hashes[0]!=hashes[2]);free(pixels);
}
int main(void){
    egg_tests();care_tests();save_tests();game_tests();echo_tests();input_and_memory_tests();naming_tests();render_tests();
    printf("PASS %d checks; %u legal tic-tac-toe positions; egg, care, persistence, games, input, rendering\n",checks,ttt_positions);
    return 0;
}
