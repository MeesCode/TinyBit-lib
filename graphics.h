#ifndef graphics_H
#define graphics_H

#include <stdint.h>

struct Color{
    uint8_t r, g, b, a;
};

extern uint8_t fillColor[2];
extern uint8_t strokeColor[2];

extern int strokeWidth;

int random_range(int, int);
void draw_sprite(int sourceX, int sourceY, int sourceW, int sourceH, int targetX, int targetY, int targetW, int targetH);
void draw_sprite_rotated(int sourceX, int sourceY, int sourceW, int sourceH, int targetX, int targetY, int targetW, int targetH, int angleDegrees);
void draw_rect(int x, int y, int w, int h);
void draw_oval(int x, int y, int w, int h);
void set_stroke(int width, int r, int g, int b, int a);
void set_fill(int r, int g, int b, int a);
void draw_pixel(int x, int y);
void draw_line(int x1, int y1, int x2, int y2);
void draw_cls();
void poly_add(int x, int y);
void poly_clear();
void draw_polygon();

#endif