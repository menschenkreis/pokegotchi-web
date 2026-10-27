#include "draw.h"
#include "icons.h"
#include "layout.h"
#include "forest_assets.h"
#include <math.h>
#include <stdio.h>
#include <string.h>

#define INK 0x293523
#define CREAM 0xf8edcf
#define GOLD 0xf4c979
#define MINT 0xb5d397
#define MUTED 0x9cae84
#define PANEL 0x283c2c
#define PANEL2 0x40533a
#define CORAL 0xe99683

static uint32_t mix(uint32_t a, uint32_t b, float t) {
    t=pg_clamp(t,0,1);
    int r=(int)(((a>>16)&255)*(1-t)+((b>>16)&255)*t);
    int g=(int)(((a>>8)&255)*(1-t)+((b>>8)&255)*t);
    int bl=(int)((a&255)*(1-t)+(b&255)*t);
    return (uint32_t)(r<<16|g<<8|bl);
}
static void leaf(pg_canvas *c,int x,int y,int direction,uint32_t color) {
    pg_ellipse(c,x+direction*7,y-5,10,5,color);
    pg_line(c,x,y,x+direction*13,y-7,1,0x416856);
}
static void scene_background(pg_canvas *c,const pg_game *g) {
    /* A fixed 160px RGB565 art plate, expanded directly into the 466px canvas.
       Both variants live in flash; no decode, heap allocation or animation buffer. */
    bool night=g->sleeping;
    const uint16_t *plate=night?pg_forest_night:pg_forest_day;
    for(int y=0;y<466;++y){
        const uint16_t *src=plate+(y*160/466)*160;
        uint16_t *dst=c->pixels+y*466;
        for(int x=0;x<466;++x)dst[x]=src[x*160/466];
    }
    /* Non-home panels cover these home-only decorations. */
    if(g->screen!=PG_HOME)return;
    /* A chunky brass lantern on a carved wooden hook: an in-world control.
       Its pane and switch live inside pg_ui_lamp. */
    float pulse=.5f+.5f*sinf(g->anim*1.8f);
    uint32_t brass=night?0xb99c70:0xd8ab65;
    if(!night){
        pg_ellipse(c,119,216,44,48,mix(0x779a69,0x829f6d,pulse));
        pg_ellipse(c,119,216,35,40,0xa8b87b);
        pg_ellipse(c,119,216,28,34,0xccca8b);
    }
    pg_ellipse(c,99,285,26,6,0x34584c);
    pg_line(c,85,279,85,173,8,INK);
    pg_line(c,85,278,85,174,4,0x946e53);
    pg_line(c,86,173,119,173,7,INK);
    pg_line(c,86,173,119,173,3,0xc49c75);
    pg_line(c,88,185,100,175,3,0x946e53);
    pg_ellipse(c,119,182,9,9,INK);pg_ellipse(c,119,182,5,5,brass);
    pg_roundrect(c,91,188,56,62,9,INK);
    pg_roundrect(c,95,192,48,54,6,g->lamp_touch?CREAM:brass);
    pg_roundrect(c,100,197,38,44,4,INK);
    pg_roundrect(c,103,200,32,38,3,night?0x3e5866:0xf4d98e);
    pg_rect(c,108,203,5,31,night?0x587381:0xfff2c9);
    pg_rect(c,116,200,5,38,night?0x8b8067:0xd4a261);
    pg_line(c,100,216,137,216,3,night?0x8b8067:0xd4a261);
    pg_roundrect(c,89,187,60,9,3,INK);pg_rect(c,94,189,50,3,brass);
    pg_roundrect(c,89,244,60,9,3,INK);pg_rect(c,95,246,48,3,brass);
    /* Raised rocker has a next-action sun/moon; it stays legible when unlit. */
    pg_roundrect(c,100,251,38,26,7,INK);
    pg_roundrect(c,103,254,32,20,5,g->lamp_touch?GOLD:0xe3c89a);
    pg_line(c,107,274,131,274,2,0x8c6d50);
    pg_icon(c,night?PG_ICON_SUN:PG_ICON_MOON,119,264,1,INK);
    /* Fireflies at night, drifting pollen and butterflies by day. */
    for(int i=0;i<7;++i){int x=94+i*43+(int)(sinf(g->anim*.6f+i)*10),y=178+i%3*29+(int)(sinf(g->anim+i*2)*9);
        if(night){float tw=.5f+.5f*sinf(g->anim*2+i);pg_ellipse(c,x,y,3,3,mix(0x344e58,0x9aa976,tw));pg_rect(c,x,y,2,2,mix(0x5b7578,0xffe4a2,tw));}
        else if(i%3==0){int wing=2+(int)(fabsf(sinf(g->anim*6+i))*3);pg_ellipse(c,x-wing,y-1,wing,3,0xf7d397);pg_ellipse(c,x+wing,y-1,wing,3,0xf2b698);pg_line(c,x,y-3,x,y+3,1,INK);}
        else pg_rect(c,x,y,2,2,0xd7dfaa);
    }
}
static void home_objects(pg_canvas *c,const pg_game *g) {
    uint32_t bed=g->pet.bed_style==1?0x7993a0:g->pet.bed_style==2?0xc49c75:0x9c8f83;
    /* A padded basket, rolled rim, plump pillow and stitched quilt. */
    pg_ellipse(c,195,300,58,12,0x365548);
    pg_roundrect(c,141,270,108,31,14,0x655e58);
    pg_roundrect(c,145,266,100,30,13,bed);
    pg_roundrect(c,151,270,88,24,10,0xe1d5b5);
    pg_roundrect(c,156,263,43,17,7,0xf6e5c6);
    pg_line(c,164,267,188,267,2,CREAM);
    pg_roundrect(c,171,281,65,14,5,mix(bed,MINT,.25f));
    for(int i=0;i<5;++i)pg_line(c,179+i*11,285,179+i*11,290,1,0xc5d5b2);
    int sway=(int)(sinf(g->anim*0.7f)*2);
    pg_ellipse(c,352,285,21,7,0x3b5a4d);
    pg_roundrect(c,336,265,32,19,4,0xab7660);
    pg_rect(c,339,268,26,3,0xd79c77);
    pg_line(c,351,266,350+sway,219,3,0x3b6b4d);
    leaf(c,350+sway,242,-1,0x94bc7b);leaf(c,350+sway,231,1,0xa5c383);
    leaf(c,350+sway,252,1,0x789e69);
    if(g->pet.plant_style) {
        pg_ellipse(c,350+sway,217,9,8,g->pet.plant_style==1?CORAL:0xb2aff0);
        pg_ellipse(c,350+sway,217,3,3,GOLD);
    } else leaf(c,350+sway,219,-1,0xc3d18a);
    if(g->pet.toy_style==0) {
        pg_ellipse(c,303,296,12,9,0xc89a69);pg_ellipse(c,301,293,9,8,GOLD);
        pg_line(c,296,289,306,299,2,0xbe805d);
    } else if(g->pet.toy_style==1) {
        pg_star(c,305,291,11,GOLD);pg_ellipse(c,305,291,4,4,CREAM);
    } else {
        pg_roundrect(c,292,283,23,17,4,0x9d91bd);pg_rect(c,297,286,13,4,0xd3b6c8);
    }
    if(g->pet.legacy_count) {
        uint32_t colors[]={MINT,CORAL,0xb9b6e5};
        pg_ellipse(c,100,317,9,4,0x537164);
        pg_line(c,100,315,100,301,2,0xb1ae87);pg_heart(c,100,300,5,colors[g->pet.legacy_species]);
    }
}
/* Transparent RGB565 frames are composited directly from flash. */
static void forest_sprite(pg_canvas *c,const uint16_t *frames,int w,int h,int frame,int x,int y){
    const uint16_t *p=frames+(size_t)frame*w*h;
    const uint16_t key=pg_rgb(0xff00ff);
    for(int yy=0;yy<h;++yy)for(int xx=0;xx<w;++xx){
        int dx=x+xx,dy=y+yy;uint16_t color=p[yy*w+xx];
        if(color!=key&&(unsigned)dx<(unsigned)c->width&&(unsigned)dy<(unsigned)c->height)
            c->pixels[dy*c->width+dx]=color;
    }
}
static void draw_egg(pg_canvas *c,const pg_game *g) {
    int x=233,y=252;
    bool warming=g->pet.phase==PG_EGG&&g->warmth_time>0&&!g->sleeping;
    float age=2.8f-g->warmth_time;
    float envelope=warming?fminf(1,age*5)*fminf(1,g->warmth_time*3):0;
    if(warming){
        /* A nest-sized glow and rising ribbons say warmth, not fire or hatch. */
        pg_ellipse(c,233,268,67,43,mix(0x4b6050,0x9c8658,.55f*envelope));
        for(int side=-1;side<=1;side+=2)for(int i=0;i<2;++i){
            float t=fmodf(age*.65f+i*.5f,1);
            int wx=233+side*(58+(int)(sinf(t*6)*3)),wy=278-(int)(t*62);
            uint32_t color=mix(0x64765e,GOLD,envelope*(1-t));
            pg_line(c,wx,wy,wx+side*3,wy-7,3,color);
            pg_line(c,wx+side*3,wy-7,wx,wy-14,3,color);
        }
    }
    pg_ellipse(c,233,301,58,12,0x4b6050);
    for(int i=0;i<7;++i)pg_ellipse(c,233,299-i,55-i*2,8, i%2?0xb69e72:0xc8b685);
    int frame;
    if(g->pet.phase==PG_HATCHING){
        if(g->hatch_time<1.5f)frame=1+((int)(g->hatch_time*5)&1);
        else if(g->hatch_time<3)frame=4;
        else if(g->hatch_time<4.5f)frame=5;
        else frame=6;
    }else if(g->sleeping)frame=7;
    else if(warming)frame=3;
    else if(g->pet.incubation>PG_EGG_SECONDS*.8f)frame=4;
    else {static const int idle[]={0,1,0,2};frame=idle[((int)(g->anim*1.2f))&3];}
    forest_sprite(c,pg_forest_egg,80,112,frame,193,196);
    if(g->pet.phase==PG_HATCHING && g->hatch_time>4) {
        pg_star(c,x-53,y-24,10,GOLD);pg_star(c,x+49,y-7,8,CREAM);
        pg_star(c,x+9,y-68,7,MINT);
    }
    if(warming){
        int approach=(int)((1-envelope)*12);
        /* Two cupped mitten hands, thumbs turned toward the shell. */
        for(int side=-1;side<=1;side+=2){
            int hx=233+side*(47+approach),hy=286;
            pg_ellipse(c,hx,hy,17,11,0x8b6f59);
            pg_ellipse(c,hx,hy-3,15,10,0xf0cfaa);
            pg_ellipse(c,hx-side*9,hy-9,7,10,CREAM);
            pg_line(c,hx+side*4,hy-3,hx+side*9,hy-6,2,0xceaa85);
        }
        if(age>.45f){
            float t=pg_clamp((age-.45f)/2.35f,0,1);
            pg_heart(c,x-28,y-61-(int)(t*14),6,CORAL);
            if(age>1.1f)pg_heart(c,x+30,y-52-(int)(t*18),4,GOLD);
        }
    }else if(g->petting>0) {
        pg_heart(c,200,y-57,7,CORAL);pg_star(c,275,y-40,5,GOLD);
    }
    if(g->pet.warmth<40) {
        pg_line(c,185,246,177,252,2,0x9fc4c9);pg_line(c,279,252,287,246,2,0x9fc4c9);
    }
}

/* Deterministic cosmetic particles: no heap, emitter state or care RNG.
   Effects stay inside the existing 72px participant canvas. */
static void reaction_particles(pg_canvas *c,const pg_game *g,pg_emote e,float age){
    if(e==PG_EMOTE_NONE||g->emote_time<=0)return;
    float envelope=pg_clamp(age*6,0,1)*pg_clamp(g->emote_time*4,0,1);
    if(envelope<.12f)return;
    uint32_t accent=g->pet.species==PG_MOSSLET?MINT:g->pet.species==PG_EMBERKIN?GOLD:0xc1c7ec;
    int count=e==PG_EMOTE_CHEER?10:e==PG_EMOTE_CLEAN?7:4;
    for(int i=0;i<count;++i){
        uint32_t hash=g->performance_variant+(uint32_t)i*UINT32_C(0x9e3779b9);hash^=hash>>16;
        float t=age-(hash%13)*.018f;
        if(t<0)continue;
        float p=fmodf(t*(e==PG_EMOTE_CHEER?.70f:.55f)+i*.21f,1);
        int side=i%2?1:-1;
        int x=36+side*(24+(int)((hash>>8)%7)),y=49-(int)(p*37);
        int r=(int)(envelope*(1-p)*3.5f);
        if(r<1)continue;
        switch(e){
        case PG_EMOTE_CUDDLE:
            x+=(int)(sinf(t*4+i)*2);pg_heart(c,x,y,r+1,i%2?CORAL:GOLD);break;
        case PG_EMOTE_GREET:
            if(i<2)pg_heart(c,x,y,r,CORAL);
            else{pg_line(c,x-2,y,x+2,y,1,GOLD);pg_line(c,x,y-2,x,y+2,1,GOLD);}break;
        case PG_EMOTE_CHEER:
            y=12+(int)(p*p*47);x=36+side*(20+(int)(p*10));
            if(i%3==0)pg_star(c,x,y,r+1,GOLD);
            else pg_line(c,x,y,x+(i%2?2:-2),y+2,2,i%3==1?CORAL:accent);
            break;
        case PG_EMOTE_CLEAN:
            pg_ellipse(c,x,y,r+1,r+1,0x9bcbd5);pg_ellipse(c,x,y,r,r,INK);pg_pixel(c,x-1,y-1,CREAM);
            if(p>.72f)pg_star(c,x,y,2,CREAM);
            break;
        case PG_EMOTE_CURIOUS:
        case PG_EMOTE_INVITE:
            if(i==0){int bob=(int)(sinf(t*5)*2);
                pg_roundrect(c,50,5+bob,20,21,5,INK);
                pg_icon(c,e==PG_EMOTE_CURIOUS?PG_ICON_QUESTION:PG_ICON_GAME,60,15+bob,1,GOLD);}
            else if(i==2)pg_star(c,10,23-(int)(p*9),r,accent);
            break;
        case PG_EMOTE_OOPS:
            if(i<2){y=22+(int)(p*15);pg_ellipse(c,x,y,2,3,0x9bcbd5);pg_pixel(c,x,y-3,CREAM);}break;
        case PG_EMOTE_YAWN:
            if(i<3){x=57+(i%2)*5;y=32-(int)(p*22);pg_ellipse(c,x,y,r,r,0xa6bce0);pg_pixel(c,x-1,y-1,CREAM);}break;
        case PG_EMOTE_SASS:
            if(i<2){x=36+side*27;y=30-(int)(p*13);pg_ellipse(c,x,y,r+1,r,CREAM);}break;
        case PG_EMOTE_WOBBLE:
            if(i<3){float angle=t*5+i*2.094f;pg_star(c,36+(int)(cosf(angle)*24),12+(int)(sinf(angle)*4),2,GOLD);}break;
        default:break;
        }
    }
}

/* Fixed sprite frames use the same RGB565 pixels on desktop and MCU. */
static void creature(pg_canvas *c,const pg_game *g,int x,int feet_y,int size,bool compact,bool arena) {
    uint16_t pixels[72*72];
    pg_canvas s={pixels,72,72};
    const uint16_t key=pg_rgb(0xff00ff);
    pg_emote e=g->emote_time>0?g->emote:PG_EMOTE_NONE;
    float age=g->emote_duration-g->emote_time;
    bool moving=g->screen==PG_HOME&&hypotf(g->x-g->target_x,g->y-g->target_y)>(g->sleeping?1:3);
    bool resting=g->screen==PG_HOME&&!moving&&(g->sleeping||(g->goal==PG_REST&&g->goal_time>3));
    if(g->sleeping&&moving)e=PG_EMOTE_NONE;
    float f=g->pet.satiety/70.0f,en=g->pet.energy/95.0f,cl=g->pet.clean/70.0f;
    bool unresolved=g->pet.neglect_active;
    bool hungry=unresolved?g->pet.satiety<70&&f<=en&&f<=cl:g->pet.satiety<=50;
    bool tired=unresolved?g->pet.energy<95&&en<f&&en<=cl:g->pet.energy<=50&&!hungry;
    bool smudgy=unresolved?g->pet.clean<70:g->pet.clean<=50;
    int step=(int)(age*6.0f)&1, frame=0;
    int idle_step=(int)(g->anim*.35f)&1;
    if(resting&&e!=PG_EMOTE_YAWN)frame=idle_step?27:15;
    else if(e!=PG_EMOTE_NONE&&g->emote_time<=.35f){
        /* Let every performance settle into the neutral pose before expiry. */
        frame=(e==PG_EMOTE_YAWN&&(g->sleeping||resting))?15:8;
    }else if(e!=PG_EMOTE_NONE){
        /* Each expression is a complete authored frame. Switching frames
           never stretches, rotates or shears the pixel-art creature. */
        switch(e){
        case PG_EMOTE_GREET: frame=step?4:0;break;
        case PG_EMOTE_CUDDLE: frame=step?21:20;break;
        case PG_EMOTE_NIBBLE:
            frame=age<.12f?0:age<.45f?5:age<2.15f?(step?6:0):4;
            break;
        case PG_EMOTE_CURIOUS: frame=step?5:0;break;
        case PG_EMOTE_INVITE: frame=step?4:5;break;
        case PG_EMOTE_CHEER: frame=step?23:4;break;
        case PG_EMOTE_OOPS: frame=step?19:18;break;
        case PG_EMOTE_CLEAN: frame=step?23:22;break;
        case PG_EMOTE_YAWN: frame=step?7:12;break;
        case PG_EMOTE_SASS: frame=step?16:18;break;
        case PG_EMOTE_WOBBLE: frame=step?2:1;break;
        case PG_EMOTE_FRIGHT: frame=step?17:16;break;
        default: break;
        }
    }else if(moving)frame=1+((int)(g->anim*5.0f)&1);
    else if(hungry)frame=idle_step?28:10;
    else if(tired)frame=idle_step?29:12;
    else if(g->blink<0)frame=26;
    else {
        /* Long holds and tiny, registered pixel changes suggest breathing. */
        int phase=(int)fmodf(g->anim,8.0f);
        frame=phase==2||phase==3?24:phase==5||phase==6?25:8;
    }
    const uint16_t *art=g->pet.species==PG_MOSSLET?pg_forest_pet_0:
                        g->pet.species==PG_EMBERKIN?pg_forest_pet_1:pg_forest_pet_2;
    memcpy(pixels,art+(size_t)frame*72*72,sizeof pixels);
    if(smudgy){pg_ellipse(&s,22,52,3,2,0x786f61);pg_line(&s,48,49,53,51,2,0x786f61);}
    if(e==PG_EMOTE_NIBBLE&&frame==6&&age<1.85f){
        uint32_t food=g->food_flavour==0?CORAL:g->food_flavour==1?GOLD:0xb5cb86;
        pg_ellipse(&s,36,49,3,3,food);
    }
    reaction_particles(&s,g,e,age);
    if(compact){
        /* Direct nearest-neighbour sampling keeps compact game portraits
           bounded without another intermediate framebuffer. */
        for(int j=0;j<48;++j)for(int i=0;i<48;++i){
            int dx=x-24+i,dy=feet_y-24+j;
            if(arena&&(dx-233)*(dx-233)+(dy-244)*(dy-244)>114*114)continue;
            uint16_t p=pixels[(j*72/48)*72+i*72/48];
            if(p!=key&&(unsigned)dx<(unsigned)c->width&&(unsigned)dy<(unsigned)c->height)
                c->pixels[dy*c->width+dx]=p;
        }
        return;
    }
    int origin_x=x-36*size,origin_y=feet_y-66*size;
    for(int y0=0;y0<72;++y0)for(int x0=0;x0<72;++x0){
        uint16_t p=pixels[y0*72+x0];if(p==key)continue;
        int px=origin_x+x0*size,py=origin_y+y0*size;
        for(int yy=0;yy<size;++yy)for(int xx=0;xx<size;++xx)
            if((unsigned)(px+xx)<(unsigned)c->width&&(unsigned)(py+yy)<(unsigned)c->height)
                c->pixels[(py+yy)*c->width+px+xx]=p;
    }
}

void pg_creature(pg_canvas *c,const pg_game *g,int x,int feet_y,int size){creature(c,g,x,feet_y,size,false,false);}

/* Artwork and input share the round display layout. */
/* Two hand-drawn leaf pixels make a quiet, frame-based menu ornament. */
static void menu_leaf(pg_canvas *c,int x,int y,int frame){
    static const uint8_t rows[2][7]={
        {0x02,0x07,0x0f,0x1e,0x1c,0x08,0x08},
        {0x08,0x1c,0x1e,0x0f,0x07,0x02,0x02}};
    for(int yy=0;yy<7;++yy)for(int xx=0;xx<5;++xx)
        if(rows[frame][yy]&(1<<(4-xx)))pg_rect(c,x+xx*2,y+yy*2,2,2,0xa3bf7c);
}
/* Carved wooden plaques share a small pixel border across menus. */
static void wood_plaque(pg_canvas *c,int x,int y,int w,int h,uint32_t color){
    pg_roundrect(c,x,y+2,w,h,8,0x263222);
    pg_roundrect(c,x,y,w,h,8,0x8b6948);
    pg_roundrect(c,x+3,y+3,w-6,h-6,5,color);
    if(w>=50&&h>=28){
        pg_rect(c,x+8,y+3,w-16,2,0xb49465);
        pg_rect(c,x+8,y+h-5,w-16,2,0x654c39);
        pg_rect(c,x+5,y+6,3,3,GOLD);pg_rect(c,x+w-8,y+6,3,3,GOLD);
        pg_rect(c,x+5,y+h-9,3,3,0x654c39);pg_rect(c,x+w-8,y+h-9,3,3,0x654c39);
    }
}
/* Small, fixed pixel-art frames shared by the mini-games. Each frame is a
   complete sprite; animation selects a frame without scaling or deformation. */
typedef enum { FOREST_ACORN, FOREST_STAR, FOREST_BUBBLE, FOREST_NOTE, FOREST_LEAF } forest_game_sprite;
static void game_sprite(pg_canvas *c,forest_game_sprite sprite,int frame,int cx,int cy,int scale){
    static const char art[5][2][9][10]={
        { {"...111...","..12221..",".1333331.","123333321","123444321",".2344432.","..23432..","...232...","....2...."},
          {"....11...","..11221..",".1333331.","123333321","123444321",".2344432.","..23432..","...232...","....2...."} },
        { {"....4....","....4....","...444...","4..444..4",".4444444.","..44444..",".44.4.44.","....4....","........."},
          {".........","..4...4..","...444...","...444...",".4444444.","...444...","...4.4...","..4...4..","........."} },
        { {"..11111..",".1222221.","123322221","123322221","122222221","122222221",".1222221.","..11111..","........."},
          {"...111...","..12221..",".1232221.","123322221","122222221",".1222221.","..12221..","...111...","........."} },
        { {"....11...","....122..","....1222.","....1..2.","....1....","..111....",".1331....",".1331....","..11....."},
          {"....11...","....122..","....1.22.","....1..2.","....1....","..111....",".1331....",".1331....","..11....."} },
        { {"......11.","....1121.","..112221.",".1223321.",".1233321.","..12321..","...121...","..1......",".1......."},
          {".11......",".12211...",".122221..",".1233221.",".1233321.","..12321..","...121...","......1..",".......1."} }
    };
    static const uint32_t colors[5][4]={
        {0x604334,0x9a673e,0xcf985d,0xf2c677},
        {0xf5c969,0xf5c969,0xf5c969,0xffe7a4},
        {0x80b7a7,0xc1e2cb,0xf2f5db,0xf2f5db},
        {0x5c4935,0xd39c60,0xf3d08b,0xf9e3b3},
        {0x405a36,0x799a4c,0xa9c273,0xd4dda0}
    };
    for(int y=0;y<9;++y)for(int x=0;x<9;++x){
        char p=art[sprite][frame&1][y][x];
        if(p>='1'&&p<='4')pg_rect(c,cx+(x-4)*scale,cy+(y-4)*scale,scale,scale,colors[sprite][p-'1']);
    }
}
static void icon_button(pg_canvas *c,int x,int y,int w,int h,pg_icon_id a,pg_icon_id b,uint32_t color){
    wood_plaque(c,x,y,w,h,color);
    if(b==PG_ICON_COUNT)pg_icon(c,a,x+w/2,y+h/2,2,CREAM);
    else {pg_icon(c,a,x+w/2-25,y+h/2,2,CREAM);pg_icon(c,b,x+w/2+25,y+h/2,2,GOLD);}
}
static void icon_header(pg_canvas *c,pg_icon_id id){
    pg_icon(c,id,233,65,3,MINT);
}
static void control(pg_canvas *c,pg_ui_rect r,pg_icon_id id){icon_button(c,r.x,r.y,r.w,r.h,id,PG_ICON_COUNT,PANEL2);}
static void ui_panel(pg_canvas *c,pg_ui_rect r,uint32_t color){
    wood_plaque(c,r.x,r.y,r.w,r.h,color);
}
static void ui_button(pg_canvas *c,pg_ui_rect r,pg_icon_id a,pg_icon_id b,uint32_t color){icon_button(c,r.x,r.y,r.w,r.h,a,b,color);}
static void back(pg_canvas *c){control(c,pg_ui_back,PG_ICON_BACK);}
static void pips(pg_canvas *c,int x,int y,int count,float filled,uint32_t color){
    for(int i=0;i<count;++i)pg_roundrect(c,x+i*16,y,11,8,2,i<filled?color:0x5e6f49);
}
static void need_meter(pg_canvas *c,int x,int y,int width,pg_icon_id id,float amount,uint32_t color){
    pg_icon(c,id,x+15,y+20,2,color);
    int bar_x=x+38,bar_w=width-38;
    char value[8];snprintf(value,sizeof value,"%.0f",(double)pg_clamp(amount,0,100));
    pg_text(c,bar_x,y,value,3,CREAM);
    pg_roundrect(c,bar_x,y+25,bar_w,13,3,0x273625);
    int w=(int)((bar_w-4)*pg_clamp(amount,0,100)/100);
    if(w>0)pg_rect(c,bar_x+2,y+28,w,7,color);
    for(int i=1;i<5;++i)pg_rect(c,bar_x+bar_w*i/5,y+25,2,13,PANEL);
}
static void count_icon(pg_canvas *c,pg_icon_id id,int x,int y,unsigned value,uint32_t color){
    char n[16];snprintf(n,sizeof n,"%u",value);pg_icon(c,id,x,y,1,color);pg_text(c,x+15,y-6,n,2,CREAM);
}
static void game_count(pg_canvas *c,pg_icon_id id,int x,unsigned value,uint32_t color){
    char n[16];snprintf(n,sizeof n,"%u",value);size_t digits=strlen(n);
    pg_icon(c,id,x,64,2,color);pg_center(c,x,89,n,digits<=5?3:digits<=8?2:1,CREAM);
}
static pg_icon_id event_icon(const pg_event *e){
    static const pg_icon_id icons[]={PG_ICON_EGG,PG_ICON_WARM,PG_ICON_SPROUT,PG_ICON_BOWL,
        PG_ICON_HEART,PG_ICON_CLEAN,PG_ICON_MOON,PG_ICON_TROPHY,PG_ICON_REPLAY,PG_ICON_GAME,
        PG_ICON_SPROUT,PG_ICON_SPARKLE,PG_ICON_HEART,PG_ICON_EAR,PG_ICON_HEART,PG_ICON_EGG};
    return e->kind<sizeof icons/sizeof *icons?icons[e->kind]:PG_ICON_BOOK;
}
static void thought(pg_canvas *c,const pg_game *g){
    pg_icon_id a=PG_ICON_HEART,b=PG_ICON_SPARKLE;
    bool egg=g->pet.phase==PG_EGG||g->pet.phase==PG_HATCHING;
    uint32_t color=MINT;
    if(egg){
        a=g->pet.warmth<40?PG_ICON_COLD:g->pet.warmth>85?PG_ICON_HOT:PG_ICON_HOLD;
        b=g->pet.phase==PG_HATCHING?PG_ICON_SPARKLE:PG_ICON_EGG;
        if(g->pet.warmth<40||g->pet.warmth>85)color=GOLD;
    }else if(g->pet.health<30||g->pet.satiety<25){a=PG_ICON_ALERT;b=PG_ICON_BOWL;color=CORAL;}
    else if(g->sleeping){a=PG_ICON_MOON;b=PG_ICON_TAP;}
    else if(g->emote_time>0){
        static const pg_icon_id emotes[]={PG_ICON_HEART,PG_ICON_PAW,PG_ICON_HEART,PG_ICON_BOWL,
            PG_ICON_EYE,PG_ICON_GAME,PG_ICON_TROPHY,PG_ICON_REPLAY,PG_ICON_CLEAN,PG_ICON_MOON,PG_ICON_ALERT,PG_ICON_TILT_GESTURE};
        a=g->emote==PG_EMOTE_FRIGHT?PG_ICON_ALERT:emotes[g->emote<PG_EMOTE_FRIGHT?g->emote:0];
        b=g->emote==PG_EMOTE_OOPS?PG_ICON_HEART:g->emote==PG_EMOTE_INVITE?PG_ICON_QUESTION:PG_ICON_SPARKLE;
    }else if(g->caption_time>0&&g->pet.event_count){a=event_icon(&g->pet.events[(g->pet.event_count-1)%PG_EVENT_CAP]);}
    else{
        static const pg_icon_id goals[]={PG_ICON_BOWL,PG_ICON_MOON,PG_ICON_PAW,PG_ICON_SPROUT,PG_ICON_GAME,PG_ICON_HEART,PG_ICON_GAME};
        a=goals[g->goal<PG_GOAL_COUNT?g->goal:0];b=g->goal==PG_INVITE?PG_ICON_QUESTION:PG_ICON_HEART;
    }
    pg_roundrect(c,164,322,138,40,12,PANEL);
    pg_icon(c,a,211,344,2,color);pg_icon(c,b,255,344,2,CREAM);
    if(!egg&&g->pet.health<30)pg_center(c,233,307,"NEEDS FOOD",1,CORAL);
}
static void compact_pet(pg_canvas *c,const pg_game *g,int x,int y,bool arena){
    creature(c,g,x,y,1,true,arena);
}
static void battery_scaled(pg_canvas *c,const pg_game *g,int x,int y,int s){
    const pg_battery_snapshot *b=&g->utilities.battery;
    pg_roundrect(c,x,y,32*s,16*s,3*s,MUTED);pg_rect(c,x+2*s,y+2*s,28*s,12*s,PANEL);pg_rect(c,x+32*s,y+5*s,3*s,6*s,MUTED);
    if(!b->valid){pg_center(c,x+16*s,y+4*s,"?",s,GOLD);return;}
    if(!b->present){pg_center(c,x+16*s,y+4*s,"X",s,MUTED);return;}
    unsigned n=(b->percent+24)/25;
    for(unsigned i=0;i<n&&i<4;++i)pg_rect(c,x+(4+(int)i*6)*s,y+4*s,4*s,8*s,b->percent<=20?CORAL:MINT);
    if(b->charging)pg_icon(c,PG_ICON_SPARKLE,x+16*s,y+8*s,s,GOLD);
}
static void battery_glyph(pg_canvas *c,const pg_game *g,int x,int y){battery_scaled(c,g,x,y,1);}
static void home_status(pg_canvas *c,const pg_game *g,bool egg){
    /* Narrow crown, wider second row: every pixel fits the upper circular arc. */
    pg_roundrect(c,135,32,196,36,18,PANEL);
    if(egg){pg_icon(c,PG_ICON_EGG,210,48,2,GOLD);pg_icon(c,PG_ICON_HEART,256,48,2,CREAM);}
    else pg_center(c,233,38,g->pet.name,strlen(g->pet.name)>8?2:3,CREAM);
    pg_roundrect(c,135,70,196,45,18,PANEL);
    pg_calendar t;char clock[12];
    if(pg_calendar_now(g,&t))snprintf(clock,sizeof clock,"%02d:%02d",t.hour,t.minute);
    else snprintf(clock,sizeof clock,"--:--");
    pg_center(c,185,82,clock,3,MINT);battery_scaled(c,g,245,76,2);
    pg_ui_rect r=pg_ui_home_settings;
    pg_ellipse(c,r.x+r.w/2,r.y+r.h/2,24,24,PANEL);pg_icon(c,PG_ICON_SETTINGS,r.x+r.w/2,r.y+r.h/2,2,MINT);
}
static void home(pg_canvas *c,const pg_game *g){
    bool egg=g->pet.phase==PG_EGG||g->pet.phase==PG_HATCHING;
    bool hud=g->hud_seconds>0&&!g->sleeping&&g->pet.phase!=PG_HATCHING;
    home_objects(c,g);
    if(egg)draw_egg(c,g);
    else{
        float tuck=g->sleeping?pg_clamp(g->sleep_settle/1.4f,0,1):0;
        float climb=g->sleeping&&g->sleep_settle>0&&g->sleep_settle<.7f?sinf(g->sleep_settle/.7f*3.141593f)*7:0;
        pg_ellipse(c,(int)g->x,(int)g->y+4,43,9,0x537356);
        pg_creature(c,g,(int)g->x,(int)g->y+(int)(tuck*12-climb),2);
        if(tuck>0){
            int top=294-(int)(tuck*29),height=302-top;
            uint32_t quilt=g->pet.bed_style==1?0x718da6:g->pet.bed_style==2?0xc59376:0x70988c;
            pg_roundrect(c,146,top,99,height,10,quilt);
            pg_line(c,154,top+4,236,top+4,3,0xe1d5b5);
            for(int i=0;i<7;++i)pg_line(c,155+i*13,top+7,155+i*13,top+10,1,CREAM);
            if(tuck>.6f){pg_star(c,195,top+19,5,GOLD);pg_rect(c,173,top+19,2,2,CREAM);pg_rect(c,219,top+19,2,2,CREAM);}
        }
        if(g->petting>0)pg_icon(c,PG_ICON_HEART,(int)g->x-48,(int)g->y-126,1,CORAL);
        if(g->invitation){pg_roundrect(c,(int)g->x+36,(int)g->y-111,39,30,10,CREAM);pg_icon(c,PG_ICON_GAME,(int)g->x+55,(int)g->y-96,2,INK);}
        if(((g->sleeping&&g->sleep_settle>=2)||(g->goal==PG_REST&&g->goal_time>3&&!g->sleeping&&hypotf(g->x-g->target_x,g->y-g->target_y)<3))&&g->emote!=PG_EMOTE_YAWN)
        {
            for(int i=0;i<3;++i){float t=fmodf(g->anim*.35f+i*.33f,1);int zx=(int)g->x+35+(int)(t*21),zy=(int)g->y-92-(int)(t*34);
                pg_text(c,zx,zy,"Z",i==2?2:1,mix(0x597889,CREAM,1-t));}
        }
        if(!g->sleeping&&g->emote==PG_EMOTE_GREET&&g->emote_time>0){
            int bob=(int)(sinf(g->anim*5)*3);pg_roundrect(c,(int)g->x+38,(int)g->y-126+bob,30,26,9,CREAM);
            pg_icon(c,PG_ICON_HEART,(int)g->x+53,(int)g->y-113+bob,1,CORAL);
        }
        if(g->bowl_time>0){
            static const uint32_t food_colors[]={CORAL,GOLD,0xb5cb86};
            pg_ellipse(c,234,300,20,6,0xe3c4a1);pg_ellipse(c,234,296,20,6,0xaa7559);
            int portions=g->bowl_time>3?4:g->bowl_time>2.3f?2:g->bowl_time>1.8f?1:0;
            for(int i=0;i<portions;++i)pg_ellipse(c,224+i*7,293+(i%2)*2,4,3,food_colors[(unsigned)g->food_flavour%3]);
        }
    }
    for(int i=0;i<8&&g->particles>0;++i){float a=i*0.785f,t=3-g->particles;pg_star(c,233+(int)(cosf(a)*(30+t*30)),235+(int)(sinf(a)*(20+t*20))-(int)(t*16),3,i%2?GOLD:MINT);}
    if(g->top_hud_seconds>0&&!g->sleeping&&g->pet.phase!=PG_HATCHING)home_status(c,g,egg);
    else if(!g->sleeping&&g->pet.phase!=PG_HATCHING)pg_roundrect(c,214,24,38,4,2,MINT);
    if(!g->sleeping&&(hud||g->caption_time>0||g->pet.satiety<25))thought(c,g);
    pg_icon_id dock[]={egg?PG_ICON_WARM:PG_ICON_BOWL,PG_ICON_HEART,egg?PG_ICON_HOURGLASS:PG_ICON_GAME,PG_ICON_BOOK};
    for(int i=0;hud&&i<4;++i){int x=pg_ui_dock[i][0],y=pg_ui_dock[i][1];pg_ellipse(c,x,y+3,30,29,0x202a19);pg_ellipse(c,x,y,29,29,i==1?0x94783e:0x4d643d);pg_icon(c,dock[i],x,y+1,3,i==1?GOLD:MINT);}
    if(!hud&&!g->sleeping&&g->pet.phase!=PG_HATCHING)pg_icon(c,PG_ICON_TAP,233,405,2,0xa2c5b4);
    if(g->sleeping){pg_icon(c,PG_ICON_TAP,119,257,1,GOLD);}
}
static void card(pg_canvas *c,const pg_game *g){
    const pg_pet *p=&g->pet;
    if(p->phase==PG_EGG){
        icon_header(c,PG_ICON_EGG);
        need_meter(c,105,138,256,PG_ICON_WARM,p->warmth,p->warmth<40?GOLD:MINT);
        need_meter(c,105,185,256,PG_ICON_HEART,p->comfort,CORAL);
        unsigned remaining=(unsigned)ceil(fmax(0,fmin(PG_EGG_SECONDS,PG_EGG_SECONDS-p->incubation)));
        char countdown[16];
        if(remaining)snprintf(countdown,sizeof countdown,"%02u:%02u:%02u",remaining/3600,(remaining/60)%60,remaining%60);
        else snprintf(countdown,sizeof countdown,"READY");
        pg_icon(c,PG_ICON_HOURGLASS,121,246,2,0xb9b6e5);
        pg_center(c,254,236,countdown,3,CREAM);
        pg_icon(c,PG_ICON_HOLD,170,290,2,CREAM);pips(c,207,286,3,(float)p->egg_visits,GOLD);
        pg_icon(c,PG_ICON_SHELTER,278,290,2,p->sheltered?MINT:MUTED);if(p->sheltered)pg_icon(c,PG_ICON_CHECK,307,290,1,MINT);
    }else{
        pg_center(c,233,44,p->name,strlen(p->name)>12?2:3,GOLD);pg_center(c,233,68,pg_species_names[p->species],1,MUTED);
        pg_roundrect(c,153,75,160,40,10,PANEL2);pg_center(c,233,88,"RENAME",2,CREAM);
        for(int i=0;i<3;++i)pg_icon(c,PG_ICON_SPROUT,211+i*22,123,1,i<=(int)g->stage?MINT:0x5e6f49);
        need_meter(c,93,133,128,PG_ICON_BOWL,p->satiety,GOLD);need_meter(c,247,133,128,PG_ICON_ENERGY,p->energy,MINT);
        need_meter(c,93,177,128,PG_ICON_CLEAN,p->clean,0x9dc5ce);need_meter(c,247,177,128,PG_ICON_HEART,p->affection,CORAL);
        need_meter(c,93,221,128,PG_ICON_HEALTH,p->health,p->health<30?CORAL:MINT);need_meter(c,247,221,128,PG_ICON_SHIELD,p->trust,0xc6b5db);
        pg_icon_id traits[]={PG_ICON_CURIOUS,PG_ICON_PLAYFUL,PG_ICON_BOLD};float values[]={p->curious,p->playful,p->bold};
        for(int i=0;i<3;++i){int x=148+i*85;pg_icon(c,traits[i],x,282,2,CREAM);pips(c,x-19,304,3,values[i]/34,MINT);}
        count_icon(c,PG_ICON_DAY,162,328,(unsigned)(p->age/PG_DAY)+1,GOLD);
        count_icon(c,PG_ICON_GAME,264,328,p->games_played[0]+p->games_played[1]+p->games_played[2]+p->games_played[3]+p->games_played[4]+p->games_played[5],MINT);
    }
    icon_button(c,119,340,228,37,PG_ICON_STROKE,PG_ICON_HEART,PANEL2);back(c);
}
static void name_editor(pg_canvas *c,const pg_game *g){
    pg_center(c,233,54,"NAME",3,GOLD);
    pg_roundrect(c,83,99,300,55,12,PANEL2);
    char preview[17];snprintf(preview,sizeof preview,"%s%s",g->name_edit,g->name_cursor==(int)strlen(g->name_edit)?"_":"");
    pg_center(c,233,111,preview,3,CREAM);
    size_t n=strlen(g->name_edit);int cursor=g->name_cursor;
    int shown=(int)n+(cursor==(int)n?1:0);
    pg_line(c,233-shown*9+cursor*18,144,247-shown*9+cursor*18,144,3,GOLD);
    pg_line(c,222,178,233,167,4,MINT);pg_line(c,233,167,244,178,4,MINT);
    char letter[2]={cursor<(int)n?g->name_edit[cursor]:' ',0};
    pg_center(c,233,190,letter[0]==' '?"_":letter,8,CREAM);
    pg_line(c,222,268,233,279,4,MINT);pg_line(c,233,279,244,268,4,MINT);
    icon_button(c,93,208,58,58,PG_ICON_BACK,PG_ICON_COUNT,PANEL2);
    icon_button(c,315,208,58,58,PG_ICON_NEXT,PG_ICON_COUNT,PANEL2);
    pg_roundrect(c,169,285,128,40,8,PANEL2);pg_center(c,233,294,"DELETE",3,CREAM);
    pg_roundrect(c,133,335,88,48,10,PANEL2);
    if(g->naming_hatch)pg_center(c,177,349,"KEEP",3,CREAM);
    else pg_icon(c,PG_ICON_BACK,177,359,2,CREAM);
    pg_roundrect(c,245,335,88,48,10,0x526947);pg_center(c,289,349,"SAVE",3,g->name_edit[0]?MINT:MUTED);
}
static void food(pg_canvas *c,const pg_game *g){
    icon_header(c,PG_ICON_BOWL);
    for(int i=0;i<3;++i){
        int x=128+i*106;wood_plaque(c,x-47,160,94,144,PANEL2);
        static const pg_icon_id food_icons[]={PG_ICON_BERRY,PG_ICON_BISCUIT,PG_ICON_PEAR};
        static const uint32_t food_colors[]={CORAL,GOLD,0xb5cb86};
        pg_icon(c,food_icons[i],x,216,3,food_colors[i]);
        pg_icon(c,PG_ICON_TAP,x,269,2,CREAM);
        if(g->pet.meals>5&&pg_favorite_food(g)==i)pg_icon(c,PG_ICON_HEART,x,292,1,CORAL);
    }
    pg_icon(c,PG_ICON_INFINITY,233,342,2,MUTED);back(c);
}
static void care(pg_canvas *c,const pg_game *g){
    bool egg=g->pet.phase==PG_EGG;icon_header(c,PG_ICON_HEART);
    icon_button(c,95,155,276,58,egg?PG_ICON_HOLD:PG_ICON_STROKE,egg?PG_ICON_WARM:PG_ICON_HEART,0x526947);
    icon_button(c,95,222,276,58,egg?PG_ICON_STROKE_EGG:PG_ICON_CLEAN,egg?PG_ICON_EGG:PG_ICON_COUNT,PANEL2);
    if(egg)icon_button(c,95,289,276,58,PG_ICON_SHELTER,PG_ICON_COUNT,0x383e59);
    back(c);
}
static void games_menu(pg_canvas *c,const pg_game *g){
    icon_header(c,PG_ICON_GAME);pips(c,222,110,2,(float)g->games_page+1,MINT);
    static const pg_icon_id icons[]={PG_ICON_TTT,PG_ICON_TILT,PG_ICON_MUSIC,PG_ICON_ECHO,PG_ICON_BALANCE,PG_ICON_WASH};
    static const char *const labels[]={"PAW MATCH","STAR TILT","LITTLE TUNES","ECHOES","LEAF BALANCE","BUBBLE BATH"};
    int page=g->games_page<0?0:g->games_page>1?1:g->games_page;
    for(int i=0;i<3;++i){int which=i+page*3,y=148+i*73;
        wood_plaque(c,95,y,276,64,i==0?0x526947:i==1?0x6a593c:0x64514a);
        pg_icon(c,icons[which],128,y+32,2,MINT);pg_center(c,252,y+25,labels[which],2,CREAM);}
    control(c,pg_ui_games_home,PG_ICON_HOME);if(page>0)control(c,pg_ui_page_previous,PG_ICON_BACK);if(page<1)control(c,pg_ui_page_next,PG_ICON_NEXT);
}
static void ttt(pg_canvas *c,const pg_game *g){
    /* A reserved portrait above the board survives turn changes and results. */
    pg_creature(c,g,233,PG_UI_PAW_FEET,1);
    pg_center(c,141,58,"YOU",2,g->ttt.turn==1?GOLD:MUTED);
    pg_center(c,325,58,"PAL",2,g->ttt.turn==-1?MINT:MUTED);
    pg_icon(c,PG_ICON_HAND,129,97,2,g->ttt.turn==1?GOLD:MUTED);pg_icon(c,PG_ICON_X,164,97,1,GOLD);
    pg_icon(c,PG_ICON_PAW,302,97,2,g->ttt.turn==-1?MINT:MUTED);pg_icon(c,PG_ICON_CIRCLE,337,97,1,MINT);
    for(int i=0;i<9;++i){int x=pg_ui_paw_board.x+(i%3)*PG_UI_PAW_STEP,y=pg_ui_paw_board.y+(i/3)*PG_UI_PAW_STEP;wood_plaque(c,x+3,y+3,64,64,0x586f48);
        if(g->ttt.cells[i]){pg_icon(c,g->ttt.cells[i]==1?PG_ICON_X:PG_ICON_CIRCLE,x+35,y+35,3,g->ttt.cells[i]==1?GOLD:MINT);
            game_sprite(c,FOREST_STAR,((int)(g->anim*2)+i)&1,x+55,y+53,1);}
    }
    if(g->ttt.finished){icon_button(c,135,351,196,32,PG_ICON_REPLAY,g->ttt.winner==1?PG_ICON_TROPHY:g->ttt.winner==-1?PG_ICON_HEART:PG_ICON_REPLAY,0x526947);}
    else{pg_icon(c,PG_ICON_STAR,186,368,1,MINT);pips(c,206,363,5,g->pet.confidence[0]/20,MINT);}
    back(c);
}
static void tilt(pg_canvas *c,const pg_game *g){
    pg_creature(c,g,233,PG_UI_PAW_FEET,1);
    game_count(c,PG_ICON_CLOCK,141,(unsigned)ceilf(g->tilt.seconds),GOLD);game_count(c,PG_ICON_STAR,325,(unsigned)g->tilt.score,MINT);
    pg_ellipse(c,233,244,120,120,0x654b36);pg_ellipse(c,233,244,116,116,0xb18a5c);
    pg_ellipse(c,233,244,111,111,0x394f35);pg_ellipse(c,233,244,105,105,0x4d6640);
    for(int i=0;i<15;++i)pg_rect(c,145+(i*47)%175,161+(i*31)%172,2,2,0x708755);
    if(!g->tilt.finished){
        game_sprite(c,FOREST_ACORN,((int)(g->anim*4))&1,(int)g->tilt.x,(int)g->tilt.y,3);
        game_sprite(c,FOREST_STAR,((int)(g->anim*3))&1,(int)g->tilt.target_x,(int)g->tilt.target_y,3);
    }
    if(g->tilt.finished){pg_center(c,233,166,g->tilt.fell?"FELL OFF!":"TIME UP",2,CREAM);pg_icon(c,!g->tilt.fell&&g->tilt.score>=5?PG_ICON_TROPHY:PG_ICON_HEART,233,244,5,GOLD);icon_button(c,135,351,196,32,PG_ICON_REPLAY,PG_ICON_COUNT,0x526947);}
    else{
        static const int xy[4][2]={{233,142},{97,244},{369,244},{233,346}},dir[4][2]={{0,-1},{-1,0},{1,0},{0,1}};
        for(int i=0;i<4;++i){int x=xy[i][0],y=xy[i][1],dx=dir[i][0],dy=dir[i][1];if(i)pg_roundrect(c,x-24,y-24,48,48,10,PANEL2);pg_line(c,x-dx*9,y-dy*9,x+dx*9,y+dy*9,3,MINT);pg_line(c,x+dx*9,y+dy*9,x+dy*7,y-dx*7,3,MINT);pg_line(c,x+dx*9,y+dy*9,x-dy*7,y+dx*7,3,MINT);}
        if(g->tilt.ready>0){pg_roundrect(c,140,211,186,60,12,PANEL);pg_center(c,233,224,"HOLD STILL",2,CREAM);pg_icon(c,PG_ICON_STILL,233,253,2,GOLD);}
        pg_center(c,233,369,"TAP TOP TO CENTER",2,MUTED);
    }back(c);
}
static void echoes(pg_canvas *c,const pg_game *g){
    const pg_echo *e=&g->echo;char text[32];
    pg_icon(c,PG_ICON_STAR,152,60,2,GOLD);snprintf(text,sizeof text,"%u",(unsigned)e->score);pg_center(c,152,82,text,strlen(text)<=5?3:2,CREAM);
    pg_icon(c,PG_ICON_TROPHY,314,60,2,GOLD);snprintf(text,sizeof text,"%u",(unsigned)g->device.echo_best);pg_center(c,314,82,text,strlen(text)<=5?3:2,CREAM);
    for(int i=0;i<3;++i)pg_icon(c,PG_ICON_HEART,209+i*24,108,1,i<e->attempts?CORAL:PANEL2);
    pg_game pose=*g;pose.facing=e->prompt==PG_SWIPE?-1:1;pose.emote=e->stage==PG_ECHO_CUE?PG_EMOTE_INVITE:e->stage==PG_ECHO_INPUT?PG_EMOTE_CURIOUS:e->stage==PG_ECHO_RETRY?PG_EMOTE_SASS:PG_EMOTE_CHEER;pose.emote_duration=1;pose.emote_time=1-fmodf(g->anim,.8f);
    compact_pet(c,&pose,233,153,false);
    bool go=e->stage==PG_ECHO_INPUT;
    pg_ellipse(c,233,268,78,78,0x75573d);pg_ellipse(c,233,268,74,74,go?0x9cb47a:0xa78357);
    pg_ellipse(c,233,268,69,69,PANEL2);
    game_sprite(c,FOREST_LEAF,((int)(g->anim*2))&1,172,258,2);
    game_sprite(c,FOREST_LEAF,((int)(g->anim*2)+1)&1,294,258,2);
    pg_icon_id icon=e->stage==PG_ECHO_DONE?PG_ICON_TROPHY:e->stage==PG_ECHO_PASS?PG_ICON_CHECK:e->stage==PG_ECHO_RETRY?PG_ICON_REPLAY:e->prompt==PG_SHAKE?PG_ICON_SHAKE:e->prompt==PG_SWIPE?PG_ICON_SWIPE:PG_ICON_TAP;
    pg_icon(c,icon,233,255,5,CREAM);pg_icon(c,go?PG_ICON_PLAY:e->stage==PG_ECHO_RETRY?PG_ICON_HEART:PG_ICON_EYE,233,311,2,go?MINT:GOLD);
    if(go){snprintf(text,sizeof text,"GO! %d",(int)ceilf(e->timer));pg_center(c,233,365,text,2,MINT);pg_roundrect(c,163,352,140,5,2,PANEL2);pg_roundrect(c,163,352,(int)(140*pg_clamp(e->timer/fmaxf(e->response_seconds,1.4f),0,1)),5,2,MINT);}
    else if(e->stage==PG_ECHO_CUE||e->stage==PG_ECHO_DONE)icon_button(c,119,343,228,40,PG_ICON_REPLAY,e->stage==PG_ECHO_DONE?PG_ICON_TROPHY:PG_ICON_EAR,PANEL2);
    back(c);
}
static void rhythm(pg_canvas *c,const pg_game *g){
    pg_icon(c,PG_ICON_MUSIC,139,83,3,MINT);game_count(c,PG_ICON_STAR,325,(unsigned)(g->rhythm.length-1),GOLD);
    pg_game pose=*g;pose.facing=g->rhythm.lit==0?-1:1;pose.emote=g->rhythm.finished?PG_EMOTE_CHEER:g->rhythm.showing&&g->rhythm.lit>=0?PG_EMOTE_INVITE:PG_EMOTE_CURIOUS;pose.emote_duration=1;pose.emote_time=1-fmodf(g->anim,.8f);pg_creature(c,&pose,233,PG_UI_TUNES_FEET,1);
    uint32_t colors[]={0x739e82,0xbd8d72,0x858eb8};pg_icon_id shapes[]={PG_ICON_SPROUT,PG_ICON_STAR,PG_ICON_MOON};
    for(int i=0;i<3;++i){int x=134+i*99;bool lit=g->rhythm.lit==i;wood_plaque(c,x-43,190,86,119,lit?mix(colors[i],CREAM,0.55f):colors[i]);pg_icon(c,shapes[i],x,240,3,lit?CREAM:i==0?MINT:i==1?GOLD:0xbcc7eb);char n[2]={(char)('1'+i),0};pg_center(c,x,282,n,1,CREAM);if(lit)game_sprite(c,FOREST_NOTE,((int)(g->anim*4))&1,x,294,1);}
    if(g->rhythm.finished){icon_button(c,135,351,196,32,PG_ICON_REPLAY,PG_ICON_COUNT,0x526947);}
    else{pg_icon(c,g->rhythm.showing?PG_ICON_EAR:PG_ICON_TAP,207,349,2,MUTED);pg_icon(c,PG_ICON_MUSIC,259,349,2,MINT);}
    back(c);
}
static pg_icon_id event_object(const pg_event *e){
    static const pg_icon_id foods[]={PG_ICON_BERRY,PG_ICON_BISCUIT,PG_ICON_PEAR};
    static const pg_icon_id games[]={PG_ICON_TTT,PG_ICON_TILT,PG_ICON_MUSIC,PG_ICON_ECHO,PG_ICON_BALANCE,PG_ICON_WASH};
    if(e->kind==PG_EV_FED||e->kind==PG_EV_FAVORITE)return foods[e->arg<3?e->arg:0];
    if(e->kind==PG_EV_WON||e->kind==PG_EV_LOST||e->kind==PG_EV_DRAW)return games[e->arg<PG_GAME_COUNT?e->arg:0];
    if(e->kind==PG_EV_SLEPT||e->kind==PG_EV_CLEANED)return PG_ICON_BED;
    if(e->kind==PG_EV_DISCOVERY){
        if(e->arg>=128&&e->arg<128+PG_GAME_COUNT)return games[e->arg-128];
        return e->arg>0&&e->arg<4?games[e->arg-1]:e->arg==4||e->arg==5?PG_ICON_BED:PG_ICON_SPROUT;
    }
    return e->kind==PG_EV_FOUND||e->kind==PG_EV_SUCCESSOR||e->kind==PG_EV_WARMED?PG_ICON_EGG:PG_ICON_HEART;
}
static void album(pg_canvas *c,const pg_game *g){
    icon_header(c,PG_ICON_BOOK);
    if(g->album_expanded){
        const pg_event *e=&g->album_selected;
        pg_icon(c,event_icon(e),202,194,4,MINT);pg_icon(c,event_object(e),278,194,3,GOLD);
        pg_wrapped(c,233,250,pg_event_text(e),22,2,CREAM);
        count_icon(c,PG_ICON_DAY,210,318,(unsigned)(e->at/PG_DAY)+1,GOLD);back(c);return;
    }
    unsigned n=g->pet.event_count<PG_EVENT_CAP?g->pet.event_count:PG_EVENT_CAP,first=(unsigned)g->album_page*5;
    for(unsigned i=0;i<5;++i){unsigned offset=first+i;if(offset>=n)break;const pg_event *e=&g->pet.events[(g->pet.event_count-1-offset)%PG_EVENT_CAP];int y=142+(int)i*37;
        pg_line(c,108,y-18,108,y+18,1,0x5e6f49);pg_rect(c,105,y-3,7,7,MINT);pg_icon(c,event_icon(e),153,y,2,e->kind==PG_EV_HATCHED?GOLD:MINT);
        pg_icon(c,event_object(e),202,y,1,CREAM);
        count_icon(c,PG_ICON_DAY,275,y,(unsigned)(e->at/PG_DAY)+1,MUTED);
    }
    icon_button(c,151,340,164,36,PG_ICON_BOOK,PG_ICON_NEXT,PANEL2);back(c);
}
static void settings_nav(pg_canvas *c,const pg_game *g){
    if(g->utilities.settings_page>0)control(c,pg_ui_page_previous,PG_ICON_BACK);
    if(g->utilities.settings_page<2)control(c,pg_ui_page_next,PG_ICON_NEXT);
    control(c,pg_ui_games_home,PG_ICON_HOME);
}
static void settings(pg_canvas *c,const pg_game *g){
    icon_header(c,PG_ICON_SETTINGS);int page=g->utilities.settings_page;pips(c,214,97,3,(float)page+1,MINT);
    if(page==0){ui_panel(c,pg_ui_settings_rows[0],PANEL2);pg_icon(c,PG_ICON_SUN,132,144,2,GOLD);pips(c,167,140,5,g->pet.brightness<=20?1:g->pet.brightness<=40?2:g->pet.brightness<=65?3:g->pet.brightness<=85?4:5,MINT);char n[12];snprintf(n,sizeof n,"%u",g->pet.brightness);pg_center(c,325,137,n,2,CREAM);ui_button(c,pg_ui_settings_rows[1],g->muted?PG_ICON_MUTE:PG_ICON_SPEAKER,PG_ICON_COUNT,PANEL2);}
    else if(page==1){char n[24];ui_panel(c,pg_ui_settings_rows[0],PANEL2);pg_icon(c,PG_ICON_SPEAKER,137,144,2,MINT);snprintf(n,sizeof n,"VOLUME %u",g->device.volume);pg_center(c,255,136,n,2,CREAM);ui_panel(c,pg_ui_settings_rows[1],PANEL2);pg_center(c,233,202,g->device.auto_sleep?"AUTO SLEEP: ON":"AUTO SLEEP: OFF",2,CREAM);pg_center(c,233,226,"2 MINUTES",1,MUTED);ui_panel(c,pg_ui_settings_rows[2],PANEL2);pg_icon(c,PG_ICON_CLOCK,137,293,2,GOLD);pg_center(c,255,286,"SET TIME",2,CREAM);}
    else{ui_panel(c,pg_ui_settings_rows[0],PANEL2);pg_center(c,233,134,"TIME / BATTERY",2,CREAM);ui_button(c,pg_ui_settings_rows[1],PG_ICON_BOOK,PG_ICON_QUESTION,PANEL2);ui_panel(c,pg_ui_settings_rows[2],PANEL2);pg_center(c,233,280,"DEV TOOLS",2,MUTED);}
    if(g->utilities.storage_fault)pg_icon(c,PG_ICON_ALERT,341,95,1,CORAL);
    settings_nav(c,g);
}
static void volume_page(pg_canvas *c,const pg_game *g){
    icon_header(c,PG_ICON_SPEAKER);char n[20];snprintf(n,sizeof n,"%u%%",g->device.volume);pg_center(c,233,135,n,4,CREAM);
    ui_panel(c,pg_ui_decrease,PANEL2);pg_center(c,133,218,"-",3,MINT);ui_panel(c,pg_ui_increase,PANEL2);pg_center(c,333,218,"+",3,MINT);
    ui_button(c,pg_ui_volume_mute,g->muted?PG_ICON_MUTE:PG_ICON_SPEAKER,PG_ICON_COUNT,PANEL2);pg_center(c,233,351,g->muted?"MUTED":"SOUND ENABLED",1,MUTED);back(c);
}
static void clock_page(pg_canvas *c,const pg_game *g){
    static const char *const labels[]={"YEAR","MONTH","DAY","HOUR","MINUTE"};const pg_calendar *t=&g->utilities.edit;int vals[]={t->year,t->month,t->day,t->hour,t->minute};int field=g->utilities.clock_field%5;char n[20];
    pg_center(c,233,55,"SET TIME",2,CREAM);pg_center(c,233,105,labels[field],2,MINT);snprintf(n,sizeof n,"%02d",vals[field]);pg_center(c,233,144,n,4,CREAM);if(g->utilities.clock_error)pg_center(c,233,182,"TIME UNAVAILABLE",1,CORAL);
    ui_panel(c,pg_ui_decrease,PANEL2);pg_center(c,133,218,"-",3,MINT);ui_panel(c,pg_ui_increase,PANEL2);pg_center(c,333,218,"+",3,MINT);
    control(c,pg_ui_clock_previous,PG_ICON_BACK);control(c,pg_ui_clock_next,PG_ICON_NEXT);
    ui_button(c,pg_ui_cancel,PG_ICON_X,PG_ICON_COUNT,PANEL2);ui_button(c,pg_ui_confirm,PG_ICON_CHECK,PG_ICON_COUNT,PANEL2);back(c);
}
static void device_status(pg_canvas *c,const pg_game *g){
    icon_header(c,PG_ICON_CLOCK);pg_calendar t;char n[40];
    if(pg_calendar_now(g,&t)){snprintf(n,sizeof n,"%04d-%02d-%02d",t.year,t.month,t.day);pg_center(c,233,130,n,2,CREAM);snprintf(n,sizeof n,"%02d:%02d:%02d",t.hour,t.minute,t.second);pg_center(c,233,166,n,3,MINT);}else pg_center(c,233,150,"SET DATE/TIME",2,GOLD);
    const pg_battery_snapshot *b=&g->utilities.battery;battery_glyph(c,g,214,218);
    if(!b->valid)snprintf(n,sizeof n,"BATTERY UNKNOWN");else if(!b->present)snprintf(n,sizeof n,"NO BATTERY");else snprintf(n,sizeof n,"%u%% %s",b->percent,b->charging?"CHARGING":"BATTERY");pg_center(c,233,258,n,2,CREAM);
    if(g->utilities.storage_fault){pg_center(c,233,308,"DEVICE DATA NEEDS",1,CORAL);pg_center(c,233,326,"RECOVERY / UNSAVED",1,CORAL);}else pg_center(c,233,320,"PMIC ESTIMATE",1,MUTED);back(c);
}
static void developer(pg_canvas *c,const pg_game *g){
    pg_center(c,233,60,"DEV TOOLS",3,GOLD);
    pg_center(c,233,102,"TEST BUILD",2,MUTED);
    pg_roundrect(c,103,150,260,66,14,0x526947);pg_icon(c,PG_ICON_EGG,139,183,2,MINT);
    pg_center(c,256,175,"HATCH",3,CREAM);
    pg_roundrect(c,103,238,260,66,14,g->debug_reset_armed?0x784b49:PANEL2);pg_icon(c,PG_ICON_REPLAY,139,271,2,CORAL);
    pg_center(c,256,262,g->debug_reset_armed?"SURE?":"RESET",3,CREAM);
    pg_center(c,233,329,g->debug_reset_armed?"TAP AGAIN TO ERASE":"NEW EGG / ERASE LIFE",2,g->debug_reset_armed?CORAL:MUTED);
    back(c);
}
static void balance(pg_canvas *c,const pg_game *g){
    const pg_balance *b=&g->balance;
    pg_center(c,233,53,"LEAF BALANCE",2,CREAM);
    for(int i=0;i<3;++i)pg_icon(c,PG_ICON_STAR,197+i*36,89,2,b->steady>=(i+1)*4?GOLD:0x3b5a62);
    /* Soft leaf seesaw and an original creature with outstretched tiny paws. */
    float angle=b->angle;int dx=(int)(cosf(angle)*110),dy=(int)(sinf(angle)*75);
    pg_ellipse(c,233,300,111,12,0x263b28);
    pg_ellipse(c,233,267,103,22,0x3c5537);
    pg_line(c,233-dx,278-dy,233+dx,278+dy,18,0x4b693a);
    pg_line(c,233-dx+5,273-dy,233+dx-5,273+dy,8,0xa4bf70);
    pg_line(c,233,286,218,309,6,0x9c7651);pg_line(c,233,286,248,309,6,0x9c7651);
    game_sprite(c,FOREST_LEAF,((int)(g->anim*2))&1,233-dx,276-dy,2);
    game_sprite(c,FOREST_LEAF,((int)(g->anim*2)+1)&1,233+dx,276+dy,2);
    int px=233+(int)(angle*58),py=269+(int)(sinf(angle)*angle*35)-(b->rescue>0?(int)(sinf(b->rescue*7)*9):0);
    pg_game pose=*g;pose.goal=PG_PLAY;pose.facing=angle<0?-1:1;pose.pet.health=pose.pet.satiety=100;
    pg_creature(c,&pose,px,py,2);
    pg_icon(c,PG_ICON_HEART,px-82,py-55,1,b->rescue>0?CORAL:MINT);
    if(!b->started){pg_center(c,233,316,"TIP GENTLY",2,MINT);}
    else if(b->ready>0)pg_center(c,233,316,"READY...",3,GOLD);
    else if(!b->finished){char n[12];snprintf(n,sizeof n,"%u",(unsigned)ceilf(b->seconds));pg_center(c,233,317,n,3,CREAM);}
    if(!b->started||b->finished){
        pg_roundrect(c,123,344,220,40,12,0x526947);
        pg_icon(c,b->finished?PG_ICON_REPLAY:PG_ICON_PLAY,211,364,2,CREAM);
        pg_icon(c,b->finished?(b->steady>=12?PG_ICON_TROPHY:PG_ICON_HEART):PG_ICON_TILT_GESTURE,259,364,2,GOLD);
    }else {pg_icon(c,PG_ICON_BACK,75,230,2,MINT);pg_icon(c,PG_ICON_NEXT,391,230,2,MINT);}
    back(c);
}
static void wash(pg_canvas *c,const pg_game *g){
    const pg_wash *w=&g->wash;
    pg_center(c,233,53,"BUBBLE BATH",2,CREAM);
    pg_roundrect(c,158,96,150,12,4,PANEL2);
    if(w->total)pg_roundrect(c,158,96,(int)(150.0f*(w->total-w->left)/w->total),12,4,MINT);
    pg_ellipse(c,233,264,137,99,0x654b36);pg_ellipse(c,233,262,132,95,0xb18a5c);
    pg_ellipse(c,233,262,124,87,0x436f63);pg_ellipse(c,233,260,115,79,0x6d9e87);
    pg_game pose=*g;pose.goal=PG_ATTENTION;pose.pet.health=pose.pet.satiety=100;pose.emote=g->emote==PG_EMOTE_CHEER?PG_EMOTE_CHEER:PG_EMOTE_CLEAN;pose.emote_duration=2;pose.emote_time=2-fmodf(g->anim,1.5f);
    pg_creature(c,&pose,233,285,2);
    for(int i=0;i<PG_FOAM_COUNT;++i)if(w->foam[i]){
        int x=112+i%PG_FOAM_SIDE*22,y=127+i/PG_FOAM_SIDE*22;
        if(w->splash>0){float t=1-w->splash/0.65f;x=233+(int)((x-233)*t);y=248+(int)((y-248)*t);}
        game_sprite(c,FOREST_BUBBLE,((int)(g->anim*2)+i)&1,x,y,3);
    }
    if(w->finished){pg_icon(c,PG_ICON_SPARKLE,160,169,3,GOLD);pg_icon(c,PG_ICON_SPARKLE,309,183,2,CREAM);
        icon_button(c,123,344,220,40,PG_ICON_REPLAY,PG_ICON_HEART,0x526947);
    }else{pg_icon(c,PG_ICON_SWIPE,233,352,3,CREAM);}
    back(c);
}
static void help(pg_canvas *c,const pg_game *g){
    /* Text is deliberately opt-in here, for learning ambiguous symbols. */
    static const pg_icon_id icons[4][12]={
        {PG_ICON_BOWL,PG_ICON_HEART,PG_ICON_CLEAN,PG_ICON_ENERGY,PG_ICON_HEALTH,PG_ICON_SHIELD,PG_ICON_WARM,PG_ICON_HOURGLASS,PG_ICON_SPROUT,PG_ICON_DAY,PG_ICON_CURIOUS,PG_ICON_BOLD},
        {PG_ICON_TTT,PG_ICON_TILT,PG_ICON_MUSIC,PG_ICON_ECHO,PG_ICON_BALANCE,PG_ICON_WASH,PG_ICON_SHAKE,PG_ICON_SWIPE,PG_ICON_TAP,PG_ICON_TILT_GESTURE,PG_ICON_STILL,PG_ICON_TROPHY},
        {PG_ICON_STROKE,PG_ICON_HOLD,PG_ICON_STROKE_EGG,PG_ICON_SHELTER,PG_ICON_BOOK,PG_ICON_SETTINGS,PG_ICON_SPEAKER,PG_ICON_MUTE,PG_ICON_BACK,PG_ICON_NEXT,PG_ICON_HOME,PG_ICON_INFINITY},
        {PG_ICON_COLD,PG_ICON_HOT,PG_ICON_ALERT,PG_ICON_EGG,PG_ICON_SPARKLE,PG_ICON_QUESTION,PG_ICON_BERRY,PG_ICON_BISCUIT,PG_ICON_PEAR,PG_ICON_PORTRAIT,PG_ICON_STAR,PG_ICON_EAR}};
    static const char *const words[4][12]={
        {"FOOD","AFFECTION","CLEAN","ENERGY","HEALTH","TRUST","WARMTH","GROWING","GROWTH","DAY","CURIOUS","COURAGE"},
        {"TIC TAC TOE","TILT STARS","RHYTHM","ECHOES","BALANCE","BUBBLE BATH","SHAKE","SWIPE","TAP","TILT","HOLD STILL","VICTORY"},
        {"PET","HOLD EGG","STROKE EGG","SHELTER","MEMORIES","SETTINGS","SOUND ON","MUTED","BACK","NEXT","HOME","FREE FOOD"},
        {"TOO COLD","TOO WARM","NEEDS CARE","MYSTERY EGG","DISCOVERY","HELP","BERRY","BISCUIT","PEAR","COMPANION","CONFIDENCE","LISTEN"}};
    int page=(g->icon_help_page/2)%4,half=g->icon_help_page%2;pg_center(c,233,54,"ICON GUIDE",2,CREAM);pips(c,173,84,8,(float)g->icon_help_page+1,MINT);
    for(int row=0;row<6;++row){int i=half*6+row,y=126+row*36;pg_icon(c,icons[page][i],132,y+8,2,MINT);pg_text(c,164,y+2,words[page][i],2,CREAM);}
    icon_button(c,151,344,164,36,PG_ICON_BOOK,PG_ICON_NEXT,PANEL2);back(c);
}
static void memorial(pg_canvas *c,const pg_game *g){
    pg_icon(c,PG_ICON_HEART,233,57,2,CORAL);pg_center(c,233,83,g->pet.name,2,CREAM);
    pg_ellipse(c,233,280,70,15,0x294d49);pg_roundrect(c,201,186,64,86,28,0x668777);pg_heart(c,233,217,15,0xc9c6a0);
    pg_line(c,280,280,281,235,3,0x769f76);leaf(c,281,261,1,0x9abb88);pg_ellipse(c,281,233,10,9,CORAL);pg_ellipse(c,281,233,4,4,GOLD);
    pg_center(c,233,133,"A SMALL LIFE REMEMBERED",1,MUTED);
    if(g->memorial_reset_armed){
        pg_center(c,233,298,"OLD PET AND PROGRESS",1,CORAL);pg_center(c,233,316,"WILL BE ERASED. SETTINGS STAY.",1,CREAM);
        pg_roundrect(c,110,347,246,45,12,PANEL2);pg_icon(c,PG_ICON_EGG,141,370,2,MINT);
        pg_center(c,254,362,"TAP AGAIN",2,CREAM);
    }else{pg_roundrect(c,110,347,246,45,12,PANEL2);pg_icon(c,PG_ICON_EGG,141,370,2,MINT);pg_center(c,254,362,"NEW EGG",2,CREAM);}

}

/* Separate interface composition permits an unclipped geometry audit. */
void pg_render_interface(pg_canvas *c,const pg_game *g) {
    if(!c||!c->pixels||c->width!=466||c->height!=466||!g)return;
    switch(g->screen){
    case PG_HOME:home(c,g);break;case PG_CARD:card(c,g);break;case PG_FOOD:food(c,g);break;
    case PG_CARE:care(c,g);break;case PG_GAMES:games_menu(c,g);break;case PG_TTT:ttt(c,g);break;
    case PG_TILT:tilt(c,g);break;case PG_RHYTHM:rhythm(c,g);break;case PG_ALBUM:album(c,g);break;
    case PG_ECHO:echoes(c,g);break;case PG_VOLUME:volume_page(c,g);break;case PG_CLOCK_EDIT:clock_page(c,g);break;case PG_DEVICE_STATUS:device_status(c,g);break;
    case PG_BALANCE:balance(c,g);break;case PG_WASH:wash(c,g);break;case PG_DEBUG:developer(c,g);break;
    case PG_SETTINGS:settings(c,g);break;case PG_HELP:help(c,g);break;case PG_MEMORIAL:memorial(c,g);break;
    case PG_NAME_EDIT:name_editor(c,g);break;
    case PG_SCREEN_COUNT:break;
    }
    if(g->device.auto_sleep&&g->utilities.idle>=115&&!g->standby){char n[20];snprintf(n,sizeof n,"SLEEP IN %u",(unsigned)fmax(1,ceil(120-g->utilities.idle)));pg_roundrect(c,153,29,160,30,10,PANEL);pg_center(c,233,38,n,1,GOLD);}
}
void pg_render(pg_canvas *c,const pg_game *g) {
    if(!c||!c->pixels||c->width!=466||c->height!=466||!g)return;
    if(g->standby){pg_rect(c,0,0,466,466,0);return;}
    scene_background(c,g);
    if(g->screen!=PG_HOME){
        /* One write per pixel keeps the circular panel inexpensive in PSRAM. */
        for(int dy=-219;dy<=219;++dy){
            int outer=(int)(219*sqrtf(1.0f-(float)(dy*dy)/(219*219)));
            if(dy<-217||dy>217){pg_rect(c,233-outer,233+dy,outer*2+1,1,0x5e6f49);continue;}
            int inner=(int)(217*sqrtf(1.0f-(float)(dy*dy)/(217*217)));
            pg_rect(c,233-outer,233+dy,outer-inner,1,0x8a794e);
            /* Shared low-contrast woodland texture gives all interface pages
               a consistent material without new per-screen image buffers. */
            int y=233+dy;
            const uint16_t *row=pg_forest_ui+(y*160/466)*160;
            for(int x=233-inner;x<=233+inner;++x)
                c->pixels[y*466+x]=row[x*160/466];
            pg_rect(c,234+inner,233+dy,outer-inner,1,0x8a794e);
        }
    }
    if(g->screen!=PG_HOME){
        int leaf_frame=((int)(g->anim*1.2f))&1;
        menu_leaf(c,104,130,leaf_frame);menu_leaf(c,352,130,leaf_frame^1);
    }
    pg_render_interface(c,g);
    /* The simulator and device share the exact round clipping. */
    for(int y=0;y<466;++y){
        int dy=y-233,dx=(int)sqrtf(233.0f*233-dy*dy);
        pg_rect(c,0,y,233-dx,1,0x07151c);pg_rect(c,234+dx,y,466,1,0x07151c);
    }
}

void pg_render_tutorial_preview(pg_canvas *c,const pg_game *g,const pg_tutorial *tutorial){
    if(!c||!c->pixels||c->width!=466||c->height!=466||!g||!tutorial)return;
    if(tutorial->committed==PG_TUTORIAL_LANDED){pg_render(c,g);return;}
    if(g->standby){pg_rect(c,0,0,466,466,0);return;}
    scene_background(c,g);
    home_objects(c,g);
    if(tutorial->committed==PG_TUTORIAL_SWIPE){
        /* A downward-moving hand rehearses the actual upper-home gesture. */
        int y=85+(int)(fmodf(g->anim,1.5f)*30);
        pg_roundrect(c,214,24,38,4,2,MINT);
        pg_line(c,233,66,233,137,3,GOLD);
        pg_line(c,233,137,222,126,3,GOLD);pg_line(c,233,137,244,126,3,GOLD);
        pg_icon(c,PG_ICON_HAND,264,y,3,CREAM);
    }else if(tutorial->committed==PG_TUTORIAL_LAMP_NIGHT||tutorial->committed==PG_TUTORIAL_LAMP_DAY){
        if(tutorial->committed==PG_TUTORIAL_LAMP_NIGHT&&g->top_hud_seconds>0){
            pg_roundrect(c,135,32,196,36,18,PANEL);
            pg_icon(c,PG_ICON_SPARKLE,233,48,2,MINT);
            pg_roundrect(c,135,70,196,45,18,PANEL);
            pg_icon(c,PG_ICON_CLOCK,184,92,2,MINT);battery_scaled(c,g,245,76,2);
            pg_ui_rect r=pg_ui_home_settings;
            pg_ellipse(c,r.x+r.w/2,r.y+r.h/2,24,24,PANEL);pg_icon(c,PG_ICON_SETTINGS,r.x+r.w/2,r.y+r.h/2,2,MINT);
        }
        int bob=(int)(sinf(g->anim*3)*4);
        pg_icon(c,PG_ICON_TAP,181,219+bob,3,CREAM);
        pg_star(c,156,195,5,GOLD);
    }else if(tutorial->committed==PG_TUTORIAL_DELIVERY){
        float f=pg_clamp(tutorial->descent_seconds/PG_TUTORIAL_DESCENT_SECONDS,0,1);
        int y=118+(int)(f*130);
        int frame=((int)(g->anim*5.0f))&7;
        forest_sprite(c,pg_forest_parachute,112,160,frame,177,y-104);
    }
    for(int y=0;y<466;++y){
        int dy=y-233,dx=(int)sqrtf(233.0f*233-dy*dy);
        pg_rect(c,0,y,233-dx,1,0x07151c);pg_rect(c,234+dx,y,466,1,0x07151c);
    }
}
