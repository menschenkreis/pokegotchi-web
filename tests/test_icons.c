#include "icons.h"
#include "layout.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define CHECK(x) do{if(!(x)){fprintf(stderr,"FAIL %d: %s\n",__LINE__,#x);exit(1);}++checks;}while(0)
static unsigned checks;
static void art(void){
    uint16_t guarded[64*64+2];pg_canvas c={guarded+1,64,64};uint32_t hashes[PG_ICON_COUNT];
    for(int id=0;id<PG_ICON_COUNT;++id){
        memset(guarded,0,sizeof guarded);guarded[0]=0x1234;guarded[64*64+1]=0xabcd;
        pg_icon(&c,(pg_icon_id)id,32,32,2,0x91d5b2);hashes[id]=pg_crc32(c.pixels,64*64*2);
        int count=0;for(int i=0;i<64*64;++i)count+=c.pixels[i]!=0;
        CHECK(count>0);CHECK(guarded[0]==0x1234&&guarded[64*64+1]==0xabcd);
        for(int j=0;j<id;++j)CHECK(hashes[id]!=hashes[j]);
        pg_icon(&c,(pg_icon_id)id,-2,-2,8,0xffffff);CHECK(guarded[0]==0x1234&&guarded[64*64+1]==0xabcd);
    }
    /* The higher-detail masters must preserve the layout contract at every scale. */
    uint16_t *large=calloc(160*160,sizeof *large);CHECK(large);pg_canvas lc={large,160,160};
    for(int id=0;id<PG_ICON_COUNT;++id)for(int scale=1;scale<=8;++scale){
        memset(large,0,160*160*2);pg_icon(&lc,(pg_icon_id)id,80,80,scale,0xffffff);
        int ink=0,outside=0;
        for(int y=0;y<160;++y)for(int x=0;x<160;++x)if(large[y*160+x]){
            ++ink;if(x<80-8*scale||x>=80+8*scale||y<80-8*scale||y>=80+8*scale)++outside;
        }
        CHECK(ink>0&&outside==0);
    }
    free(large);
    uint32_t before=pg_crc32(guarded,sizeof guarded);
    pg_icon(&c,(pg_icon_id)-1,32,32,2,0xffffff);pg_icon(&c,PG_ICON_COUNT,32,32,2,0xffffff);
    pg_icon(&c,PG_ICON_HEART,32,32,0,0xffffff);pg_icon(&c,PG_ICON_HEART,32,32,9,0xffffff);
    CHECK(before==pg_crc32(guarded,sizeof guarded));
}
static void controls(void){
    pg_game g;pg_init(&g,42);pg_pointer(&g,0,233,28);pg_pointer(&g,2,233,100);pg_tap(&g,351,77);CHECK(g.screen==PG_SETTINGS);
    pg_tap(&g,313,394);pg_tap(&g,313,394);pg_tap(&g,233,219);CHECK(g.screen==PG_HELP&&g.icon_help_page==0);
    for(int i=1;i<=8;++i){pg_tap(&g,233,362);CHECK(g.icon_help_page==i%8);}
    pg_tap(&g,233,416);CHECK(g.screen==PG_SETTINGS);pg_back(&g);CHECK(g.screen==PG_HOME);
    g.pet.phase=PG_ALIVE;g.screen=PG_HOME;pg_tap(&g,113,385);CHECK(g.screen==PG_FOOD);
    /* Artwork and hit regions agree, including the upper tile and empty gutters. */
    pg_tap(&g,182,178);CHECK(g.screen==PG_FOOD&&g.pet.meals==0);
    g.pet.satiety=50;pg_tap(&g,128,175);CHECK(g.screen==PG_HOME&&g.pet.meals==1&&g.food_flavour==0);
    for(int i=0;i<PG_GAME_COUNT;++i){g.screen=PG_GAMES;g.games_page=i>=3;pg_tap(&g,233,180+(i%3)*73);CHECK(g.screen==(i==0?PG_TTT:i==1?PG_TILT:i==2?PG_RHYTHM:i==3?PG_ECHO:i==4?PG_BALANCE:PG_WASH));pg_back(&g);}
    g.screen=PG_HOME;pg_tap(&g,353,385);CHECK(g.screen==PG_ALBUM&&!g.album_expanded);
    for(int i=0;i<40;++i)pg_event_add(&g,PG_EV_FED,i%3);
    for(int page=0;page<7;++page){CHECK(g.album_page==page);pg_tap(&g,233,355);}CHECK(g.album_page==0);
    pg_tap(&g,180,141);CHECK(g.album_expanded&&g.album_selected.kind==PG_EV_FED);
    pg_event saved=g.album_selected;pg_event_add(&g,PG_EV_PETTED,0);CHECK(!memcmp(&saved,&g.album_selected,sizeof saved));
    pg_back(&g);CHECK(g.screen==PG_ALBUM&&!g.album_expanded);pg_back(&g);CHECK(g.screen==PG_HOME);
    g.pet.phase=PG_DEAD;g.screen=PG_MEMORIAL;pg_tap(&g,233,365);CHECK(g.pet.phase==PG_DEAD&&g.memorial_reset_armed);
    pg_back(&g);CHECK(g.pet.phase==PG_DEAD&&!g.memorial_reset_armed);
    pg_tap(&g,233,365);pg_tap(&g,233,365);CHECK(g.pet.phase==PG_EGG);
}
static void purity(void){
    uint16_t *pixels=malloc(PG_W*PG_H*2);CHECK(pixels);pg_canvas c={pixels,PG_W,PG_H};pg_game g;pg_init(&g,42);g.pet.phase=PG_ALIVE;
    for(int screen=0;screen<PG_SCREEN_COUNT;++screen){g.screen=(pg_screen)screen;pg_game before=g;pg_render(&c,&g);CHECK(!memcmp(&g,&before,sizeof g));CHECK(pixels[0]==pg_rgb(0x07151c));}
    free(pixels);
}
static void round_screen(void){
    /* Audit pixels before clipping: a final circular mask must never hide UI. */
    uint16_t *pixels=calloc(PG_W*PG_H,2);CHECK(pixels);pg_canvas c={pixels,PG_W,PG_H};
    unsigned audited=0;
    for(int species=0;species<3;++species)for(int screen=0;screen<PG_SCREEN_COUNT;++screen)for(int variant=0;variant<12;++variant){
        pg_game g;pg_init(&g,42);g.pet.phase=variant==0?PG_EGG:PG_ALIVE;g.pet.species=(pg_species)species;
        pg_ttt_start(&g);pg_tilt_start(&g);pg_rhythm_start(&g);pg_echo_start(&g);pg_balance_start(&g);pg_wash_start(&g);
        g.screen=(pg_screen)screen;g.games_page=variant%2;g.utilities.settings_page=variant%3;g.utilities.clock_field=variant%5;g.utilities.edit=(pg_calendar){2099,12,31,23,59,0};g.echo.score=variant>=8?UINT32_MAX:0;g.device.echo_best=g.echo.score;g.echo.attempts=variant%4;g.device.auto_sleep=variant==11;g.utilities.idle=variant==11?117:0;g.utilities.storage_fault=variant==10;g.utilities.clock_error=variant==9;g.icon_help_page=variant;g.pet.egg_visits=3;
        if(variant>=7)strcpy(g.pet.name,"ABCDEFGHIJKLMNO");
        if(screen==PG_NAME_EDIT){strcpy(g.name_edit,variant>=7?"ABCDEF":"MISO");g.naming_hatch=variant%2;}
        g.utilities.battery=(pg_battery_snapshot){.valid=variant>=4,.present=variant!=5,.charging=variant==6,.percent=variant==7?5:100,.synthetic=true};
        g.ttt.finished=g.tilt.finished=g.rhythm.finished=g.balance.finished=g.wash.finished=variant==2;
        g.balance.started=variant!=0;g.balance.angle=variant==3?0.65f:0;g.top_hud_seconds=g.hud_seconds=variant==2?0:8;g.debug_reset_armed=variant==3;
        g.echo.stage=variant==2?PG_ECHO_DONE:PG_ECHO_INPUT;
        g.x=g.target_x=variant==3?333:233;g.y=g.target_y=268;g.invitation=variant==3;
        g.emote=variant==3?PG_EMOTE_INVITE:PG_EMOTE_NONE;g.emote_time=1;g.emote_duration=2;
        for(int i=0;i<7;++i)pg_event_add(&g,PG_EV_WON,i%4);
        memset(pixels,0,PG_W*PG_H*2);pg_game before=g;pg_render_interface(&c,&g);
        CHECK(!memcmp(&g,&before,sizeof g));
        for(int y=0;y<PG_H;++y)for(int x=0;x<PG_W;++x)if(pixels[y*PG_W+x]){
            int dx=x-233,dy=y-233;
            if(dx*dx+dy*dy>PG_UI_SAFE_RADIUS*PG_UI_SAFE_RADIUS){
                fprintf(stderr,"Clipped/crowded UI screen=%d variant=%d species=%d pixel=(%d,%d)\n",screen,variant,species,x,y);exit(1);
            }
        }
        ++audited;
    }
    /* The physical corners must do nothing even through direct tap routing. */
    for(int screen=0;screen<PG_SCREEN_COUNT;++screen){
        pg_game g;pg_init(&g,42);g.pet.phase=PG_ALIVE;g.screen=(pg_screen)screen;
        for(int y=0;y<PG_H;y+=13)for(int x=0;x<PG_W;x+=13)if(!pg_ui_visible(x,y)){
            pg_game before=g;pg_tap(&g,x,y);CHECK(!memcmp(&before,&g,sizeof g));
        }
    }
    pg_game g;pg_init(&g,42);g.pet.phase=PG_ALIVE;
    const pg_screen destinations[]={PG_FOOD,PG_CARE,PG_GAMES,PG_ALBUM};
    for(int i=0;i<4;++i){g.screen=PG_HOME;pg_tap(&g,pg_ui_dock[i][0],pg_ui_dock[i][1]);CHECK(g.screen==destinations[i]);}
    g.screen=PG_SETTINGS;bool was_muted=g.muted;pg_tap(&g,30,180);CHECK(g.muted==was_muted);
    g.utilities.settings_page=0;pg_tap(&g,233,219);CHECK(g.muted!=was_muted);
    g.screen=PG_HOME;pg_tap(&g,113,420);CHECK(g.screen==PG_HOME); /* obsolete off-arc dock area */
    g.screen=PG_GAMES;g.games_page=0;pg_tap(&g,365,430);CHECK(g.games_page==0); /* formerly clipped page corner */
    pg_tap(&g,313,394);CHECK(g.games_page==1);pg_tap(&g,153,394);CHECK(g.games_page==0);
    pg_tap(&g,233,415);CHECK(g.screen==PG_HOME);
    pg_pointer(&g,0,233,252);pg_pointer(&g,2,0,0);CHECK(!g.pointer_down&&g.screen==PG_HOME);
    pg_open_game(&g,3);g.echo.stage=PG_ECHO_INPUT;g.echo.prompt=PG_SWIPE;
    pg_pointer(&g,0,233,238);pg_pointer(&g,2,465,465);CHECK(!g.pointer_down&&!g.echo.pointer_armed&&g.echo.score==0);
    free(pixels);printf("Round safe-area audit: %u unclipped interface variants, all screens and three species\n",audited);
}
static void participant_clearance(void){
    /* Compare every painted sprite pixel with the final composite, independently
       of the UI drawing order. This catches a portrait hidden behind controls. */
    uint16_t *sprite=calloc(PG_W*PG_H,2),*screen=calloc(PG_W*PG_H,2);CHECK(sprite&&screen);
    pg_canvas a={sprite,PG_W,PG_H},b={screen,PG_W,PG_H};unsigned poses=0;
    for(int species=0;species<3;++species)for(int stage=0;stage<3;++stage)
    for(int reaction=0;reaction<PG_EMOTE_COUNT;++reaction)for(int frame=0;frame<4;++frame)
    for(int view=0;view<4;++view){
        pg_game g;pg_init(&g,42);g.pet.phase=PG_ALIVE;g.pet.species=(pg_species)species;g.stage=(pg_stage)stage;
        g.screen=view<2?PG_TTT:PG_TILT;g.ttt.finished=g.tilt.finished=view%2;g.ttt.winner=1;
        pg_react(&g,(pg_emote)reaction,PG_SOUND_NONE);
        g.emote_time=g.emote_duration*(1-frame*.24f);g.anim=frame*.3f;
        memset(sprite,0,PG_W*PG_H*2);memset(screen,0,PG_W*PG_H*2);
        pg_creature(&a,&g,233,PG_UI_PAW_FEET,1);pg_render_interface(&b,&g);
        unsigned painted=0;
        for(int y=16;y<139;++y)for(int x=180;x<286;++x)if(sprite[y*PG_W+x]){
            CHECK(sprite[y*PG_W+x]==screen[y*PG_W+x]);++painted;
        }
        CHECK(painted>300);++poses;
    }
    for(int species=0;species<3;++species)for(int finished=0;finished<2;++finished){
        pg_game g;pg_init(&g,42);g.pet.phase=PG_ALIVE;g.pet.species=(pg_species)species;g.screen=PG_RHYTHM;
        g.rhythm.length=10;g.rhythm.lit=1;g.rhythm.finished=finished;g.facing=1;
        g.emote=finished?PG_EMOTE_CHEER:PG_EMOTE_CURIOUS;g.emote_duration=g.emote_time=1;
        memset(sprite,0,PG_W*PG_H*2);memset(screen,0,PG_W*PG_H*2);
        pg_creature(&a,&g,233,PG_UI_TUNES_FEET,1);pg_render_interface(&b,&g);
        for(int y=75;y<190;++y)for(int x=180;x<286;++x)if(sprite[y*PG_W+x])CHECK(sprite[y*PG_W+x]==screen[y*PG_W+x]);
        ++poses;
    }
    for(int species=0;species<3;++species)for(int stage=0;stage<3;++stage)
    for(int tilt=0;tilt<3;++tilt)for(int reaction=0;reaction<2;++reaction){
        pg_game g;pg_init(&g,42);g.pet.phase=PG_ALIVE;g.pet.species=(pg_species)species;g.stage=(pg_stage)stage;
        g.screen=PG_BALANCE;g.goal=PG_PLAY;g.pet.health=g.pet.satiety=100;
        g.balance.angle=(tilt-1)*.65f;g.balance.rescue=.2f;g.balance.started=tilt!=0;g.balance.finished=tilt==2;
        g.emote=reaction?PG_EMOTE_CHEER:PG_EMOTE_NONE;g.emote_duration=1;g.emote_time=.8f;g.facing=g.balance.angle<0?-1:1;
        int x=233+(int)(g.balance.angle*58),y=269+(int)(sinf(g.balance.angle)*g.balance.angle*35)-(int)(sinf(.2f*7)*9);
        memset(sprite,0,PG_W*PG_H*2);memset(screen,0,PG_W*PG_H*2);
        pg_creature(&a,&g,x,y,2);pg_render_interface(&b,&g);
        for(int row=100;row<308;++row)for(int col=100;col<366;++col)if(sprite[row*PG_W+col])CHECK(sprite[row*PG_W+col]==screen[row*PG_W+col]);
        ++poses;
    }
    pg_game g;
    for(int cell=0;cell<9;++cell){
        pg_init(&g,42);g.pet.phase=PG_ALIVE;pg_ttt_start(&g);
        pg_tap(&g,pg_ui_paw_board.x+(cell%3)*PG_UI_PAW_STEP+35,pg_ui_paw_board.y+(cell/3)*PG_UI_PAW_STEP+35);
        CHECK(g.ttt.cells[cell]==1);
    }
    pg_init(&g,42);g.pet.phase=PG_ALIVE;pg_ttt_start(&g);pg_tap(&g,233,126);CHECK(g.ttt.turn==1&&g.ttt.cells[1]==0);
    free(sprite);free(screen);printf("Participant clearance: %u complete active/result sprite poses and all nine touch cells\n",poses);
}
static void rectangle_pixels(void){
    /* A scalar reference checks alignment, odd spans and clipping independently
       of the optimized RGB565 implementation. Guards must remain unchanged. */
    const int widths[]={1,2,3,7,31,63,64,65};uint32_t rng=17;
    uint16_t actual[65*9+4],expected[65*9+4];
    for(unsigned wi=0;wi<sizeof widths/sizeof *widths;++wi)for(int alignment=0;alignment<2;++alignment){
        int width=widths[wi],offset=alignment+1;
        for(int attempt=0;attempt<1024;++attempt){
            for(unsigned n=0;n<sizeof actual/sizeof *actual;++n)actual[n]=expected[n]=0x5aa5;
            rng=rng*1664525u+1013904223u;int x=(int)(rng%(unsigned)(width+12))-6;
            rng=rng*1664525u+1013904223u;int y=(int)(rng%17)-4;
            rng=rng*1664525u+1013904223u;int w=(int)(rng%(unsigned)(width+12))-3;
            rng=rng*1664525u+1013904223u;int h=(int)(rng%17)-3;
            rng=rng*1664525u+1013904223u;uint32_t color=rng&0xffffff;
            pg_canvas c={actual+offset,width,9};pg_rect(&c,x,y,w,h,color);
            for(int row=0;row<9;++row)for(int col=0;col<width;++col)
                if(col>=x&&col<x+w&&row>=y&&row<y+h)expected[offset+row*width+col]=pg_rgb(color);
            CHECK(!memcmp(actual,expected,sizeof actual));
        }
    }
    printf("RGB565 fill: 16,384 scalar-reference/guard cases pass\n");
}
static void damage_regions(void){
    size_t bytes=PG_W*PG_H*2;
    uint16_t *a=malloc(bytes),*b=malloc(bytes),*shown=malloc(bytes);CHECK(a&&b&&shown);
    pg_canvas front={a,PG_W,PG_H},back_frame={b,PG_W,PG_H};pg_damage guarded[PG_DAMAGE_CAP+2];
    uint32_t rng=42;
    for(int trial=0;trial<64;++trial){
        for(int i=0;i<PG_W*PG_H;++i)a[i]=(uint16_t)(i*31+trial);
        memcpy(b,a,bytes);
        if(trial==1)for(int i=0;i<PG_W*PG_H;++i)b[i]^=0xffff;
        else if(trial>1){
            for(int i=0;i<1024;++i){rng=rng*1664525u+1013904223u;b[rng%(PG_W*PG_H)]^=0x3141;}
            b[0]^=1;b[PG_W*PG_H-1]^=1;
        }
        guarded[0]=(pg_damage){123,456,789,1011};guarded[PG_DAMAGE_CAP+1]=guarded[0];
        uint32_t before_a=pg_crc32(a,bytes),before_b=pg_crc32(b,bytes);
        int n=pg_frame_damage(&front,&back_frame,guarded+1,PG_DAMAGE_CAP);CHECK(n>=0&&n<=PG_DAMAGE_CAP);
        if(trial==0)CHECK(n==0);if(trial==1)CHECK(n==PG_DAMAGE_CAP);
        CHECK(pg_crc32(a,bytes)==before_a&&pg_crc32(b,bytes)==before_b);
        CHECK(guarded[0].x1==123&&guarded[PG_DAMAGE_CAP+1].y2==1011);
        memcpy(shown,a,bytes);
        for(int i=0;i<n;++i){pg_damage r=guarded[i+1];
            CHECK(r.x1>=0&&r.x1<=r.x2&&r.x2<PG_W&&r.y1>=0&&r.y1<=r.y2&&r.y2<PG_H);
            for(int y=r.y1;y<=r.y2;++y)memcpy(shown+y*PG_W+r.x1,b+y*PG_W+r.x1,(r.x2-r.x1+1)*2);
        }
        CHECK(!memcmp(shown,b,bytes));
    }
    CHECK(pg_frame_damage(&front,&back_frame,guarded,PG_DAMAGE_CAP-1)==-1);
    pg_canvas invalid=front;invalid.width=465;CHECK(pg_frame_damage(&invalid,&back_frame,guarded,PG_DAMAGE_CAP)==-1);
    free(a);free(b);free(shown);printf("Frame damage: 64 immutable-frame patch/guard cases cover all changed pixels\n");
}
static void compact_echo_portrait(void){
    /* The old two-buffer path is the pixel reference for the direct sampler. */
    uint16_t *screen=malloc(PG_W*PG_H*2),sprite[72*72];CHECK(screen);
    pg_canvas c={screen,PG_W,PG_H},sc={sprite,72,72};
    for(int species=0;species<3;++species)for(int stage=0;stage<=PG_ECHO_DONE;++stage)for(int frame=0;frame<8;++frame){
        pg_game g;pg_init(&g,17);g.pet.phase=PG_ALIVE;g.pet.species=species;g.screen=PG_ECHO;
        g.echo.stage=stage;g.echo.prompt=frame%3;g.anim=frame*.17f;
        pg_game pose=g;pose.facing=g.echo.prompt==PG_SWIPE?-1:1;
        pose.emote=stage==PG_ECHO_CUE?PG_EMOTE_INVITE:stage==PG_ECHO_INPUT?PG_EMOTE_CURIOUS:stage==PG_ECHO_RETRY?PG_EMOTE_SASS:PG_EMOTE_CHEER;
        pose.emote_time=pose.emote_duration=1;
        for(int i=0;i<72*72;++i)sprite[i]=pg_rgb(0xff00ff);
        pg_creature(&sc,&pose,36,66,1);pg_render(&c,&g);
        int ink=0;for(int y=0;y<48;++y)for(int x=0;x<48;++x){
            uint16_t expected=sprite[(y*72/48)*72+x*72/48];
            if(expected!=pg_rgb(0xff00ff)){CHECK(screen[(129+y)*PG_W+209+x]==expected);++ink;}
        }CHECK(ink>0);
    }
    free(screen);
}
int main(void){compact_echo_portrait();art();controls();purity();round_screen();participant_clearance();rectangle_pixels();damage_regions();printf("PASS %u icon/interface checks; %d original sprites, clipping, targets, guide, journal, memorial, pure rendering\n",checks,PG_ICON_COUNT);return 0;}
