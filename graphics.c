
#include <time.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>

#include "graphics.h"
#include "memory.h"
#include "tinybit.h"

uint8_t fillColor[2] = { 0x00, 0x00 };
uint8_t strokeColor[2] = { 0x00, 0x00 };
int strokeWidth = 0;

#define MAX_POLYGON_POINTS 32
typedef struct {
    int x, y;
} Point;

static Point polygon_points[MAX_POLYGON_POINTS];
static int polygon_point_count = 0;

static const int sin_table[] = {
    0, 1143, 2287, 3429, 4571, 5711, 6850, 7986, 9120, 10252, 11380,
    12504, 13625, 14742, 15854, 16961, 18064, 19160, 20251, 21336, 22414,
    23486, 24550, 25606, 26655, 27696, 28729, 29752, 30767, 31772, 32768,
    33753, 34728, 35693, 36647, 37589, 38521, 39440, 40347, 41243, 42125,
    42995, 43852, 44695, 45525, 46340, 47142, 47929, 48702, 49460, 50203,
    50931, 51643, 52339, 53019, 53683, 54331, 54963, 55577, 56175, 56755,
    57319, 57864, 58393, 58903, 59395, 59870, 60326, 60763, 61183, 61583,
    61965, 62328, 62672, 62997, 63302, 63589, 63856, 64103, 64331, 64540,
    64729, 64898, 65047, 65176, 65286, 65376, 65446, 65496, 65526, 65536,
};

// Fast sine approximation using lookup table
int fast_sin(int angle) {
    angle = angle % 360;
    if (angle < 0) angle += 360;

    if (angle <= 90) return sin_table[angle];
    else if (angle <= 180) return sin_table[180 - angle];
    else if (angle <= 270) return -sin_table[angle - 180];
    else return -sin_table[360 - angle];
}

// Fast cosine approximation using sine lookup table
int fast_cos(int angle) {
    return fast_sin(angle + 90);
}

// Alpha blend foreground pixel with background pixel
void blend(uint8_t* result_bytes, uint8_t* fg, uint8_t* bg) {
    // Extract from byte format: byte[0]=rrrrgggg, byte[1]=bbbbaaaa
    uint8_t fg_r = fg[0] & 0xF0;           // upper 4 bits of byte 0
    uint8_t fg_g = (fg[0] & 0x0F) << 4;    // lower 4 bits of byte 0, shifted up
    uint8_t fg_b = fg[1] & 0xF0;           // upper 4 bits of byte 1  
    uint8_t fg_a = (fg[1] & 0x0F) << 4;    // lower 4 bits of byte 1, shifted up

    if (fg_a == 0xF0) {  // fully opaque
        result_bytes[0] = fg[0];
        result_bytes[1] = fg[1];
        return;
    }
    if (fg_a == 0x00) {  // fully transparent
        return;
    }

    uint8_t bg_r = bg[0] & 0xF0;
    uint8_t bg_g = (bg[0] & 0x0F) << 4;
    uint8_t bg_b = bg[1] & 0xF0;
    uint8_t bg_a = (bg[1] & 0x0F) << 4;

    uint8_t inverse_alpha = 0xF0 - fg_a;

    uint8_t out_r = (fg_r * fg_a + bg_r * inverse_alpha) >> 8;
    uint8_t out_g = (fg_g * fg_a + bg_g * inverse_alpha) >> 8;
    uint8_t out_b = (fg_b * fg_a + bg_b * inverse_alpha) >> 8;
    uint8_t out_a = fg_a + ((bg_a * inverse_alpha) >> 8);

    // Pack back to byte format: byte[0]=rrrrgggg, byte[1]=bbbbaaaa
    result_bytes[0] = (out_r & 0xF0) | ((out_g >> 4) & 0x0F);
    result_bytes[1] = (out_b & 0xF0) | ((out_a >> 4) & 0x0F);
}

// Generate random integer within specified range
int random_range(int min, int max) {
    return min + rand() / (RAND_MAX / (max - min + 1) + 1);
}

// Draw a sprite from spritesheet to display with scaling and clipping
void draw_sprite(int sourceX, int sourceY, int sourceW, int sourceH, int targetX, int targetY, int targetW, int targetH) {
    int clipStartX = targetX < 0 ? -targetX : 0;
    int clipStartY = targetY < 0 ? -targetY : 0;
    int clipEndX = (targetX + targetW > TB_SCREEN_WIDTH) ? TB_SCREEN_WIDTH - targetX : targetW;
    int clipEndY = (targetY + targetH > TB_SCREEN_HEIGHT) ? TB_SCREEN_HEIGHT - targetY : targetH;

    if (clipStartX >= clipEndX || clipStartY >= clipEndY) return;

    int scale_x_fixed_point = (sourceW << 16) / targetW;
    int scale_y_fixed_point = (sourceH << 16) / targetH;

    uint8_t* dst = &tinybit_memory->display[((targetY + clipStartY) * TB_SCREEN_WIDTH + targetX + clipStartX) * 2];

    for (int y = clipStartY; y < clipEndY; ++y) {
        int sourcePixelY = sourceY + ((y * scale_y_fixed_point) >> 16);
        uint8_t* src_row = &tinybit_memory->spritesheet[sourcePixelY * TB_SCREEN_WIDTH * 2];

        for (int x = clipStartX; x < clipEndX; ++x) {
            int sourcePixelX = sourceX + ((x * scale_x_fixed_point) >> 16);
            uint8_t* src_pixel = src_row + sourcePixelX * 2;
            blend(dst, src_pixel, dst);
            dst += 2;
        }
        dst += (TB_SCREEN_WIDTH - (clipEndX - clipStartX)) * 2;
    }
}

// Draw a rectangle with optional stroke and fill
void draw_rect(int x, int y, int w, int h) {
    int clipX = x < 0 ? 0 : x;
    int clipY = y < 0 ? 0 : y;
    int clipW = (x + w > TB_SCREEN_WIDTH) ? TB_SCREEN_WIDTH - x : w;
    int clipH = (y + h > TB_SCREEN_HEIGHT) ? TB_SCREEN_HEIGHT - y : h;

    if (clipX >= TB_SCREEN_WIDTH || clipY >= TB_SCREEN_HEIGHT || clipW <= 0 || clipH <= 0) return;

    uint8_t* display = tinybit_memory->display;

    if (strokeWidth > 0) {
        for (int i = 0; i < strokeWidth && i < clipH; i++) {
            for (int j = 0; j < clipW; j++) {
                uint8_t* top_pixel = &display[((clipY + i) * TB_SCREEN_WIDTH + clipX + j) * 2];
                uint8_t* bottom_pixel = &display[((clipY + clipH - 1 - i) * TB_SCREEN_WIDTH + clipX + j) * 2];

                blend(top_pixel, strokeColor, top_pixel);
                if (clipH - 1 - i != i) {
                    blend(bottom_pixel, strokeColor, bottom_pixel);
                }
            }
        }

        for (int j = strokeWidth; j < clipH - strokeWidth; j++) {
            for (int i = 0; i < strokeWidth && i < clipW; i++) {
                uint8_t* left_pixel = &display[((clipY + j) * TB_SCREEN_WIDTH + clipX + i) * 2];
                uint8_t* right_pixel = &display[((clipY + j) * TB_SCREEN_WIDTH + clipX + clipW - 1 - i) * 2];

                blend(left_pixel, strokeColor, left_pixel);
                if (clipW - 1 - i != i) {
                    blend(right_pixel, strokeColor, right_pixel);
                }
            }
        }
    }

    int fillStartX = strokeWidth;
    int fillStartY = strokeWidth;
    int fillW = clipW - 2 * strokeWidth;
    int fillH = clipH - 2 * strokeWidth;

    if (fillW > 0 && fillH > 0) {
        for (int j = 0; j < fillH; j++) {
            for (int i = 0; i < fillW; i++) {
                uint8_t* pixel = &display[((clipY + fillStartY + j) * TB_SCREEN_WIDTH + clipX + fillStartX + i) * 2];
                blend(pixel, fillColor, pixel);
            }
        }
    }
}

// Draw an oval with optional stroke and fill
void draw_oval(int x, int y, int w, int h) {
    int rx = w >> 1;
    int ry = h >> 1;
    int rx2 = rx * rx;
    int ry2 = ry * ry;

    int strokeRx = rx - strokeWidth;
    int strokeRy = ry - strokeWidth;
    int strokeRx2 = strokeRx * strokeRx;
    int strokeRy2 = strokeRy * strokeRy;

    uint8_t* display = tinybit_memory->display;

    for (int j = 0; j < h; j++) {
        int dy = j - ry;
        int dy2 = dy * dy;

        for (int i = 0; i < w; i++) {
            int px = x + i;
            int py = y + j;

            if (px < 0 || px >= TB_SCREEN_WIDTH || py < 0 || py >= TB_SCREEN_HEIGHT) continue;

            int dx = i - rx;
            int dx2 = dx * dx;

            int outer_test = dx2 * ry2 + dy2 * rx2;

            if (outer_test <= rx2 * ry2) {
                uint8_t* pixel = &display[(py * TB_SCREEN_WIDTH + px) * 2];

                if (strokeWidth > 0 && strokeRx > 0 && strokeRy > 0) {
                    int inner_test = dx2 * strokeRy2 + dy2 * strokeRx2;
                    if (inner_test > strokeRx2 * strokeRy2) {
                        blend(pixel, strokeColor, pixel);
                    }
                    else {
                        blend(pixel, fillColor, pixel);
                    }
                }
                else {
                    blend(pixel, fillColor, pixel);
                }
            }
        }
    }
}

// Set stroke color and width for drawing operations
void set_stroke(int width, int r, int g, int b, int a) {
    strokeWidth = width >= 0 ? width : 0;
    strokeColor[0] = (r & 0xF0) | ((g >> 4) & 0x0F);
    strokeColor[1] = (b & 0xF0) | ((a >> 4) & 0x0F);
}

// Set fill color for drawing operations
void set_fill(int r, int g, int b, int a) {
    fillColor[0] = (r & 0xF0) | ((g >> 4) & 0x0F);
    fillColor[1] = (b & 0xF0) | ((a >> 4) & 0x0F);
}

// Draw a single pixel at specified coordinates
void draw_pixel(int x, int y) {
    if (x < 0 || x >= TB_SCREEN_WIDTH || y < 0 || y >= TB_SCREEN_HEIGHT) {
        return;
    }
    uint8_t* pixel = &tinybit_memory->display[(y * TB_SCREEN_WIDTH + x) * 2];
    blend(pixel, fillColor, pixel);
}

// Draw a line between two points using Bresenham's algorithm
void draw_line(int x1, int y1, int x2, int y2) {
    if (strokeWidth <= 0) return;
    
    int dx = abs(x2 - x1);
    int dy = abs(y2 - y1);
    int sx = x1 < x2 ? 1 : -1;
    int sy = y1 < y2 ? 1 : -1;
    int err = dx - dy;

    int x = x1;
    int y = y1;

    while (1) {
        if (strokeWidth == 1) {
            if (x >= 0 && x < TB_SCREEN_WIDTH && y >= 0 && y < TB_SCREEN_HEIGHT) {
                uint8_t* pixel = &tinybit_memory->display[(y * TB_SCREEN_WIDTH + x) * 2];
                blend(pixel, strokeColor, pixel);
            }
        } else {
            int radius = strokeWidth >> 1;
            for (int dy = -radius; dy <= radius; dy++) {
                for (int dx = -radius; dx <= radius; dx++) {
                    int px = x + dx;
                    int py = y + dy;
                    if (px >= 0 && px < TB_SCREEN_WIDTH && py >= 0 && py < TB_SCREEN_HEIGHT) {
                        uint8_t* pixel = &tinybit_memory->display[(py * TB_SCREEN_WIDTH + px) * 2];
                        blend(pixel, strokeColor, pixel);
                    }
                }
            }
        }

        if (x == x2 && y == y2) break;

        int e2 = 2 * err;
        if (e2 > -dy) {
            err -= dy;
            x += sx;
        }
        if (e2 < dx) {
            err += dx;
            y += sy;
        }
    }
}

// Draw a rotated sprite from spritesheet to display with scaling and clipping
void draw_sprite_rotated(int sourceX, int sourceY, int sourceW, int sourceH, int targetX, int targetY, int targetW, int targetH, int angleDegrees) {
    int cosA = fast_cos(angleDegrees);
    int sinA = fast_sin(angleDegrees);

    int centerX = targetW >> 1;
    int centerY = targetH >> 1;

    // Calculate expanded bounds for rotated sprite to prevent clipping
    int absCosSin = (abs(cosA) + abs(sinA)) >> 16;
    if (absCosSin == 0) absCosSin = 1;
    
    int expandedW = (targetW * absCosSin) + targetW;
    int expandedH = (targetH * absCosSin) + targetH;
    
    int expandX = (expandedW - targetW) >> 1;
    int expandY = (expandedH - targetH) >> 1;

    int clipStartX = (targetX - expandX) < 0 ? -(targetX - expandX) : 0;
    int clipStartY = (targetY - expandY) < 0 ? -(targetY - expandY) : 0;
    int clipEndX = (targetX - expandX + expandedW > TB_SCREEN_WIDTH) ? TB_SCREEN_WIDTH - (targetX - expandX) : expandedW;
    int clipEndY = (targetY - expandY + expandedH > TB_SCREEN_HEIGHT) ? TB_SCREEN_HEIGHT - (targetY - expandY) : expandedH;

    if (clipStartX >= clipEndX || clipStartY >= clipEndY) return;

    int scale_x_fixed_point = (sourceW << 16) / targetW;
    int scale_y_fixed_point = (sourceH << 16) / targetH;

    for (int y = clipStartY; y < clipEndY; ++y) {
        for (int x = clipStartX; x < clipEndX; ++x) {
            int screenX = targetX - expandX + x;
            int screenY = targetY - expandY + y;
            
            if (screenX < 0 || screenX >= TB_SCREEN_WIDTH || screenY < 0 || screenY >= TB_SCREEN_HEIGHT) continue;

            int relX = x - expandX - centerX;
            int relY = y - expandY - centerY;

            int rotX = ((relX * cosA + relY * sinA) >> 16) + centerX;
            int rotY = ((-relX * sinA + relY * cosA) >> 16) + centerY;

            if (rotX >= 0 && rotX < targetW && rotY >= 0 && rotY < targetH) {
                int sourcePixelX = sourceX + ((rotX * scale_x_fixed_point) >> 16);
                int sourcePixelY = sourceY + ((rotY * scale_y_fixed_point) >> 16);

                if (sourcePixelX >= sourceX && sourcePixelX < sourceX + sourceW &&
                    sourcePixelY >= sourceY && sourcePixelY < sourceY + sourceH) {

                    uint8_t* src = &tinybit_memory->spritesheet[(sourcePixelY * TB_SCREEN_WIDTH + sourcePixelX) * 2];
                    uint8_t* dst = &tinybit_memory->display[(screenY * TB_SCREEN_WIDTH + screenX) * 2];
                    blend(dst, src, dst);
                }
            }
        }
    }
}

// Clear the display buffer (set all pixels to black/transparent)
void draw_cls() {
    memset(&tinybit_memory->display, 0, TB_MEM_DISPLAY_SIZE);
}

// Add a point to the polygon vertex list
void poly_add(int x, int y) {
    if (polygon_point_count < MAX_POLYGON_POINTS) {
        polygon_points[polygon_point_count].x = x;
        polygon_points[polygon_point_count].y = y;
        polygon_point_count++;
    }
}

// Clear the polygon vertex list
void poly_clear() {
    polygon_point_count = 0;
}

// Draw a filled polygon using the current vertex list with optional stroke
void draw_polygon() {
    if (polygon_point_count < 3) return;

    int minY = polygon_points[0].y;
    int maxY = polygon_points[0].y;
    
    for (int i = 1; i < polygon_point_count; i++) {
        if (polygon_points[i].y < minY) minY = polygon_points[i].y;
        if (polygon_points[i].y > maxY) maxY = polygon_points[i].y;
    }

    if (minY >= TB_SCREEN_HEIGHT || maxY < 0) return;

    minY = minY < 0 ? 0 : minY;
    maxY = maxY >= TB_SCREEN_HEIGHT ? TB_SCREEN_HEIGHT - 1 : maxY;

    for (int y = minY; y <= maxY; y++) {
        int intersections[MAX_POLYGON_POINTS];
        int intersectionCount = 0;

        for (int i = 0; i < polygon_point_count; i++) {
            int j = (i + 1) % polygon_point_count;
            int y1 = polygon_points[i].y;
            int y2 = polygon_points[j].y;
            
            if ((y1 <= y && y2 > y) || (y2 <= y && y1 > y)) {
                int x1 = polygon_points[i].x;
                int x2 = polygon_points[j].x;
                int x = x1 + ((y - y1) * (x2 - x1)) / (y2 - y1);
                intersections[intersectionCount++] = x;
            }
        }

        for (int i = 0; i < intersectionCount - 1; i++) {
            for (int j = i + 1; j < intersectionCount; j++) {
                if (intersections[i] > intersections[j]) {
                    int temp = intersections[i];
                    intersections[i] = intersections[j];
                    intersections[j] = temp;
                }
            }
        }

        for (int i = 0; i < intersectionCount; i += 2) {
            if (i + 1 < intersectionCount) {
                int x1 = intersections[i];
                int x2 = intersections[i + 1];
                
                x1 = x1 < 0 ? 0 : x1;
                x2 = x2 >= TB_SCREEN_WIDTH ? TB_SCREEN_WIDTH - 1 : x2;
                
                for (int x = x1; x <= x2; x++) {
                    uint8_t* pixel = &tinybit_memory->display[(y * TB_SCREEN_WIDTH + x) * 2];
                    blend(pixel, fillColor, pixel);
                }
            }
        }
    }

    if (strokeWidth > 0) {
        for (int i = 0; i < polygon_point_count; i++) {
            int j = (i + 1) % polygon_point_count;
            draw_line(polygon_points[i].x, polygon_points[i].y, 
                     polygon_points[j].x, polygon_points[j].y);
        }
    }
}