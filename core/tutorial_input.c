#include "tutorial_input.h"
#include "layout.h"
#include <math.h>

void pg_tutorial_pointer(pg_game *g,pg_tutorial *t,int kind,float x,float y,bool auto_commit){
    if(g->standby||t->commit!=PG_TUTORIAL_IDLE)return;
    if(kind==3||!isfinite(x)||!isfinite(y)||!pg_ui_visible(x,y)){
        if(kind!=0)g->top_pull=g->lamp_touch=false;
        return;
    }
    if(t->committed==PG_TUTORIAL_SWIPE){
        if(kind==0&&y<=160){g->top_pull=true;g->pointer_start_x=x;g->pointer_start_y=y;}
        else if(kind==2&&g->top_pull){
            float dx=x-g->pointer_start_x,dy=y-g->pointer_start_y;g->top_pull=false;
            if(dy>=42&&fabsf(dx)<=64&&dy>fabsf(dx)*1.2f&&
               pg_tutorial_action_received(t,PG_TUTORIAL_SWIPE_ACCEPTED)){
                g->top_hud_seconds=2;
                if(auto_commit)(void)pg_tutorial_resolve_commit(t,PG_TUTORIAL_COMMIT_OK);
            }
        }
    }else if(t->committed==PG_TUTORIAL_LAMP_NIGHT||t->committed==PG_TUTORIAL_LAMP_DAY){
        if(kind==0&&pg_ui_hit(pg_ui_lamp,(int)x,(int)y)){
            g->lamp_touch=true;g->pointer_start_x=x;g->pointer_start_y=y;
        }else if(kind==2&&g->lamp_touch){
            bool hit=pg_ui_hit(pg_ui_lamp,(int)x,(int)y)&&hypotf(x-g->pointer_start_x,y-g->pointer_start_y)<20;
            g->lamp_touch=false;
            if(hit){
                pg_tutorial_action action=t->committed==PG_TUTORIAL_LAMP_NIGHT?PG_TUTORIAL_LAMP_NIGHT_ACCEPTED:PG_TUTORIAL_LAMP_DAY_ACCEPTED;
                if(pg_tutorial_action_received(t,action)&&auto_commit&&pg_tutorial_resolve_commit(t,PG_TUTORIAL_COMMIT_OK))
                    g->sleeping=t->committed==PG_TUTORIAL_LAMP_DAY;
            }
        }
    }
}
