#include "game.h"
#include "layout.h"
#include <math.h>
#include <string.h>

void pg_debug_hatch(pg_game *g){
    if(g->pet.phase!=PG_EGG)return;
    pg_echo_cancel(g);g->sleeping=false;g->screen=PG_HOME;g->hud_seconds=8;
    g->pet.incubation=PG_EGG_SECONDS;g->pet.egg_visits=g->pet.egg_visits<3?3:g->pet.egg_visits;
    g->pet.phase=PG_HATCHING;g->hatch_time=0;g->dirty=true;++g->revision;
    pg_say(g,"A LITTLE HELLO IS ON ITS WAY.",PG_SOUND_HATCH);
}
void pg_debug_reset(pg_game *g){
    if(g->screen!=PG_DEBUG||!g->debug_reset_armed)return;
    if(g->transactional_resets){g->reset_request=PG_REQUEST_DEV_RESET;return;}
    pg_device_data device=g->device;pg_device_runtime utilities=g->utilities;
    bool muted=g->muted,audio=g->echo_audio,brain=g->brain_ready;uint8_t brightness=g->pet.brightness;
    uint32_t revision=g->revision+1,session=g->echo.session+1,seed=pg_random(&g->pet);
    pg_init(g,seed);g->device=device;g->utilities=utilities;g->utilities.editing=false;g->utilities.idle=0;g->muted=g->pet.muted=muted;g->pet.brightness=brightness;
    g->echo.session=session;g->echo_audio=audio;g->brain_ready=brain;g->revision=revision;g->dirty=true;
}
void pg_balance_start(pg_game *g){
    memset(&g->balance,0,sizeof g->balance);g->balance.seconds=20;
    g->balance.gust_timer=1;g->tilt_x=g->tilt_y=0;g->screen=PG_BALANCE;
    g->pointer_down=false;
}
void pg_balance_go(pg_game *g){
    if(g->screen!=PG_BALANCE||g->balance.started||g->balance.finished)return;
    g->balance.started=true;g->balance.ready=1.2f;g->tilt_x=0;++g->motion_serial;
    if(!g->muted)g->sound=PG_SOUND_NOTE0;
}
void pg_wash_start(pg_game *g){
    memset(&g->wash,0,sizeof g->wash);g->wash.splash=0.65f;g->screen=PG_WASH;
    g->pointer_down=false;
    /* Full coverage, irregular lather, no invisible cells outside the bowl. */
    for(int i=0;i<PG_FOAM_COUNT;++i){int x=112+i%PG_FOAM_SIDE*22,y=127+i/PG_FOAM_SIDE*22;
        if((x-233)*(x-233)+(y-248)*(y-248)<=122*122){
            g->wash.foam[i]=(uint8_t)(1+pg_random(&g->pet)%3);++g->wash.left;
        }
    }
    g->wash.total=g->wash.left;
    if(!g->muted)g->sound=PG_SOUND_CLEAN;
}
static void wipe(pg_game *g,float ax,float ay,float bx,float by){
    pg_wash *w=&g->wash;float dx=bx-ax,dy=by-ay,len=dx*dx+dy*dy;
    unsigned before=w->left;
    for(int i=0;i<PG_FOAM_COUNT;++i)if(w->foam[i]){
        float x=112+i%PG_FOAM_SIDE*22,y=127+i/PG_FOAM_SIDE*22;
        float t=len>0?pg_clamp(((x-ax)*dx+(y-ay)*dy)/len,0,1):0;
        float ex=x-(ax+t*dx),ey=y-(ay+t*dy);
        if(ex*ex+ey*ey<=38*38){w->foam[i]=0;--w->left;}
    }
    if(w->total&&w->left&&((w->total-before)*5/w->total)!=((w->total-w->left)*5/w->total))pg_react(g,PG_EMOTE_CHEER,PG_SOUND_NONE);
    if(before!=w->left&&w->sound_gap<=0){if(!g->muted)g->sound=PG_SOUND_TAP;w->sound_gap=0.12f;}
    if(w->left==0&&!w->finished){
        w->finished=true;w->stroke=false;pg_clean(g);pg_game_result(g,5,1);
        pg_react(g,PG_EMOTE_CLEAN,PG_SOUND_CLEAN);
    }
}
void pg_wash_pointer(pg_game *g,int kind,float x,float y){
    pg_wash *w=&g->wash;
    if(g->screen!=PG_WASH||g->sleeping||!isfinite(x)||!isfinite(y))return;
    g->player_idle=0;
    if(!pg_ui_visible(x,y)){w->stroke=false;return;}
    if(kind==0){
        if(pg_ui_hit(pg_ui_back,(int)x,(int)y)){pg_back(g);return;}
        if(w->finished){pg_tap(g,(int)x,(int)y);return;}
        w->stroke=w->splash<=0;w->last_x=x;w->last_y=y;
        return; /* A tap alone does not wash: make a stroke. */
    }
    if(w->stroke&&!w->finished&&w->splash<=0){
        if(hypotf(x-w->last_x,y-w->last_y)>=2)wipe(g,w->last_x,w->last_y,x,y);
        w->last_x=x;w->last_y=y;
    }
    if(kind==2)w->stroke=false;
}
void pg_nurture_tick(pg_game *g,float dt){
    if(g->screen==PG_WASH){g->wash.splash=fmaxf(0,g->wash.splash-dt);g->wash.sound_gap=fmaxf(0,g->wash.sound_gap-dt);}
    if(g->screen!=PG_BALANCE||!g->balance.started||g->balance.finished)return;
    pg_balance *b=&g->balance;
    if(b->ready>0){float sample=fminf(dt,b->ready);b->neutral+=g->tilt_x*sample/1.2f;b->ready=fmaxf(0,b->ready-dt);return;}
    b->seconds=fmaxf(0,b->seconds-dt);b->rescue=fmaxf(0,b->rescue-dt);
    b->gust_timer-=dt;
    if(b->gust_timer<=0){b->gust=(pg_randf(&g->pet)-0.5f)*0.8f;b->gust_timer=1.5f+pg_randf(&g->pet)*1.5f;}
    b->velocity+=((g->tilt_x-b->neutral)*2.8f+b->gust+b->angle*0.45f)*dt;
    b->velocity*=expf(-2.2f*dt);b->angle+=b->velocity*dt;
    if(fabsf(b->angle)<0.24f){b->steady=fminf(12,b->steady+dt);}
    if(fabsf(b->angle)>0.65f){
        b->angle*=0.25f;b->velocity=0;b->rescue=1.2f;++b->rescues;
        if(!g->muted)g->sound=PG_SOUND_CURIOUS;
    }
    if(b->steady>=12||b->seconds<=0){
        b->finished=true;g->tilt_x=0;pg_game_result(g,4,b->steady>=12?1:-1);
    }
}
