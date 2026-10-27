#ifndef PG_LAYOUT_H
#define PG_LAYOUT_H
#include <stdbool.h>
/* Shared physical geometry: artwork and touch routing use the same controls. */
enum { PG_UI_CENTER=233, PG_UI_RADIUS=233, PG_UI_SAFE_RADIUS=221 };
typedef struct {int x,y,w,h;} pg_ui_rect;
static const int pg_ui_dock[4][2]={{113,365},{193,395},{273,395},{353,365}};
static const pg_ui_rect pg_ui_lamp={83,187,57,61};
static const pg_ui_rect pg_ui_back={169,389,128,44};
static const pg_ui_rect pg_ui_page_previous={125,366,56,56};
static const pg_ui_rect pg_ui_page_next={285,366,56,56};
static const pg_ui_rect pg_ui_games_home={201,395,64,40};
static inline bool pg_ui_visible(float x,float y){
    if(!(x>=0&&x<466&&y>=0&&y<466))return false;
    float dx=x-PG_UI_CENTER,dy=y-PG_UI_CENTER;
    return dx*dx+dy*dy<=PG_UI_RADIUS*PG_UI_RADIUS;
}
static inline bool pg_ui_hit(pg_ui_rect r,int x,int y){
    return pg_ui_visible((float)x,(float)y)&&x>=r.x&&x<r.x+r.w&&y>=r.y&&y<r.y+r.h;
}
static inline bool pg_ui_dock_hit(int i,int x,int y){
    int dx=x-pg_ui_dock[i][0],dy=y-pg_ui_dock[i][1];
    return dx*dx+dy*dy<=32*32;
}
#endif
