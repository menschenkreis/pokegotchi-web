#include "game.h"
#include "layout.h"
#include <math.h>
#include <string.h>

static bool playing(const pg_game *g) {
    return g->screen==PG_ECHO && g->pet.phase==PG_ALIVE && !g->sleeping && !g->standby;
}
static void stage(pg_game *g, pg_echo_stage next) {
    pg_echo *e=&g->echo;
    if(next==PG_ECHO_PASS||next==PG_ECHO_DONE)pg_react(g,PG_EMOTE_CHEER,PG_SOUND_NONE);
    else if(next==PG_ECHO_RETRY)pg_react(g,PG_EMOTE_OOPS,PG_SOUND_NONE);
    e->stage=next;e->pointer_armed=false;e->impulses=0;e->shake_sign=0;e->shake_gap=0;
    e->audio_wait=g->echo_audio&&!g->muted&&(next==PG_ECHO_CUE||next==PG_ECHO_GO);
    e->timer=next==PG_ECHO_CUE?0.6f:next==PG_ECHO_GO?0.18f:next==PG_ECHO_INPUT?3:1;
    if(e->audio_wait)e->timer=2; /* Lost output falls back to a fresh visual cue. */
}
static void prompt(pg_game *g) {
    pg_echo *e=&g->echo;
    ++e->round;e->prompt=(pg_gesture)e->sequence[e->passed];e->muted=g->muted;
    g->pointer_down=false;g->sound=PG_SOUND_NONE;
    stage(g,PG_ECHO_CUE);
}
void pg_echo_cancel(pg_game *g) {
    ++g->echo.session;g->echo.stage=PG_ECHO_OFF;g->echo.audio_wait=false;
    g->echo.pointer_armed=false;g->echo.impulses=0;g->pointer_down=false;
}
void pg_echo_start(pg_game *g) {
    uint32_t session=g->echo.session+1;
    memset(&g->echo,0,sizeof g->echo);
    g->echo.session=session;g->screen=PG_ECHO;
    for(int i=0;i<5;++i)g->echo.sequence[i]=(uint8_t)(i%3);
    /* All three gestures appear, with a fresh order each session. */
    for(int i=4;i>0;--i){int j=(int)(pg_random(&g->pet)%(uint32_t)(i+1));
        uint8_t t=g->echo.sequence[i];g->echo.sequence[i]=g->echo.sequence[j];g->echo.sequence[j]=t;}
    prompt(g);
}
void pg_echo_replay(pg_game *g) {
    if(playing(g)&&(g->echo.stage==PG_ECHO_CUE||g->echo.stage==PG_ECHO_GO||g->echo.stage==PG_ECHO_RETRY))prompt(g);
}
void pg_echo_next(pg_game *g) {
    if(playing(g)&&g->echo.stage==PG_ECHO_PASS)prompt(g);
}
void pg_echo_audio_done(pg_game *g,uint32_t session,uint32_t round,pg_echo_stage current) {
    pg_echo *e=&g->echo;
    if(!playing(g)||g->muted||!e->audio_wait||e->session!=session||e->round!=round||e->stage!=current)return;
    if(current==PG_ECHO_CUE)stage(g,PG_ECHO_GO);
    else if(current==PG_ECHO_GO)stage(g,PG_ECHO_INPUT);
}
void pg_echo_tick(pg_game *g,float dt) {
    if(!playing(g))return;
    pg_echo *e=&g->echo;
    e->shake_gap+=dt;e->shake_cooldown=fmaxf(0,e->shake_cooldown-dt);
    if(e->shake_gap>0.7f){e->impulses=0;e->shake_sign=0;}
    if(e->muted!=g->muted&&(e->stage==PG_ECHO_CUE||e->stage==PG_ECHO_GO||e->stage==PG_ECHO_INPUT)){prompt(g);return;}
    if(e->stage==PG_ECHO_PASS||e->stage==PG_ECHO_DONE||e->stage==PG_ECHO_OFF)return;
    e->timer=fmaxf(0,e->timer-dt);
    if(e->timer>0)return;
    if(e->audio_wait){g->echo_audio=false;prompt(g);return;}
    if(e->stage==PG_ECHO_CUE)stage(g,PG_ECHO_GO);
    else if(e->stage==PG_ECHO_GO)stage(g,PG_ECHO_INPUT);
    else if(e->stage==PG_ECHO_INPUT)stage(g,PG_ECHO_RETRY);
    else if(e->stage==PG_ECHO_RETRY)prompt(g);
}
void pg_echo_gesture(pg_game *g,pg_gesture action) {
    pg_echo *e=&g->echo;
    if(!playing(g)||e->stage!=PG_ECHO_INPUT||action<PG_SHAKE||action>PG_TAP)return;
    g->player_idle=0;
    if(action!=e->prompt){stage(g,PG_ECHO_RETRY);return;}
    ++e->passed;
    if(e->passed==5){stage(g,PG_ECHO_DONE);pg_game_result(g,3,1);g->sound=PG_SOUND_NONE;}
    else stage(g,PG_ECHO_PASS);
}
void pg_echo_shake_key(pg_game *g) {
    /* A non-repeating key edge represents one complete physical gesture. */
    if(g->echo.shake_cooldown>0)return;
    pg_echo_gesture(g,PG_SHAKE);g->echo.shake_cooldown=0.8f;
}
void pg_echo_accel(pg_game *g,float x,float y,float z,float dt) {
    if(!isfinite(x)||!isfinite(y)||!isfinite(z)||!isfinite(dt)||dt<=0||dt>0.1f)return;
    pg_echo *e=&g->echo;float a[3]={x,y,z};
    if(!e->gravity_ready){memcpy(e->gravity,a,sizeof a);e->gravity_ready=true;return;}
    float linear[3],alpha=dt/(0.35f+dt);int axis=0;
    for(int i=0;i<3;++i){e->gravity[i]+=alpha*(a[i]-e->gravity[i]);linear[i]=a[i]-e->gravity[i];
        if(fabsf(linear[i])>fabsf(linear[axis]))axis=i;}
    if(!playing(g)||e->stage!=PG_ECHO_INPUT||e->shake_cooldown>0||fabsf(linear[axis])<0.35f)return;
    int sign=linear[axis]>0?1:-1;
    if(e->shake_gap<0.06f)return;
    if(!e->impulses||axis!=e->shake_axis){e->impulses=1;e->shake_axis=axis;e->shake_sign=sign;e->shake_gap=0;}
    else if(sign!=e->shake_sign){++e->impulses;e->shake_sign=sign;e->shake_gap=0;}
    if(e->impulses>=3){pg_echo_gesture(g,PG_SHAKE);e->shake_cooldown=0.8f;}
}
void pg_echo_pointer(pg_game *g,int kind,float x,float y) {
    pg_echo *e=&g->echo;
    if(!pg_ui_visible(x,y)){if(kind!=0){g->pointer_down=false;e->pointer_armed=false;}return;}
    if(kind==0){
        if(g->pointer_down||(x-233)*(x-233)+(y-233)*(y-233)>233*233)return;
        g->pointer_down=true;g->pointer_start_x=g->pointer_x=x;g->pointer_start_y=g->pointer_y=y;
        g->pointer_time=g->pointer_distance=0;g->player_idle=0;
        e->pointer_armed=e->stage==PG_ECHO_INPUT&&y>=120&&y<=330;
        e->pointer_round=e->round;return;
    }
    if(!g->pointer_down)return;
    g->pointer_distance+=hypotf(x-g->pointer_x,y-g->pointer_y);g->pointer_x=x;g->pointer_y=y;
    if(kind!=2)return;
    g->pointer_down=false;
    float distance=hypotf(x-g->pointer_start_x,y-g->pointer_start_y);
    bool short_tap=g->pointer_time<=0.5f&&g->pointer_distance<12;
    if(short_tap&&g->pointer_start_y>=340&&y>=340){pg_tap(g,(int)x,(int)y);return;}
    if(!e->pointer_armed||e->pointer_round!=e->round||e->stage!=PG_ECHO_INPUT)return;
    e->pointer_armed=false;
    if(distance>=64&&g->pointer_time<=0.8f)pg_echo_gesture(g,PG_SWIPE);
    else if(short_tap&&hypotf(x-233,y-238)<=90&&hypotf(g->pointer_start_x-233,g->pointer_start_y-238)<=90)
        pg_echo_gesture(g,PG_TAP);
}
