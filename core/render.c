#include "draw.h"
#include "icons.h"
#include "layout.h"
#include <math.h>
#include <stdio.h>
#include <string.h>

#define INK 0x142b35
#define CREAM 0xf8edcf
#define GOLD 0xf4c979
#define MINT 0x91d5b2
#define MUTED 0x90aaa8
#define PANEL 0x142d39
#define PANEL2 0x1c3b46
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
    float night=g->sleeping?1:0;
    uint32_t top=mix(0x397d80,0x101b35,night),bottom=mix(0xa3c698,0x31495c,night);
    for(int y=0;y<466;++y)pg_rect(c,0,y,466,1,mix(top,bottom,(float)y/466));
    /* The round screen is a window through the trees into a tiny living grove. */
    if(night){
        for(int i=0;i<30;++i){int x=56+(i*137)%355,y=50+(i*53)%180;
            float tw=.5f+.5f*sinf(g->anim*.8f+i*2.3f);
            pg_star(c,x,y,i%5==0?3:1,mix(0x46647c,0xe5e6b8,tw));}
        pg_ellipse(c,329,123,25,25,0x49546b);pg_ellipse(c,329,123,20,20,0xf4e2b0);
        pg_ellipse(c,338,116,18,18,mix(top,bottom,.24f));
    }else{
        pg_ellipse(c,328,123,32,32,0xb2ca99);pg_ellipse(c,328,123,23,23,0xffe1a0);
        for(int i=0;i<3;++i){int x=65+(int)fmodf(g->anim*(2+i)+i*137,380),y=116+i*29;
            pg_ellipse(c,x,y,28,7,0xc4dac1);pg_ellipse(c,x-9,y-5,15,9,0xc4dac1);}
    }
    pg_ellipse(c,78,245,125,64,mix(0x4e8d79,0x263e54,night));
    pg_ellipse(c,378,239,150,62,mix(0x609b80,0x2d4b59,night));
    pg_ellipse(c,230,269,189,52,mix(0x79aa83,0x3b5b60,night));
    /* Framing trunks, swaying canopy, little bark marks. */
    for(int side=0;side<2;++side){int x=side?411:48,sgn=side?-1:1;
        pg_line(c,x,271,x-sgn*10,109,16,mix(0x335c53,0x1b3343,night));
        pg_line(c,x,167,x+sgn*45,110,8,mix(0x335c53,0x1b3343,night));
        for(int j=0;j<4;++j){int sway=(int)(sinf(g->anim*.65f+j)*3);
            pg_ellipse(c,x+sgn*(j*21-12)+sway,72+j%2*22,48-j*3,40,mix(j%2?0x377b65:0x286253,0x183849,night));}
    }
    /* A small stream below the island catches the moving sky. */
    pg_ellipse(c,252,354,136,26,mix(0x5c9d99,0x29465e,night));
    for(int i=0;i<6;++i){int x=174+i*23+(int)(sinf(g->anim+i)*5);
        pg_line(c,x,344+i%3*9,x+15,344+i%3*9,1,mix(0xb1d6bb,0x66849b,night));}
    pg_ellipse(c,233,322,162,37,0x142d36);
    pg_ellipse(c,233,311,156,31,0x1c4244);
    pg_ellipse(c,233,304,156,28,0x4b775a);
    pg_ellipse(c,233,298,149,24,0x72936a);
    pg_ellipse(c,233,291,145,18,0x87a572);
    for(int i=0;i<50;++i) {
        int x=95+(i*47)%275, y=289+(i*13)%29;
        if((x-233)*(x-233)/220+((y-299)*(y-299))>370)continue;
        pg_rect(c,x,y,3+(i%3),2,i%3?0x99b57a:0xb8bf82);
    }
    for(int i=0;i<9;++i) {
        int x=113+i*29,y=318+(i*17)%11;
        pg_rect(c,x,y,3,3,0x6c8f65);
        if(i%3==0)pg_line(c,x,y,x+2,y+10,2,0x456e59);
    }
    /* Artwork shares pg_ui_lamp's generous, always available touch target. */
    if(!night){pg_ellipse(c,119,225,27,31,0x7f9e67);pg_ellipse(c,119,225,20,25,0xa8b575);}
    pg_line(c,99,278,99,195,5,0x294b48);pg_line(c,99,196,119,196,4,0x294b48);
    pg_line(c,119,196,119,208,2,GOLD);
    pg_roundrect(c,105,207,29,34,5,INK);pg_roundrect(c,109,211,21,26,3,night?0x486275:0xf2d58a);
    pg_rect(c,117,213,4,21,night?0x718b94:0xfff2c9);
    pg_line(c,105,208,134,208,3,GOLD);pg_line(c,107,240,132,240,3,GOLD);
    pg_ellipse(c,104,283,19,5,0x34584c);
    /* Fireflies at night, drifting pollen and butterflies by day. */
    for(int i=0;i<7;++i){int x=94+i*43+(int)(sinf(g->anim*.6f+i)*10),y=178+i%3*29+(int)(sinf(g->anim+i*2)*9);
        if(night){float tw=.5f+.5f*sinf(g->anim*2+i);pg_ellipse(c,x,y,3,3,mix(0x344e58,0x9aa976,tw));pg_rect(c,x,y,2,2,mix(0x5b7578,0xffe4a2,tw));}
        else if(i%3==0){int wing=2+(int)(fabsf(sinf(g->anim*6+i))*3);pg_ellipse(c,x-wing,y-1,wing,3,0xf7d397);pg_ellipse(c,x+wing,y-1,wing,3,0xf2b698);pg_line(c,x,y-3,x,y+3,1,INK);}
        else pg_rect(c,x,y,2,2,0xd7dfaa);
    }
}
static void home_objects(pg_canvas *c,const pg_game *g) {
    uint32_t bed=g->pet.bed_style==1?0x7993a0:g->pet.bed_style==2?0xc49c75:0x9c8f83;
    pg_ellipse(c,195,291,34,10,0x365548);
    pg_roundrect(c,166,277,58,17,7,bed);
    pg_roundrect(c,171,276,47,12,5,0xd2c6a5);
    pg_line(c,178,281,202,281,2,0xe8dcc0);
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
static void draw_egg(pg_canvas *c,const pg_game *g) {
    int shake=g->pet.phase==PG_HATCHING?(int)(sinf(g->hatch_time*24)*g->hatch_time):0;
    int x=233+shake,y=252+(int)(sinf(g->anim*1.8f)*1.5f);
    pg_ellipse(c,233,301,58,12,0x4b6050);
    for(int i=0;i<7;++i)pg_ellipse(c,233,299-i,55-i*2,8, i%2?0xb69e72:0xc8b685);
    uint32_t shell=g->pet.warmth<40?0xb5c6c4:CREAM;
    pg_ellipse(c,x,y+2,40,50,0x506457);
    pg_ellipse(c,x,y,38,49,shell);
    pg_ellipse(c,x-2,y-20,32,35,shell);
    pg_ellipse(c,x+7,y+3,29,41,mix(shell,0xbcbb9a,0.33f));
    pg_ellipse(c,x-9,y-4,28,41,shell);
    pg_ellipse(c,x-17,y-23,8,15,0xfff7de);
    pg_ellipse(c,x-15,y+12,5,7,0xb5be94);
    pg_ellipse(c,x+11,y-13,4,5,0xd5b997);
    pg_ellipse(c,x+15,y+24,6,4,0xaebc92);
    pg_ellipse(c,x-5,y+29,3,4,0xd5b997);
    pg_ellipse(c,x+4,y-36,3,4,0xb5be94);
    if(g->pet.phase==PG_HATCHING || g->pet.incubation>PG_EGG_SECONDS*0.8) {
        pg_line(c,x-13,y-14,x-3,y-4,2,0x8e9276);pg_line(c,x-3,y-4,x-10,y+4,2,0x8e9276);
        pg_line(c,x-10,y+4,x+2,y+13,2,0x8e9276);
    }
    if(g->pet.phase==PG_HATCHING && g->hatch_time>4) {
        pg_star(c,x-53,y-24,10,GOLD);pg_star(c,x+49,y-7,8,CREAM);
        pg_star(c,x+9,y-68,7,MINT);
    }
    if(g->petting>0) {
        pg_heart(c,200,y-57,7,CORAL);pg_star(c,275,y-40,5,GOLD);
    }
    if(g->pet.warmth<40) {
        pg_line(c,185,246,177,252,2,0x9fc4c9);pg_line(c,279,252,287,246,2,0x9fc4c9);
    }
}

/* Hand-authored, code-native sprite art. The same pixels run on desktop and MCU. */
void pg_creature(pg_canvas *c,const pg_game *g,int x,int feet_y,int size) {
    uint16_t pixels[72*72];
    pg_canvas s={pixels,72,72};
    const uint16_t key=pg_rgb(0xff00ff);
    for(int i=0;i<72*72;++i)pixels[i]=key;
    pg_emote e=g->emote_time>0?g->emote:PG_EMOTE_NONE;
    bool moving=hypotf(g->x-g->target_x,g->y-g->target_y)>3;
    bool resting=g->sleeping||(g->goal==PG_REST&&g->goal_time>3&&!moving);
    bool sleeping=resting&&e!=PG_EMOTE_YAWN;
    bool worried=g->pet.health<40||g->pet.satiety<20;
    float age=g->emote_duration-g->emote_time;
    float pulse=sinf(age*12), settle=fminf(1,g->emote_time*4);
    float tempo=g->pet.species==PG_MOSSLET?2.4f:g->pet.species==PG_EMBERKIN?4.6f:1.9f;
    int bounce=(int)(sinf(g->anim*tempo)*(g->pet.species==PG_LUNAPIP?2.5f:1));
    int lift=0, sway=0;float lean=0, squash=1;
    int ear=(int)(sinf(g->anim*(e==PG_EMOTE_NONE?3:8))*2);
    if(resting){bounce=0;ear=0;squash=1+sinf(g->anim*1.8f)*0.018f;}
    if(e==PG_EMOTE_CHEER){
        float hop=fmaxf(0,sinf(age*(g->pet.species==PG_EMBERKIN?13:9)));
        lift=(int)(hop*(g->pet.species==PG_EMBERKIN?9:5)*settle);
        squash=1+(hop<0.1f?-0.07f:0.035f)*settle;
        sway=(int)(sinf(age*9)*2*settle);ear=(int)(pulse*3);
    }else if(e==PG_EMOTE_CUDDLE){lean=0.065f*sinf(age*3)*settle;squash=0.96f;}
    else if(e==PG_EMOTE_GREET||e==PG_EMOTE_INVITE){lean=0.06f*sinf(age*5)*settle;ear=(int)(pulse*3);}
    else if(e==PG_EMOTE_CURIOUS||g->goal==PG_INSPECT){lean=0.10f*sinf(g->anim*2);ear=3;}
    else if(e==PG_EMOTE_OOPS){lean=0.065f*sinf(age*8)*settle;ear=4;}
    else if(e==PG_EMOTE_NIBBLE){bounce=(int)(sinf(age*16)*1.5f);squash=1+0.025f*sinf(age*16);}
    else if(e==PG_EMOTE_CLEAN){sway=(int)(pulse*2*settle);ear=(int)(pulse*3);}
    if(moving&&!resting){lean+=sinf(g->anim*9)*.055f;bounce+=(int)(fabsf(sinf(g->anim*9))*2);}
    if(sleeping){lean=-.12f;squash=.88f+sinf(g->anim*1.8f)*.035f;}
    if(worried&&e==PG_EMOTE_NONE){ear=4;lean=-0.025f;}
    int b=g->stage==PG_ADULT?2:g->stage==PG_YOUNG?1:0;
    int foot=moving?(int)(sinf(g->anim*9)*2):0;
    int face_y=39+bounce, eye_dx=moving?(int)g->facing:0;
    if(e==PG_EMOTE_CURIOUS||e==PG_EMOTE_INVITE)eye_dx=(g->performance_variant&1)?2:-2;
    if(g->pet.species==PG_MOSSLET) {
        /* Leaf ears, seed-shaped body, stubby paws, little leafy collar. */
        pg_ellipse(&s,23-ear,22+bounce+ear,7,16,0x244e49);pg_ellipse(&s,49+ear,19+bounce-ear,7,18,0x244e49);
        pg_ellipse(&s,23-ear,20+bounce+ear,5,13,0x80cc96);pg_ellipse(&s,49+ear,17+bounce-ear,5,14,0xb0dd9e);
        pg_line(&s,24-ear,28+bounce+ear,21-ear,11+bounce+ear,1,0x559c76);
        pg_line(&s,48+ear,27+bounce-ear,51+ear,9+bounce-ear,1,0x70af7d);
        pg_ellipse(&s,36,45+bounce,23+b,21,0x244e49);
        pg_ellipse(&s,36,44+bounce,21+b,20,0x72c4a0);
        pg_ellipse(&s,32,42+bounce,18+b,18,0x9addb2);
        pg_ellipse(&s,35,51+bounce,14,11,0xd8eab9);
        pg_ellipse(&s,20,49+bounce,5,7,0x67b291);pg_ellipse(&s,53,48+bounce,5,7,0x67b291);
        pg_ellipse(&s,27,63+foot,7,3,0x355b4c);pg_ellipse(&s,46,63-foot,7,3,0x355b4c);
        pg_ellipse(&s,27,61+foot,6,3,0x8dd2a2);pg_ellipse(&s,46,61-foot,6,3,0x8dd2a2);
        pg_line(&s,27,26+bounce,35,29+bounce,3,0x6ca66a);pg_line(&s,35,29+bounce,43,25+bounce,3,0x6ca66a);
        if(b>0) {pg_ellipse(&s,36,27+bounce,4,3,GOLD);pg_pixel(&s,36,26+bounce,CREAM);}
    } else if(g->pet.species==PG_EMBERKIN) {
        /* Asymmetric flame tail, tall pointed ears, cream muzzle and chest. */
        pg_ellipse(&s,58+ear,49+bounce,10,13,0x713f48);pg_ellipse(&s,58+ear,47+bounce,8,12,0xdd775c);
        pg_ellipse(&s,61+ear,40+bounce-ear,5,8,GOLD);
        pg_line(&s,20,31+bounce,17-ear,9+bounce+ear,13,0x713f48);
        pg_line(&s,49,29+bounce,54+ear,7+bounce-ear,13,0x713f48);
        pg_line(&s,20,27+bounce,18-ear,12+bounce+ear,8,0xf0a469);
        pg_line(&s,49,25+bounce,53+ear,10+bounce-ear,8,0xf0a469);
        pg_line(&s,20,25+bounce,18-ear,16+bounce+ear,3,0xd78170);
        pg_line(&s,49,23+bounce,52+ear,14+bounce-ear,3,0xd78170);
        pg_ellipse(&s,35,47+bounce,20+b,19,0x713f48);
        pg_ellipse(&s,35,46+bounce,18+b,18,0xe98e65);
        pg_ellipse(&s,33,34+bounce,22+b,17,0x713f48);
        pg_ellipse(&s,33,33+bounce,20+b,16,0xf5b17b);
        pg_ellipse(&s,33,44+bounce,14,9,0xffdfad);
        pg_ellipse(&s,35,54+bounce,10,9,0xffdfad);
        pg_ellipse(&s,24,64+foot,7,3,0x713f48);pg_ellipse(&s,45,64-foot,7,3,0x713f48);
        pg_ellipse(&s,24,62+foot,6,3,0xf5b17b);pg_ellipse(&s,45,62-foot,6,3,0xf5b17b);
        face_y=34+bounce;
        pg_rect(&s,31,23+bounce,5,4,0xffd09a);
        if(b>0){pg_rect(&s,23,51+bounce,3,5,GOLD);pg_rect(&s,46,50+bounce,3,5,GOLD);}
    } else {
        /* Cloud fins, rounded raindrop body, star freckles and ribbon tail. */
        pg_line(&s,53,55+bounce,64,46+bounce+ear,6,0x575e99);
        pg_ellipse(&s,64,44+bounce+ear,5,7,0xb7b6e6);
        for(int i=0;i<3;++i){
            pg_ellipse(&s,17-i*2,30+i*5+bounce+ear,7,5,0x555d94);
            pg_ellipse(&s,16-i*2,29+i*5+bounce+ear,6,4,0xa6bce0);
            pg_ellipse(&s,55+i*2,30+i*5+bounce+ear,7,5,0x555d94);
            pg_ellipse(&s,56+i*2,29+i*5+bounce+ear,6,4,0xa6bce0);
        }
        pg_ellipse(&s,36,43+bounce,22+b,23,0x424d80);
        pg_ellipse(&s,36,42+bounce,20+b,22,0x929ed3);
        pg_ellipse(&s,31,39+bounce,16+b,19,0xc1c7ec);
        pg_ellipse(&s,35,52+bounce,13,12,0xdce2ee);
        pg_ellipse(&s,26,64+foot,7,3,0x424d80);pg_ellipse(&s,47,64-foot,7,3,0x424d80);
        pg_ellipse(&s,26,62+foot,6,3,0xafbae2);pg_ellipse(&s,47,62-foot,6,3,0xafbae2);
        pg_ellipse(&s,36,21+bounce,3,8,0x9cadd9);
        pg_ellipse(&s,38,14+bounce,5,5,0xdce2ee);
        pg_star(&s,37,27+bounce,3,GOLD);
        if(b>0){pg_rect(&s,19,45+bounce,2,2,CREAM);pg_rect(&s,52,46+bounce,2,2,CREAM);}
    }
    /* Common face grammar, different head placement and species silhouette. */
    int left=27+eye_dx,right=44+eye_dx;
    uint32_t eye=0x273740;
    if(e==PG_EMOTE_CHEER||e==PG_EMOTE_CUDDLE) {
        pg_line(&s,left-3,face_y+1,left,face_y-2,2,eye);pg_line(&s,left,face_y-2,left+3,face_y+1,2,eye);
        pg_line(&s,right-3,face_y+1,right,face_y-2,2,eye);pg_line(&s,right,face_y-2,right+3,face_y+1,2,eye);
    }else if(sleeping || g->blink<0 || (e==PG_EMOTE_NONE&&g->petting>0.5f)) {
        pg_line(&s,left-2,face_y,left+2,face_y+1,2,eye);
        pg_line(&s,right-2,face_y+1,right+2,face_y,2,eye);
    } else {
        pg_roundrect(&s,left-2,face_y-3,4,6,1,eye);
        pg_roundrect(&s,right-2,face_y-3,4,e==PG_EMOTE_INVITE||e==PG_EMOTE_CURIOUS?8:6,1,eye);
        pg_pixel(&s,left-1,face_y-2,CREAM);pg_pixel(&s,right-1,face_y-2,CREAM);
    }
    pg_ellipse(&s,22,face_y+5,4,2,0xd59d9a);pg_ellipse(&s,49,face_y+5,4,2,0xd59d9a);
    if(e==PG_EMOTE_YAWN||e==PG_EMOTE_INVITE){
        pg_ellipse(&s,35,face_y+8,3,4,eye);pg_ellipse(&s,35,face_y+9,1,2,0xd59d9a);
    }else if(e==PG_EMOTE_CHEER||e==PG_EMOTE_GREET){
        pg_ellipse(&s,35,face_y+8,4,3,eye);pg_line(&s,33,face_y+10,37,face_y+10,1,CORAL);
    }else if(e==PG_EMOTE_OOPS||worried){
        pg_line(&s,32,face_y+10,35,face_y+8,1,eye);pg_line(&s,35,face_y+8,38,face_y+10,1,eye);
        pg_line(&s,left-3,face_y-6,left+2,face_y-8,1,eye);
        pg_line(&s,right-2,face_y-8,right+3,face_y-6,1,eye);
    }else {pg_pixel(&s,33,face_y+7,eye);pg_pixel(&s,35,face_y+8,eye);pg_pixel(&s,37,face_y+7,eye);}
    if(e==PG_EMOTE_NIBBLE&&sinf(age*16)>0)pg_ellipse(&s,35,face_y+8,3,2,0x9f655b);
    uint32_t paw=g->pet.species==PG_MOSSLET?0x8dd2a2:g->pet.species==PG_EMBERKIN?0xf5b17b:0xafbae2;
    if(e==PG_EMOTE_GREET||e==PG_EMOTE_INVITE||e==PG_EMOTE_CHEER){
        int wave=(int)(pulse*3);pg_ellipse(&s,57,42+wave+bounce,5,6,paw);
        if(e==PG_EMOTE_CHEER)pg_ellipse(&s,15,41-wave+bounce,5,6,paw);
    }else if(e==PG_EMOTE_CUDDLE||e==PG_EMOTE_NIBBLE){
        pg_ellipse(&s,28,53+bounce,5,4,paw);pg_ellipse(&s,43,53+bounce,5,4,paw);
    }
    if(e==PG_EMOTE_CLEAN){pg_star(&s,9,18+(int)(age*5)%9,3,GOLD);pg_star(&s,62,16,2,CREAM);}
    if(e==PG_EMOTE_CUDDLE)pg_heart(&s,57,14,4,CORAL);
    int origin_x=x-36*size+sway*size, origin_y=feet_y-66*size-lift*size;
    /* Inverse nearest-pixel sampling keeps squash/stretch solid, with no holes. */
    for(int ty=-8;ty<76;++ty)for(int tx=-8;tx<80;++tx) {
        int sy=66+(int)lroundf((ty-66)/squash);
        int sx=(int)lroundf(tx-(66-sy)*lean);
        if((unsigned)sy>=72||(unsigned)sx>=72)continue;
        uint16_t p=pixels[sy*72+sx];
        if(p==key)continue;
        int px=origin_x+tx*size,py=origin_y+ty*size;
        for(int yy=0;yy<size;++yy)for(int xx=0;xx<size;++xx)
            if((unsigned)(px+xx)<(unsigned)c->width&&(unsigned)(py+yy)<(unsigned)c->height)
                c->pixels[(py+yy)*c->width+px+xx]=p;
    }
}

/* Artwork and input share the round display layout. */
static void icon_button(pg_canvas *c,int x,int y,int w,int h,pg_icon_id a,pg_icon_id b,uint32_t color){
    pg_roundrect(c,x,y+3,w,h,12,0x081e28);pg_roundrect(c,x,y,w,h,12,color);
    if(b==PG_ICON_COUNT)pg_icon(c,a,x+w/2,y+h/2,2,CREAM);
    else {pg_icon(c,a,x+w/2-25,y+h/2,2,CREAM);pg_icon(c,b,x+w/2+25,y+h/2,2,GOLD);}
}
static void icon_header(pg_canvas *c,pg_icon_id id){
    pg_icon(c,id,233,65,3,MINT);
}
static void control(pg_canvas *c,pg_ui_rect r,pg_icon_id id){icon_button(c,r.x,r.y,r.w,r.h,id,PG_ICON_COUNT,PANEL2);}
static void back(pg_canvas *c){control(c,pg_ui_back,PG_ICON_BACK);}
static void pips(pg_canvas *c,int x,int y,int count,float filled,uint32_t color){
    for(int i=0;i<count;++i)pg_roundrect(c,x+i*16,y,11,8,2,i<filled?color:0x35525a);
}
static void need_meter(pg_canvas *c,int x,int y,int width,pg_icon_id id,float amount,uint32_t color){
    pg_icon(c,id,x+15,y+14,2,color);
    int bar_x=x+38,bar_w=width-38;
    pg_roundrect(c,bar_x,y+7,bar_w,14,3,0x29414b);
    int w=(int)((bar_w-4)*pg_clamp(amount,0,100)/100);
    if(w>0)pg_rect(c,bar_x+2,y+10,w,8,color);
    for(int i=1;i<5;++i)pg_rect(c,bar_x+bar_w*i/5,y+7,2,14,PANEL);
}
static void count_icon(pg_canvas *c,pg_icon_id id,int x,int y,unsigned value,uint32_t color){
    char n[16];snprintf(n,sizeof n,"%u",value);pg_icon(c,id,x,y,1,color);pg_text(c,x+15,y-6,n,2,CREAM);
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
            PG_ICON_EYE,PG_ICON_GAME,PG_ICON_TROPHY,PG_ICON_REPLAY,PG_ICON_CLEAN,PG_ICON_MOON};
        a=emotes[g->emote<PG_EMOTE_COUNT?g->emote:0];
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
static void home(pg_canvas *c,const pg_game *g){
    bool egg=g->pet.phase==PG_EGG||g->pet.phase==PG_HATCHING;
    bool hud=g->hud_seconds>0&&!g->sleeping&&g->pet.phase!=PG_HATCHING;
    if(hud){
        pg_ellipse(c,351,77,24,24,PANEL);pg_icon(c,PG_ICON_SETTINGS,351,77,2,MINT);
        pg_roundrect(c,151,68,164,42,16,PANEL);
        if(egg){pg_icon(c,PG_ICON_EGG,215,90,2,GOLD);pg_icon(c,PG_ICON_HEART,255,90,2,CREAM);}
        else pg_center(c,233,81,g->pet.name,3,CREAM);
    }
    home_objects(c,g);
    if(egg)draw_egg(c,g);
    else{
        pg_ellipse(c,(int)g->x,(int)g->y+4,43,9,0x537356);pg_creature(c,g,(int)g->x,(int)g->y,2);
        if(g->petting>0)pg_icon(c,PG_ICON_HEART,(int)g->x-48,(int)g->y-126,1,CORAL);
        if(g->invitation){pg_roundrect(c,(int)g->x+36,(int)g->y-111,39,30,10,CREAM);pg_icon(c,PG_ICON_GAME,(int)g->x+55,(int)g->y-96,2,INK);}
        if((g->sleeping||(g->goal==PG_REST&&g->goal_time>3&&hypotf(g->x-g->target_x,g->y-g->target_y)<3))&&g->emote!=PG_EMOTE_YAWN)
        {
            for(int i=0;i<3;++i){float t=fmodf(g->anim*.35f+i*.33f,1);int zx=(int)g->x+35+(int)(t*21),zy=(int)g->y-92-(int)(t*34);
                pg_text(c,zx,zy,"Z",i==2?2:1,mix(0x597889,CREAM,1-t));}
        }
        if(!g->sleeping&&g->emote==PG_EMOTE_GREET&&g->emote_time>0){
            int bob=(int)(sinf(g->anim*5)*3);pg_roundrect(c,(int)g->x+38,(int)g->y-126+bob,30,26,9,CREAM);
            pg_icon(c,PG_ICON_HEART,(int)g->x+53,(int)g->y-113+bob,1,CORAL);
        }
        if(g->bowl_time>0){pg_ellipse(c,234,300,20,6,0xe3c4a1);pg_ellipse(c,234,296,20,6,0xaa7559);for(int i=0;i<4;++i)pg_ellipse(c,224+i*7,293+(i%2)*2,4,3,CORAL);}
    }
    for(int i=0;i<8&&g->particles>0;++i){float a=i*0.785f,t=3-g->particles;pg_star(c,233+(int)(cosf(a)*(30+t*30)),235+(int)(sinf(a)*(20+t*20))-(int)(t*16),3,i%2?GOLD:MINT);}
    if(!g->sleeping&&(hud||g->caption_time>0||g->pet.satiety<25))thought(c,g);
    pg_icon_id dock[]={egg?PG_ICON_WARM:PG_ICON_BOWL,PG_ICON_HEART,egg?PG_ICON_SHAKE:PG_ICON_GAME,PG_ICON_BOOK};
    for(int i=0;hud&&i<4;++i){int x=pg_ui_dock[i][0],y=pg_ui_dock[i][1];pg_ellipse(c,x,y+3,30,29,0x122f39);pg_ellipse(c,x,y,29,29,i==1?0x385349:0x23454a);pg_icon(c,dock[i],x,y+1,3,i==1?GOLD:MINT);}
    if(!hud&&!g->sleeping&&g->pet.phase!=PG_HATCHING)pg_icon(c,PG_ICON_TAP,233,405,2,0xa2c5b4);
    if(g->sleeping){pg_icon(c,PG_ICON_TAP,119,257,1,GOLD);}
}
static void card(pg_canvas *c,const pg_game *g){
    const pg_pet *p=&g->pet;
    if(p->phase==PG_EGG){
        icon_header(c,PG_ICON_EGG);
        need_meter(c,105,138,256,PG_ICON_WARM,p->warmth,p->warmth<40?GOLD:MINT);
        need_meter(c,105,185,256,PG_ICON_HEART,p->comfort,CORAL);
        need_meter(c,105,232,256,PG_ICON_HOURGLASS,(float)(p->incubation/PG_EGG_SECONDS*100),0xb9b6e5);
        pg_icon(c,PG_ICON_HOLD,170,290,2,CREAM);pips(c,207,286,3,(float)p->egg_visits,GOLD);
        pg_icon(c,PG_ICON_SHELTER,278,290,2,p->sheltered?MINT:MUTED);if(p->sheltered)pg_icon(c,PG_ICON_CHECK,307,290,1,MINT);
    }else{
        pg_center(c,233,53,p->name,2,GOLD);pg_center(c,233,77,pg_species_names[p->species],1,MUTED);
        for(int i=0;i<3;++i)pg_icon(c,PG_ICON_SPROUT,211+i*22,106,1,i<=(int)g->stage?MINT:0x35525a);
        need_meter(c,93,133,128,PG_ICON_BOWL,p->satiety,GOLD);need_meter(c,247,133,128,PG_ICON_ENERGY,p->energy,MINT);
        need_meter(c,93,177,128,PG_ICON_CLEAN,p->clean,0x9dc5ce);need_meter(c,247,177,128,PG_ICON_HEART,p->affection,CORAL);
        need_meter(c,93,221,128,PG_ICON_HEALTH,p->health,p->health<30?CORAL:MINT);need_meter(c,247,221,128,PG_ICON_SHIELD,p->trust,0xc6b5db);
        pg_icon_id traits[]={PG_ICON_CURIOUS,PG_ICON_PLAYFUL,PG_ICON_BOLD};float values[]={p->curious,p->playful,p->bold};
        for(int i=0;i<3;++i){int x=148+i*85;pg_icon(c,traits[i],x,275,1,CREAM);pips(c,x-19,290,3,values[i]/34,MINT);}
        count_icon(c,PG_ICON_DAY,162,320,(unsigned)(p->age/PG_DAY)+1,GOLD);
        count_icon(c,PG_ICON_GAME,264,320,p->games_played[0]+p->games_played[1]+p->games_played[2]+p->games_played[3]+p->games_played[4]+p->games_played[5],MINT);
    }
    icon_button(c,119,340,228,37,PG_ICON_STROKE,PG_ICON_HEART,PANEL2);back(c);
}
static void food(pg_canvas *c,const pg_game *g){
    icon_header(c,PG_ICON_BOWL);
    for(int i=0;i<3;++i){
        int x=128+i*106;pg_roundrect(c,x-47,160,94,144,14,PANEL2);
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
    icon_button(c,95,155,276,58,egg?PG_ICON_HOLD:PG_ICON_STROKE,egg?PG_ICON_WARM:PG_ICON_HEART,0x385b51);
    icon_button(c,95,222,276,58,egg?PG_ICON_STROKE_EGG:PG_ICON_CLEAN,egg?PG_ICON_EGG:PG_ICON_COUNT,PANEL2);
    icon_button(c,95,289,276,58,egg?PG_ICON_SHELTER:PG_ICON_MOON,PG_ICON_COUNT,0x383e59);back(c);
}
static void games_menu(pg_canvas *c,const pg_game *g){
    icon_header(c,PG_ICON_GAME);pips(c,222,110,2,g->games_page?2:1,MINT);
    static const pg_icon_id icons[]={PG_ICON_TTT,PG_ICON_TILT,PG_ICON_MUSIC,PG_ICON_ECHO,PG_ICON_BALANCE,PG_ICON_WASH};
    static const char *const labels[]={"PAW MATCH","STAR TILT","LITTLE TUNES","ECHOES","LEAF BALANCE","BUBBLE BATH"};
    for(int i=0;i<3;++i){int which=i+(g->games_page?3:0),y=148+i*73;
        pg_roundrect(c,95,y+3,276,64,16,0x081e28);pg_roundrect(c,95,y,276,64,16,i==0?0x385b51:i==1?0x3b4e60:0x534852);
        pg_icon(c,icons[which],128,y+32,2,which==5?0xb3e6e1:MINT);
        pg_center(c,252,y+25,labels[which],2,CREAM);
    }
    control(c,pg_ui_games_home,PG_ICON_HOME);
    control(c,g->games_page?pg_ui_page_previous:pg_ui_page_next,g->games_page?PG_ICON_BACK:PG_ICON_NEXT);
}
static void ttt(pg_canvas *c,const pg_game *g){
    icon_header(c,g->ttt.finished?(g->ttt.winner==1?PG_ICON_TROPHY:g->ttt.winner==2?PG_ICON_HEART:PG_ICON_REPLAY):PG_ICON_TTT);
    pg_icon(c,PG_ICON_HAND,165,92,1,g->ttt.turn==1?GOLD:MUTED);pg_icon(c,PG_ICON_X,185,92,1,GOLD);
    pg_icon(c,PG_ICON_PAW,281,92,1,g->ttt.turn==-1?MINT:MUTED);pg_icon(c,PG_ICON_CIRCLE,301,92,1,MINT);
    if(!g->ttt.finished)pg_icon(c,g->ttt.turn==1?PG_ICON_TAP:PG_ICON_HOURGLASS,233,112,1,CREAM);
    for(int i=0;i<9;++i){int x=122+(i%3)*74,y=121+(i/3)*74;pg_roundrect(c,x+3,y+3,68,68,10,PANEL2);
        if(g->ttt.cells[i])pg_icon(c,g->ttt.cells[i]==1?PG_ICON_X:PG_ICON_CIRCLE,x+37,y+37,3,g->ttt.cells[i]==1?GOLD:MINT);
    }
    if(g->ttt.finished){pg_creature(c,g,233,431,1);icon_button(c,135,351,196,32,PG_ICON_REPLAY,PG_ICON_COUNT,0x385b51);}
    else{pg_icon(c,PG_ICON_STAR,186,368,1,MINT);pips(c,206,363,5,g->pet.confidence[0]/20,MINT);}
    back(c);
}
static void tilt(pg_canvas *c,const pg_game *g){
    icon_header(c,PG_ICON_TILT);count_icon(c,PG_ICON_CLOCK,170,99,(unsigned)ceilf(g->tilt.seconds),GOLD);count_icon(c,PG_ICON_STAR,273,99,(unsigned)g->tilt.score,MINT);
    pg_ellipse(c,233,244,120,120,0x46615c);pg_ellipse(c,233,244,114,114,0x203e48);
    for(int i=0;i<15;++i)pg_pixel(c,145+(i*47)%175,161+(i*31)%172,0x53716d);
    pg_star(c,(int)g->tilt.target_x,(int)g->tilt.target_y,12,GOLD);
    pg_ellipse(c,(int)g->tilt.x+2,(int)g->tilt.y+4,13,11,0x102a36);pg_ellipse(c,(int)g->tilt.x,(int)g->tilt.y,12,12,MINT);pg_ellipse(c,(int)g->tilt.x-3,(int)g->tilt.y-4,4,4,0xd0eed3);
    if(g->tilt.finished){pg_creature(c,g,233,431,1);icon_button(c,135,351,196,32,PG_ICON_REPLAY,PG_ICON_COUNT,0x385b51);}
    else{
        static const int xy[4][2]={{233,142},{97,244},{369,244},{233,346}},dir[4][2]={{0,-1},{-1,0},{1,0},{0,1}};
        for(int i=0;i<4;++i){int x=xy[i][0],y=xy[i][1],dx=dir[i][0],dy=dir[i][1];pg_roundrect(c,x-24,y-24,48,48,10,PANEL2);pg_line(c,x-dx*9,y-dy*9,x+dx*9,y+dy*9,3,MINT);pg_line(c,x+dx*9,y+dy*9,x+dy*7,y-dx*7,3,MINT);pg_line(c,x+dx*9,y+dy*9,x-dy*7,y+dx*7,3,MINT);}
        if(g->tilt.ready>0){pg_roundrect(c,140,211,186,60,12,PANEL);pg_center(c,233,224,"HOLD STILL",2,CREAM);pg_icon(c,PG_ICON_STILL,233,253,2,GOLD);}
        pg_center(c,233,369,"TAP TOP TO CENTER",2,MUTED);
    }back(c);
}
static void echoes(pg_canvas *c,const pg_game *g){
    const pg_echo *e=&g->echo;icon_header(c,PG_ICON_ECHO);pips(c,198,116,5,(float)e->passed,MINT);
    if(e->stage==PG_ECHO_PASS||e->stage==PG_ECHO_DONE){
        pg_game pose=*g;pose.goal=PG_ATTENTION;pose.pet.health=100;pose.pet.satiety=100;pose.pet.affection=100;
        pg_creature(c,&pose,233,266,2);pg_icon(c,e->stage==PG_ECHO_DONE?PG_ICON_TROPHY:PG_ICON_CHECK,233,302,3,MINT);
        icon_button(c,119,343,228,40,e->stage==PG_ECHO_DONE?PG_ICON_REPLAY:PG_ICON_NEXT,PG_ICON_COUNT,0x385b51);
    }else{
        bool go=e->stage==PG_ECHO_INPUT;pg_ellipse(c,233,234,91,91,go?MINT:0x355961);pg_ellipse(c,233,234,87,87,PANEL2);
        pg_icon_id gesture=e->prompt==PG_SHAKE?PG_ICON_SHAKE:e->prompt==PG_SWIPE?PG_ICON_SWIPE:PG_ICON_TAP;
        pg_icon(c,gesture,233,225,6,CREAM);
        pg_icon_id state=go?PG_ICON_PLAY:e->stage==PG_ECHO_RETRY?PG_ICON_REPLAY:e->stage==PG_ECHO_GO?PG_ICON_HOURGLASS:PG_ICON_EYE;
        pg_icon(c,state,233,296,2,go?MINT:GOLD);
        pg_icon(c,g->muted?PG_ICON_MUTE:PG_ICON_EAR,344,139,1,MUTED);
        if(go){char timing[16];snprintf(timing,sizeof timing,"GO! %d",(int)ceilf(e->timer));pg_center(c,233,343,timing,2,MINT);pg_roundrect(c,163,372,140,5,2,PANEL2);pg_roundrect(c,163,372,(int)(140*pg_clamp(e->timer/3,0,1)),5,2,MINT);}
        else if(e->stage==PG_ECHO_CUE||e->stage==PG_ECHO_GO||e->stage==PG_ECHO_RETRY)
            icon_button(c,119,343,228,40,PG_ICON_REPLAY,e->stage==PG_ECHO_RETRY?PG_ICON_HEART:PG_ICON_EAR,PANEL2);
    }back(c);
}
static void rhythm(pg_canvas *c,const pg_game *g){
    icon_header(c,PG_ICON_MUSIC);count_icon(c,PG_ICON_STAR,216,110,(unsigned)(g->rhythm.length-1),GOLD);
    pg_icon(c,g->rhythm.finished?PG_ICON_CHECK:g->rhythm.showing?PG_ICON_EYE:PG_ICON_HAND,233,149,2,g->rhythm.showing?GOLD:MINT);
    uint32_t colors[]={0x739e82,0xbd8d72,0x858eb8};pg_icon_id shapes[]={PG_ICON_SPROUT,PG_ICON_STAR,PG_ICON_MOON};
    for(int i=0;i<3;++i){int x=134+i*99;bool lit=g->rhythm.lit==i;pg_roundrect(c,x-43,190,86,119,16,lit?mix(colors[i],CREAM,0.55f):colors[i]);pg_icon(c,shapes[i],x,240,3,lit?CREAM:i==0?MINT:i==1?GOLD:0xbcc7eb);char n[2]={(char)('1'+i),0};pg_center(c,x,282,n,1,CREAM);}
    if(g->rhythm.finished){pg_creature(c,g,233,431,1);icon_button(c,135,351,196,32,PG_ICON_REPLAY,PG_ICON_COUNT,0x385b51);}
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
        pg_line(c,108,y-18,108,y+18,1,0x35525a);pg_rect(c,105,y-3,7,7,MINT);pg_icon(c,event_icon(e),153,y,2,e->kind==PG_EV_HATCHED?GOLD:MINT);
        pg_icon(c,event_object(e),202,y,1,CREAM);
        count_icon(c,PG_ICON_DAY,275,y,(unsigned)(e->at/PG_DAY)+1,MUTED);
    }
    icon_button(c,151,340,164,36,PG_ICON_BOOK,PG_ICON_NEXT,PANEL2);back(c);
}
static void settings(pg_canvas *c,const pg_game *g){
    icon_header(c,PG_ICON_SETTINGS);
    pg_roundrect(c,101,113,264,62,14,PANEL2);pg_icon(c,PG_ICON_SUN,132,144,2,GOLD);
    pips(c,167,140,5,g->pet.brightness<=20?1:g->pet.brightness<=40?2:g->pet.brightness<=65?3:g->pet.brightness<=85?4:5,MINT);
    char n[12];snprintf(n,sizeof n,"%u",g->pet.brightness);pg_center(c,325,137,n,2,CREAM);
    icon_button(c,101,189,264,60,g->muted?PG_ICON_MUTE:PG_ICON_SPEAKER,PG_ICON_COUNT,PANEL2);
    icon_button(c,101,263,264,60,PG_ICON_BED,PG_ICON_MOON,0x383e59);
    icon_button(c,133,335,88,48,PG_ICON_QUESTION,PG_ICON_COUNT,PANEL2);
    pg_roundrect(c,245,335,88,48,12,0x303e49);pg_center(c,289,352,"DEV",2,MUTED);back(c);
}
static void developer(pg_canvas *c,const pg_game *g){
    pg_center(c,233,60,"DEV TOOLS",3,GOLD);
    pg_center(c,233,102,"TEST BUILD",2,MUTED);
    pg_roundrect(c,103,150,260,66,14,0x385b51);pg_icon(c,PG_ICON_EGG,139,183,2,MINT);
    pg_center(c,256,175,"HATCH",3,CREAM);
    pg_roundrect(c,103,238,260,66,14,g->debug_reset_armed?0x784b49:PANEL2);pg_icon(c,PG_ICON_REPLAY,139,271,2,CORAL);
    pg_center(c,256,262,g->debug_reset_armed?"SURE?":"RESET",3,CREAM);
    pg_center(c,233,329,g->debug_reset_armed?"TAP AGAIN TO ERASE":"NEW EGG / ERASE LIFE",2,g->debug_reset_armed?CORAL:MUTED);
    back(c);
}
static void balance(pg_canvas *c,const pg_game *g){
    const pg_balance *b=&g->balance;
    pg_center(c,233,53,"LEAF BALANCE",2,CREAM);
    for(int i=0;i<3;++i)pg_icon(c,PG_ICON_STAR,197+i*36,103,2,b->steady>=(i+1)*4?GOLD:0x3b5a62);
    /* Soft leaf seesaw and an original creature with outstretched tiny paws. */
    float angle=b->angle;int dx=(int)(cosf(angle)*110),dy=(int)(sinf(angle)*75);
    pg_ellipse(c,233,300,111,12,0x0b222e);
    pg_ellipse(c,233,267,103,22,0x233f47);
    pg_line(c,233-dx,278-dy,233+dx,278+dy,16,0x446d62);
    pg_line(c,233-dx+5,273-dy,233+dx-5,273+dy,8,MINT);
    pg_line(c,233,286,218,309,6,0x648b75);pg_line(c,233,286,248,309,6,0x648b75);
    int px=233+(int)(angle*58),py=269+(int)(sinf(angle)*angle*35)-(b->rescue>0?(int)(sinf(b->rescue*7)*9):0);
    pg_game pose=*g;pose.goal=PG_PLAY;pose.facing=angle<0?-1:1;pose.pet.health=pose.pet.satiety=100;
    pg_creature(c,&pose,px,py,2);
    pg_icon(c,PG_ICON_HEART,px-68,py-55,1,b->rescue>0?CORAL:MINT);
    if(!b->started){pg_icon(c,PG_ICON_TILT_GESTURE,233,142,2,GOLD);pg_center(c,233,316,"TIP GENTLY",2,MINT);}
    else if(b->ready>0)pg_center(c,233,316,"READY...",3,GOLD);
    else if(!b->finished){char n[12];snprintf(n,sizeof n,"%u",(unsigned)ceilf(b->seconds));pg_center(c,233,317,n,2,CREAM);}
    if(!b->started||b->finished){
        pg_roundrect(c,123,344,220,40,12,0x385b51);
        pg_icon(c,b->finished?PG_ICON_REPLAY:PG_ICON_PLAY,233,364,2,CREAM);
        if(b->finished)pg_icon(c,b->steady>=12?PG_ICON_TROPHY:PG_ICON_HEART,233,142,2,GOLD);
    }else {pg_icon(c,PG_ICON_BACK,126,230,2,MINT);pg_icon(c,PG_ICON_NEXT,340,230,2,MINT);}
    back(c);
}
static void wash(pg_canvas *c,const pg_game *g){
    const pg_wash *w=&g->wash;
    pg_center(c,233,53,"BUBBLE BATH",2,CREAM);
    pg_roundrect(c,158,96,150,12,4,PANEL2);
    if(w->total)pg_roundrect(c,158,96,(int)(150.0f*(w->total-w->left)/w->total),12,4,MINT);
    pg_ellipse(c,233,264,137,99,0x0c2530);pg_ellipse(c,233,262,130,91,0x345d64);
    pg_game pose=*g;pose.goal=PG_ATTENTION;pose.pet.health=pose.pet.satiety=100;pose.emote=PG_EMOTE_CLEAN;pose.emote_time=pose.emote_duration=2;
    pg_creature(c,&pose,233,285,2);
    for(int i=0;i<PG_FOAM_COUNT;++i)if(w->foam[i]){
        int x=112+i%PG_FOAM_SIDE*22,y=127+i/PG_FOAM_SIDE*22;
        if(w->splash>0){float t=1-w->splash/0.65f;x=233+(int)((x-233)*t);y=248+(int)((y-248)*t);}
        int r=15+w->foam[i];pg_ellipse(c,x+1,y+2,r,r,0x6dabae);pg_ellipse(c,x,y,r-1,r-1,i%3?0xc6e9dd:0x9ed8d1);
        pg_ellipse(c,x-4,y-5,4,3,0xf1f5df);
    }
    if(w->finished){pg_icon(c,PG_ICON_SPARKLE,160,169,3,GOLD);pg_icon(c,PG_ICON_SPARKLE,309,183,2,CREAM);
        icon_button(c,123,344,220,40,PG_ICON_REPLAY,PG_ICON_HEART,0x385b51);
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
    pg_center(c,233,133,"A SMALL LIFE REMEMBERED",1,MUTED);pg_center(c,233,306,"ALWAYS PART OF THIS HOME.",1,CREAM);
    /* Keep this consequential choice explicit; a heart must not silently make a new pet. */
    pg_roundrect(c,110,347,246,45,12,PANEL2);pg_icon(c,g->pet.death_acknowledged?PG_ICON_EGG:PG_ICON_BOOK,141,370,2,MINT);
    pg_center(c,254,362,g->pet.death_acknowledged?"WELCOME EGG":"REMEMBER",2,CREAM);
}

/* Separate interface composition permits an unclipped geometry audit. */
void pg_render_interface(pg_canvas *c,const pg_game *g) {
    if(!c||!c->pixels||c->width!=466||c->height!=466||!g)return;
    switch(g->screen){
    case PG_HOME:home(c,g);break;case PG_CARD:card(c,g);break;case PG_FOOD:food(c,g);break;
    case PG_CARE:care(c,g);break;case PG_GAMES:games_menu(c,g);break;case PG_TTT:ttt(c,g);break;
    case PG_TILT:tilt(c,g);break;case PG_RHYTHM:rhythm(c,g);break;case PG_ALBUM:album(c,g);break;
    case PG_ECHO:echoes(c,g);break;
    case PG_BALANCE:balance(c,g);break;case PG_WASH:wash(c,g);break;case PG_DEBUG:developer(c,g);break;
    case PG_SETTINGS:settings(c,g);break;case PG_HELP:help(c,g);break;case PG_MEMORIAL:memorial(c,g);break;
    case PG_SCREEN_COUNT:break;
    }
}
void pg_render(pg_canvas *c,const pg_game *g) {
    if(!c||!c->pixels||c->width!=466||c->height!=466||!g)return;
    if(g->standby){pg_rect(c,0,0,466,466,0);return;}
    scene_background(c,g);
    if(g->screen!=PG_HOME){
        /* One write per pixel keeps the circular panel inexpensive in PSRAM. */
        for(int dy=-219;dy<=219;++dy){
            int outer=(int)(219*sqrtf(1.0f-(float)(dy*dy)/(219*219)));
            if(dy<-217||dy>217){pg_rect(c,233-outer,233+dy,outer*2+1,1,0x35525a);continue;}
            int inner=(int)(217*sqrtf(1.0f-(float)(dy*dy)/(217*217)));
            pg_rect(c,233-outer,233+dy,outer-inner,1,0x35525a);
            pg_rect(c,233-inner,233+dy,inner*2+1,1,PANEL);
            pg_rect(c,234+inner,233+dy,outer-inner,1,0x35525a);
        }
    }
    pg_render_interface(c,g);
    /* The simulator and device share the exact round clipping. */
    for(int y=0;y<466;++y){
        int dy=y-233,dx=(int)sqrtf(233.0f*233-dy*dy);
        pg_rect(c,0,y,233-dx,1,0x07151c);pg_rect(c,234+dx,y,466,1,0x07151c);
    }
}
