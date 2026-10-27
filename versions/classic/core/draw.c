#include "draw.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>

int pg_frame_damage(const pg_canvas *previous,const pg_canvas *next,pg_damage *areas,size_t capacity){
    if(!previous||!next||!previous->pixels||!next->pixels||!areas||capacity<PG_DAMAGE_CAP||
       previous->width!=466||previous->height!=466||next->width!=466||next->height!=466)return -1;
    int count=0;
    for(int y=0;y<466;y+=16){
        int end=y+16<466?y+16:466,left=466,right=-1;
        for(int row=y;row<end;++row){
            const uint16_t *src=next->pixels+row*466,*dst=previous->pixels+row*466;
            if(!memcmp(src,dst,466*2))continue;
            int a=0,b=466;
            while(a+16<=b&&!memcmp(src+a,dst+a,32))a+=16;
            while(b-16>=a&&!memcmp(src+b-16,dst+b-16,32))b-=16;
            --b;
            if(a<left){left=a;}if(b>right){right=b;}
        }
        if(right>=left)areas[count++]=(pg_damage){left,y,right,end-1};
    }
    return count;
}
uint16_t pg_rgb(uint32_t n) { return (uint16_t)(((n >> 8) & 0xf800) | ((n >> 5) & 0x7e0) | ((n >> 3) & 0x1f)); }
void pg_pixel(pg_canvas *c, int x, int y, uint32_t color) {
    if ((unsigned)x < (unsigned)c->width && (unsigned)y < (unsigned)c->height)
        c->pixels[y * c->width + x] = pg_rgb(color);
}
void pg_rect(pg_canvas *c, int x, int y, int w, int h, uint32_t color) {
    int x1 = x + w, y1 = y + h;
    x = x < 0 ? 0 : x; y = y < 0 ? 0 : y;
    x1 = x1 > c->width ? c->width : x1; y1 = y1 > c->height ? c->height : y1;
    if(x>=x1||y>=y1)return;
    uint16_t v = pg_rgb(color);
    for(int j=y;j<y1;++j)
        for(int i=x;i<x1;++i)c->pixels[j*c->width+i]=v;
}
void pg_ellipse(pg_canvas *c, int x, int y, int rx, int ry, uint32_t color) {
    if (rx <= 0 || ry <= 0) return;
    for (int dy = -ry; dy <= ry; ++dy) {
        float a = 1.0f - (float)(dy * dy) / (float)(ry * ry);
        int dx = (int)(rx * sqrtf(a>0?a:0));
        pg_rect(c, x - dx, y + dy, dx * 2 + 1, 1, color);
    }
}
void pg_roundrect(pg_canvas *c, int x, int y, int w, int h, int r, uint32_t color) {
    r = r > w / 2 ? w / 2 : r; r = r > h / 2 ? h / 2 : r;
    if (r <= 0) { pg_rect(c, x, y, w, h, color); return; }
    pg_rect(c, x + r, y, w - r * 2, h, color);
    pg_rect(c, x, y + r, w, h - r * 2, color);
    pg_ellipse(c, x + r, y + r, r, r, color);
    pg_ellipse(c, x + w - r - 1, y + r, r, r, color);
    pg_ellipse(c, x + r, y + h - r - 1, r, r, color);
    pg_ellipse(c, x + w - r - 1, y + h - r - 1, r, r, color);
}
void pg_line(pg_canvas *c, int x0, int y0, int x1, int y1, int width, uint32_t color) {
    int dx = abs(x1 - x0), sx = x0 < x1 ? 1 : -1;
    int dy = -abs(y1 - y0), sy = y0 < y1 ? 1 : -1, err = dx + dy;
    for (;;) {
        pg_rect(c, x0 - width / 2, y0 - width / 2, width, width, color);
        if (x0 == x1 && y0 == y1) break;
        int e = err * 2;
        if (e >= dy) { err += dy; x0 += sx; }
        if (e <= dx) { err += dx; y0 += sy; }
    }
}

/* Original compact bitmap alphabet: five columns, seven bits per glyph. */
static const char alphabet[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789.,!?'-:/+%()<>=";
static const uint8_t glyphs[][5] = {
 {126,9,9,9,126},{127,73,73,73,54},{62,65,65,65,34},{127,65,65,34,28},
 {127,73,73,73,65},{127,9,9,9,1},{62,65,73,73,122},{127,8,8,8,127},
 {65,65,127,65,65},{32,64,65,63,1},{127,8,20,34,65},{127,64,64,64,64},
 {127,2,12,2,127},{127,4,8,16,127},{62,65,65,65,62},{127,9,9,9,6},
 {62,65,81,33,94},{127,9,25,41,70},{70,73,73,73,49},{1,1,127,1,1},
 {63,64,64,64,63},{31,32,64,32,31},{63,64,56,64,63},{99,20,8,20,99},
 {3,4,120,4,3},{97,81,73,69,67},
 {62,81,73,69,62},{0,66,127,64,0},{98,81,73,73,70},{34,65,73,73,54},
 {24,20,18,127,16},{39,69,69,69,57},{60,74,73,73,48},{1,113,9,5,3},
 {54,73,73,73,54},{6,73,73,41,30},
 {0,96,96,0,0},{0,64,32,0,0},{0,0,95,0,0},{2,1,81,9,6},{0,0,3,0,0},
 {8,8,8,8,8},{0,54,54,0,0},{32,16,8,4,2},{8,8,62,8,8},{99,19,8,100,99},
 {0,28,34,65,0},{0,65,34,28,0},{8,20,34,65,0},{0,65,34,20,8},{20,20,20,20,20}
};
void pg_text(pg_canvas *c, int x, int y, const char *text, int scale, uint32_t color) {
    if (scale < 1 || scale > 8) return;
    for (const char *p = text; *p; ++p, x += scale * 6) {
        char ch = *p >= 'a' && *p <= 'z' ? (char)(*p - 'a' + 'A') : *p;
        const char *pos = strchr(alphabet, ch);
        if (!pos) continue;
        size_t idx = (size_t)(pos - alphabet);
        for (int col = 0; col < 5; ++col)
            for (int row = 0; row < 7; ++row)
                if (glyphs[idx][col] & (1u << row)) pg_rect(c, x + col * scale, y + row * scale, scale, scale, color);
    }
}
void pg_center(pg_canvas *c, int x, int y, const char *text, int scale, uint32_t color) {
    pg_text(c, x - ((int)strlen(text) * 6 - 1) * scale / 2, y, text, scale, color);
}
void pg_wrapped(pg_canvas *c, int x, int y, const char *text, int max_chars, int scale, uint32_t color) {
    char line[96];
    if (max_chars < 1) return;
    if (max_chars > 90) max_chars = 90;
    while (*text) {
        int n = (int)strlen(text), take = n < max_chars ? n : max_chars;
        if (n > max_chars) for (int i = take; i > 0; --i) if (text[i] == ' ') { take = i; break; }
        memcpy(line, text, (size_t)take); line[take] = 0;
        pg_center(c, x, y, line, scale, color);
        text += take; while (*text == ' ') ++text;
        y += scale * 10;
    }
}
void pg_star(pg_canvas *c, int x, int y, int s, uint32_t color) {
    pg_line(c, x, y - s, x, y + s, s > 5 ? 3 : 1, color);
    pg_line(c, x - s, y, x + s, y, s > 5 ? 3 : 1, color);
    if (s > 5) pg_rect(c, x - 2, y - 2, 5, 5, color);
}
void pg_heart(pg_canvas *c, int x, int y, int s, uint32_t color) {
    pg_ellipse(c, x - s / 3, y - s / 4, s / 2, s / 2, color);
    pg_ellipse(c, x + s / 3, y - s / 4, s / 2, s / 2, color);
    for (int i = 0; i < s; ++i) pg_rect(c, x - s + i, y + i / 2, 2 * (s - i) + 1, 1, color);
}
