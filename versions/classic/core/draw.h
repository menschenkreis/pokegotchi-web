#ifndef PG_DRAW_H
#define PG_DRAW_H
#include "game.h"
#include "tutorial.h"

typedef struct { uint16_t *pixels; int width, height; } pg_canvas;
enum { PG_DAMAGE_CAP=30 };
typedef struct {int x1,y1,x2,y2;} pg_damage;
/* Compare immutable frames; returned inclusive bands cover every changed pixel. */
int pg_frame_damage(const pg_canvas *previous,const pg_canvas *next,pg_damage *areas,size_t capacity);
uint16_t pg_rgb(uint32_t hex);
void pg_pixel(pg_canvas *c, int x, int y, uint32_t color);
void pg_rect(pg_canvas *c, int x, int y, int w, int h, uint32_t color);
void pg_line(pg_canvas *c, int x0, int y0, int x1, int y1, int width, uint32_t color);
void pg_ellipse(pg_canvas *c, int x, int y, int rx, int ry, uint32_t color);
void pg_roundrect(pg_canvas *c, int x, int y, int w, int h, int radius, uint32_t color);
void pg_text(pg_canvas *c, int x, int y, const char *text, int scale, uint32_t color);
void pg_center(pg_canvas *c, int x, int y, const char *text, int scale, uint32_t color);
void pg_wrapped(pg_canvas *c, int x, int y, const char *text, int max_chars, int scale, uint32_t color);
void pg_star(pg_canvas *c, int x, int y, int size, uint32_t color);
void pg_heart(pg_canvas *c, int x, int y, int size, uint32_t color);
void pg_creature(pg_canvas *c, const pg_game *g, int x, int feet_y, int size);
/* Interface layer before the physical round mask; useful for compositing/audits. */
void pg_render_interface(pg_canvas *c, const pg_game *g);
void pg_render(pg_canvas *c, const pg_game *g);
/* Disposable tutorial preview: callers must not treat this as persisted game state. */
void pg_render_tutorial_preview(pg_canvas *c, const pg_game *g, const pg_tutorial *tutorial);
#endif
