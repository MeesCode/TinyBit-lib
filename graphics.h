#ifndef GRAPHICS_H
#define GRAPHICS_H

#include <stdint.h>

struct Color{
    uint8_t r, g, b, a;
};

typedef enum {
	TARGET_MEMORY,
    TARGET_DISPLAY,
    TARGET_SPRITESHEET
} TARGET;

// Pixel format: uint16_t where low byte = RRRRGGGG, high byte = BBBBAAAA
extern uint16_t fillColor;
extern uint16_t strokeColor;

extern int strokeWidth;

// Pack RGBA components (8-bit each, upper 4 bits used) into a RGBA4444 pixel
static inline uint16_t pack_color(int r, int g, int b, int a) {
    uint8_t rg = (r & 0xF0) | ((g >> 4) & 0x0F);
    uint8_t ba = (b & 0xF0) | ((a >> 4) & 0x0F);
    return (uint16_t)rg | ((uint16_t)ba << 8);
}

// Graphics function declarations
int random_range(int, int);
void draw_sprite(int sourceX, int sourceY, int sourceW, int sourceH, int targetX, int targetY, int targetW, int targetH, TARGET target);
void draw_sprite_rotated(int sourceX, int sourceY, int sourceW, int sourceH, int targetX, int targetY, int targetW, int targetH, int angleDegrees, TARGET target);
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
void blend(uint16_t* dst, uint16_t fg);

#endif
