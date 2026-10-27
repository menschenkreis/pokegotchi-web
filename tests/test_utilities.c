#include "game.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define CHECK(x) do{if(!(x)){fprintf(stderr,"utilities:%d %s\n",__LINE__,#x);exit(1);}}while(0)
static void settings_swipes(void){
    for(int page=0;page<3;++page)for(int direction=-1;direction<=1;direction+=2){
        pg_game g;pg_init(&g,42);g.screen=PG_SETTINGS;g.utilities.settings_page=page;
        pg_pet life=g.pet;pg_device_data device=g.device;
        /* Begin and end over live rows: navigation must not activate either. */
        pg_pointer(&g,0,233,218);pg_pointer(&g,1,233+direction*35,220);
        pg_pointer(&g,2,233+direction*95,223);
        int next=page-direction;if(next<0)next=0;if(next>2)next=2;
        CHECK(g.screen==PG_SETTINGS&&g.utilities.settings_page==next&&!g.pointer_down);
        CHECK(!memcmp(&life,&g.pet,sizeof life)&&!memcmp(&device,&g.device,sizeof device));
    }
    pg_game g;pg_init(&g,42);g.screen=PG_SETTINGS;g.utilities.settings_page=1;
    pg_pointer(&g,0,310,145);pg_pointer(&g,2,155,145);CHECK(g.utilities.settings_page==2&&g.screen==PG_SETTINGS);
    pg_pointer(&g,0,155,145);pg_pointer(&g,2,310,145);CHECK(g.utilities.settings_page==1);
    const float rejected[][4]={{233,145,233,280},{233,290,233,150},{300,145,210,240},{270,218,240,218}};
    for(unsigned i=0;i<sizeof rejected/sizeof *rejected;++i){
        pg_pointer(&g,0,rejected[i][0],rejected[i][1]);pg_pointer(&g,2,rejected[i][2],rejected[i][3]);
        CHECK(g.screen==PG_SETTINGS&&g.utilities.settings_page==1&&!g.device.auto_sleep);
    }
    pg_pointer(&g,0,233,218);pg_pointer(&g,1,310,218);pg_pointer(&g,1,233,218);pg_pointer(&g,2,233,218);
    CHECK(!g.device.auto_sleep&&g.utilities.settings_page==1);
    pg_pointer(&g,0,300,218);pg_pointer(&g,1,220,218);pg_pointer(&g,3,0,0);pg_pointer(&g,2,150,218);
    CHECK(g.utilities.settings_page==1&&!g.device.auto_sleep);
    pg_pointer(&g,0,300,218);pg_pointer(&g,2,0,0);CHECK(g.utilities.settings_page==1&&!g.pointer_down);
    /* Existing arrows and ordinary row taps remain available. */
    pg_pointer(&g,0,313,394);pg_pointer(&g,2,313,394);CHECK(g.utilities.settings_page==2);
    pg_pointer(&g,0,153,394);pg_pointer(&g,2,153,394);CHECK(g.utilities.settings_page==1);
    pg_pointer(&g,0,233,218);pg_pointer(&g,2,233,218);CHECK(g.device.auto_sleep);
    g.screen=PG_VOLUME;int page=g.utilities.settings_page;
    pg_pointer(&g,0,310,145);pg_pointer(&g,2,155,145);CHECK(g.screen==PG_VOLUME&&g.utilities.settings_page==page);
}
static void paged_menus(void){
    const pg_screen screens[]={PG_SETTINGS,PG_GAMES,PG_ALBUM,PG_HELP};
    const int counts[]={3,2,7,8};
    for(int menu=0;menu<4;++menu)for(int page=0;page<counts[menu];++page)for(int dir=-1;dir<=1;dir+=2){
        pg_game g;pg_init(&g,42);g.pet.phase=PG_ALIVE;g.pet.event_count=32;g.screen=screens[menu];
        int *value=menu==0?&g.utilities.settings_page:menu==1?&g.games_page:menu==2?&g.album_page:&g.icon_help_page;
        *value=page;pg_pet pet=g.pet;pg_device_data device=g.device;
        pg_pointer(&g,0,233,218);pg_pointer(&g,1,233+dir*35,220);pg_pointer(&g,2,233+dir*95,223);
        int next=page-dir;if(next<0)next=0;if(next>=counts[menu])next=counts[menu]-1;
        CHECK(*value==next&&g.screen==screens[menu]&&!g.pointer_down&&!g.album_expanded);
        CHECK(!memcmp(&pet,&g.pet,sizeof pet)&&!memcmp(&device,&g.device,sizeof device));
        pg_pointer(&g,0,233,218);pg_pointer(&g,1,310,218);pg_pointer(&g,1,233,218);pg_pointer(&g,2,233,218);
        CHECK(*value==next&&g.screen==screens[menu]&&!g.album_expanded);
        pg_pointer(&g,0,300,218);pg_pointer(&g,1,220,218);pg_pointer(&g,3,0,0);pg_pointer(&g,2,150,218);CHECK(*value==next);
    }
    for(int n=0;n<=32;++n){
        pg_game g;pg_init(&g,42);g.screen=PG_ALBUM;g.pet.event_count=(uint32_t)n;
        for(int i=0;i<10;++i){pg_pointer(&g,0,310,218);pg_pointer(&g,2,155,218);}
        int last=n?(n+4)/5-1:0;CHECK(g.album_page==last&&!g.album_expanded);
        g.album_expanded=true;pg_pointer(&g,0,155,218);pg_pointer(&g,2,310,218);CHECK(g.album_page==last&&g.album_expanded);
    }
}
int main(void){
    settings_swipes();
    paged_menus();
    pg_calendar c={2000,2,29,23,59,59},r;int64_t s;CHECK(pg_calendar_to_seconds(&c,&s));CHECK(pg_calendar_from_seconds(s,&r)&&r.day==29);CHECK(pg_calendar_from_seconds(s+1,&r)&&r.month==3&&r.day==1);c.year=2001;CHECK(!pg_calendar_to_seconds(&c,&s));CHECK(!pg_calendar_from_seconds(PG_CALENDAR_END,&r));
    for(int y=2000;y<=2099;++y)for(int m=1;m<=12;++m){c=(pg_calendar){y,m,1,12,34,56};CHECK(pg_calendar_to_seconds(&c,&s));CHECK(pg_calendar_from_seconds(s,&r));CHECK(r.year==y&&r.month==m&&r.minute==34&&r.second==56);}
    pg_game g;pg_init(&g,1);pg_utilities_step(&g,(pg_time_source){7,1000000,true});c=(pg_calendar){2026,10,27,12,30,0};pg_pet pet=g.pet;CHECK(pg_calendar_save(&g,&c));pg_utilities_step(&g,(pg_time_source){7,62000000,true});CHECK(pg_calendar_now(&g,&r)&&r.minute==31&&r.second==1);CHECK(!memcmp(&pet,&g.pet,sizeof pet));
    c.year=2099;CHECK(pg_calendar_save(&g,&c));CHECK(!memcmp(&pet,&g.pet,sizeof pet));pg_utilities_step(&g,(pg_time_source){8,1,true});CHECK(!pg_calendar_now(&g,&r));
    int16_t pcm[]={-32768,-101,0,101,32767};pg_pcm_gain(pcm,5,50);CHECK(pcm[0]==-32768&&pcm[1]==-101&&pcm[3]==101&&pcm[4]==32767);pg_pcm_gain(pcm,5,100);CHECK(pcm[0]==-32768&&pcm[1]==-202&&pcm[3]==202&&pcm[4]==32767);pg_pcm_gain(pcm,5,0);for(int i=0;i<5;++i)CHECK(pcm[i]==0);
    g.device.auto_sleep=true;g.sleeping=true;pg_utilities_step(&g,(pg_time_source){8,115000001,true});CHECK(!g.standby&&g.utilities.idle>=115);pg_user_activity(&g,PG_ACTIVITY_BUTTON);pg_utilities_step(&g,(pg_time_source){8,120000001,true});CHECK(!g.standby&&g.utilities.idle<6);pg_utilities_step(&g,(pg_time_source){8,240000001,true});CHECK(g.standby&&g.sleeping);pg_standby(&g,false);CHECK(!g.standby&&g.sleeping&&g.utilities.idle==0);
    g.device.auto_sleep=false;pg_utilities_step(&g,(pg_time_source){8,480000001,true});CHECK(!g.standby);g.device.volume=0;g.muted=false;CHECK(!pg_audio_enabled(&g));g.device.volume=25;CHECK(pg_audio_enabled(&g));
    pg_init(&g,2);g.pet.phase=PG_ALIVE;g.screen=PG_SETTINGS;g.utilities.settings_page=1;pg_tap(&g,233,145);CHECK(g.screen==PG_VOLUME);pg_tap(&g,333,230);CHECK(g.device.volume==75&&g.muted);pg_tap(&g,133,230);CHECK(g.device.volume==50&&g.muted);pg_back(&g);pg_tap(&g,233,220);CHECK(g.device.auto_sleep);pg_tap(&g,233,293);CHECK(g.screen==PG_CLOCK_EDIT);pg_tap(&g,289,359);CHECK(g.utilities.clock_error);
    printf("PASS Settings swipe/button navigation and gesture isolation, calendar boundaries, care independence, continuity, gain and Auto-sleep policy\n");
}
