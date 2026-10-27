#include "game.h"
#include "layout.h"
#include <math.h>
#include <string.h>
static bool playing(const pg_game *g){return g->screen==PG_ECHO&&g->pet.phase==PG_ALIVE&&!g->sleeping&&!g->standby;
    }
static uint32_t random_prompt(pg_echo *e){uint32_t x=e->prompt_rng;
    x^=x<<13;
    x^=x>>17;
    x^=x<<5;
    return e->prompt_rng=x?x:0x97c29b3d;
    }
static void bag(pg_echo *e){for(int i=0;i<3;++i)e->sequence[i]=(uint8_t)i;
    for(int i=2;i>0;--i){unsigned j=random_prompt(e)%(unsigned)(i+1);
        uint8_t v=e->sequence[i];
        e->sequence[i]=e->sequence[j];
        e->sequence[j]=v;
        }e->bag_index=0;
    }
static void stage(pg_game *g,pg_echo_stage next){
    pg_echo *e=&g->echo;
    e->stage=next;
    e->pointer_armed=false;
    e->impulses=0;
    e->shake_sign=0;
    e->shake_gap=0;
    pg_handling_reset(g);
    if(next==PG_ECHO_PASS||next==PG_ECHO_DONE)pg_react(g,PG_EMOTE_CHEER,PG_SOUND_NONE);
    else if(next==PG_ECHO_RETRY)pg_react(g,PG_EMOTE_OOPS,PG_SOUND_NONE);
    float level=(float)(e->score/5);
    e->response_seconds=fmaxf(1.4f,3-.2f*level);
    e->muted=!pg_audio_enabled(g);
    e->audio_wait=pg_audio_enabled(g)&&g->echo_audio&&(next==PG_ECHO_CUE||next==PG_ECHO_GO);
    e->timer=next==PG_ECHO_CUE?fmaxf(.35f,.6f-.05f*level):next==PG_ECHO_GO?.18f:next==PG_ECHO_INPUT?e->response_seconds:.4f;
    if(e->audio_wait)e->timer=2;
    if(next==PG_ECHO_INPUT)e->begun=true;
    pg_trace_event(g,0,(int)next);
}
void pg_echo_finish(pg_game *g,bool natural){
    pg_echo *e=&g->echo;
    if(e->stage==PG_ECHO_OFF||e->finalized)return;
    e->finalized=true;
    if(e->score>g->device.echo_best){g->device.echo_best=e->score;
        g->utilities.best_dirty=true;
        }g->utilities.checkpoint=true;
    if(!e->begun)return;
    if(natural&&g->pet.phase==PG_ALIVE){
        uint32_t played=g->pet.games_played[3],won=g->pet.games_won[3],session=g->session_games;
        pg_game_result(g,3,e->score>=5?1:-1);
        if(played==UINT32_MAX)g->pet.games_played[3]=UINT32_MAX;
        if(won==UINT32_MAX)g->pet.games_won[3]=UINT32_MAX;
        if(session==UINT32_MAX)g->session_games=UINT32_MAX;
    }else{if(g->pet.games_played[3]<UINT32_MAX)++g->pet.games_played[3];
        if(g->session_games<UINT32_MAX)++g->session_games;
        g->dirty=true;
        }
}
void pg_echo_cancel(pg_game *g){pg_echo_finish(g,false);
    ++g->echo.session;
    g->echo.stage=PG_ECHO_OFF;
    g->echo.audio_wait=false;
    g->echo.pointer_armed=false;
    g->pointer_down=false;
    pg_handling_reset(g);
    }
static void prompt(pg_game *g,bool advance){
    pg_echo *e=&g->echo;
    if(e->round==UINT32_MAX){pg_echo_finish(g,true);
        stage(g,PG_ECHO_DONE);
        return;
        }
    ++e->round;
    if(advance){if(e->bag_index>=3)bag(e);
        e->prompt=(pg_gesture)e->sequence[e->bag_index++];
        }
    g->pointer_down=false;
    g->sound=PG_SOUND_NONE;
    stage(g,PG_ECHO_CUE);
}
void pg_echo_start(pg_game *g){
    pg_echo_finish(g,false);
    uint32_t session=g->echo.session+1;
    memset(&g->echo,0,sizeof g->echo);
    g->echo.session=session;
    g->echo.prompt_rng=g->pet.identity^(session*0x9e3779b9u);
    if(!g->echo.prompt_rng)g->echo.prompt_rng=1;
    g->echo.attempts=3;
    bag(&g->echo);
    g->screen=PG_ECHO;
    prompt(g,true);
}
void pg_echo_replay(pg_game *g){if(playing(g)&&g->echo.stage==PG_ECHO_CUE)prompt(g,false);
    }
void pg_echo_next(pg_game *g){(void)g;
     /* retained ABI; advancement belongs to the stage timer */}
void pg_echo_audio_done(pg_game *g,uint32_t session,uint32_t round,pg_echo_stage current){
    pg_echo *e=&g->echo;
    if(!playing(g)||!pg_audio_enabled(g)||!e->audio_wait||e->session!=session||e->round!=round||e->stage!=current)return;
    if(current==PG_ECHO_CUE)stage(g,PG_ECHO_GO);
    else if(current==PG_ECHO_GO)stage(g,PG_ECHO_INPUT);
}
void pg_echo_tick(pg_game *g,float dt){
    if(!playing(g))return;
    pg_echo *e=&g->echo;
    e->shake_cooldown=fmaxf(0,e->shake_cooldown-dt);
    if(e->muted==pg_audio_enabled(g)&&(e->stage==PG_ECHO_CUE||e->stage==PG_ECHO_GO||e->stage==PG_ECHO_INPUT)){prompt(g,false);
        return;
        }
    if(e->stage==PG_ECHO_DONE||e->stage==PG_ECHO_OFF)return;
    if(e->stage==PG_ECHO_CUE&&e->shake_cooldown>0)return;
    e->timer=fmaxf(0,e->timer-dt);
    if(e->timer>0)return;
    if(e->audio_wait){g->echo_audio=false;
        prompt(g,false);
        return;
        }
    if(e->stage==PG_ECHO_CUE)stage(g,PG_ECHO_GO);
    else if(e->stage==PG_ECHO_GO)stage(g,PG_ECHO_INPUT);
    else if(e->stage==PG_ECHO_INPUT){if(e->attempts)--e->attempts;
        stage(g,PG_ECHO_RETRY);
        }
    else if(e->stage==PG_ECHO_PASS||e->stage==PG_ECHO_RETRY){if(!e->attempts){pg_echo_finish(g,true);
            stage(g,PG_ECHO_DONE);
            }else prompt(g,true);
        }
}
void pg_echo_gesture(pg_game *g,pg_gesture action){
    pg_echo *e=&g->echo;
    if(!playing(g)||e->stage!=PG_ECHO_INPUT||action<PG_SHAKE||action>PG_TAP)return;
    pg_user_activity(g,PG_ACTIVITY_GESTURE);
    g->player_idle=0;
    if(action!=e->prompt){if(e->attempts)--e->attempts;
        stage(g,PG_ECHO_RETRY);
        return;
        }
    if(e->score<UINT32_MAX)++e->score;
    if(e->score>g->device.echo_best){g->device.echo_best=e->score;
        g->utilities.best_dirty=true;
        }
    stage(g,PG_ECHO_PASS);
}
void pg_echo_shake_key(pg_game *g){if(g->echo.shake_cooldown>0)return;
    pg_handling_event(g,true);
    g->echo.shake_cooldown=.8f;
    }
void pg_echo_accel(pg_game *g,float x,float y,float z,float dt){
    if(!isfinite(dt)||dt<=0||dt>.2f)return;
    g->synthetic_imu_us+=(uint64_t)(dt*1000000);
    pg_imu_sample s={.accel={x,y,z},.at_us=g->synthetic_imu_us,.accel_valid=true};
    pg_handling_sample(g,&s);
}
void pg_echo_pointer(pg_game *g,int kind,float x,float y) {
    pg_echo *e=&g->echo;
    if(!pg_ui_visible(x,y)){if(kind!=0){g->pointer_down=false;
            e->pointer_armed=false;
            }return;
        }
    if(kind==0){
        if(g->pointer_down||(x-233)*(x-233)+(y-233)*(y-233)>233*233)return;
        g->pointer_down=true;
        g->pointer_start_x=g->pointer_x=x;
        g->pointer_start_y=g->pointer_y=y;
        g->pointer_time=g->pointer_distance=0;
        g->player_idle=0;
        e->pointer_armed=e->stage==PG_ECHO_INPUT&&y>=120&&y<=330;
        e->pointer_round=e->round;
        return;
    }
    if(!g->pointer_down)return;
    g->pointer_distance+=hypotf(x-g->pointer_x,y-g->pointer_y);
    g->pointer_x=x;
    g->pointer_y=y;
    if(kind!=2)return;
    g->pointer_down=false;
    float distance=hypotf(x-g->pointer_start_x,y-g->pointer_start_y);
    bool short_tap=g->pointer_time<=0.5f&&g->pointer_distance<12;
    if(short_tap&&g->pointer_start_y>=340&&y>=340){pg_tap(g,(int)x,(int)y);
        return;
        }
    if(!e->pointer_armed||e->pointer_round!=e->round||e->stage!=PG_ECHO_INPUT)return;
    e->pointer_armed=false;
    if(distance>=64&&g->pointer_time<=0.8f)pg_echo_gesture(g,PG_SWIPE);
    else if(short_tap&&hypotf(x-233,y-238)<=90&&hypotf(g->pointer_start_x-233,g->pointer_start_y-238)<=90)
        pg_echo_gesture(g,PG_TAP);
}
