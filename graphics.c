
#include <time.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>


#include "graphics.h"
#include "memory.h"
#include "tinybit.h"

uint16_t fillColor = 0;
uint16_t strokeColor = 0;
int strokeWidth = 0;

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

int fast_sin(int angle) {
    angle = angle % 360;
    if (angle < 0) angle += 360;
    
    if (angle <= 90) return sin_table[angle];
    else if (angle <= 180) return sin_table[180 - angle];
    else if (angle <= 270) return -sin_table[angle - 180];
    else return -sin_table[360 - angle];
}

int fast_cos(int angle) {
    return fast_sin(angle + 90);
}

uint16_t rgb8888_to_rgb4444(uint32_t color) {
    uint8_t r = (color >> 24) & 0xFF;
    uint8_t g = (color >> 16) & 0xFF;
    uint8_t b = (color >> 8) & 0xFF;
    uint8_t a = color & 0xFF;
    
    // Convert RGBA to BARG format: 0xBARG
    return ((b >> 4) & 0xF) << 12 | ((a >> 4) & 0xF) << 8 | ((r >> 4) & 0xF) << 4 | ((g >> 4) & 0xF);
}

uint32_t rgb4444_to_rgb8888(uint16_t color) {
    // Extract from BARG format: 0xBARG
    uint32_t b4 = (color >> 12) & 0xF;
    uint32_t a4 = (color >> 8) & 0xF;
    uint32_t r4 = (color >> 4) & 0xF;
    uint32_t g4 = color & 0xF;
    
    uint32_t r = r4 << 4;
    uint32_t g = g4 << 4;
    uint32_t b = b4 << 4;
    uint32_t a = a4 << 4;
    
    return (r & 0xFF) << 24 | (g & 0xFF) << 16 | (b & 0xFF) << 8 | (a & 0xFF);
}

void blend(uint16_t* result, uint16_t fg, uint16_t bg) {
    // Extract alpha from BARG format (bits 11-8)
    uint8_t fg_a = (fg >> 8) & 0xF;
    
    if (fg_a == 0xF) {
        *result = fg;
        return;
    }
    if (fg_a == 0) {
        return;
    }
    
    // Extract BARG components
    uint8_t fg_b = (fg >> 12) & 0xF;
    uint8_t fg_r = (fg >> 4) & 0xF;
    uint8_t fg_g = fg & 0xF;
    
    uint8_t bg_b = (bg >> 12) & 0xF;
    uint8_t bg_a = (bg >> 8) & 0xF;
    uint8_t bg_r = (bg >> 4) & 0xF;
    uint8_t bg_g = bg & 0xF;
    
    uint8_t inv_alpha = 0xF - fg_a;
    
    uint8_t out_b = (fg_b * fg_a + bg_b * inv_alpha) >> 4;
    uint8_t out_a = fg_a + ((bg_a * inv_alpha) >> 4);
    uint8_t out_r = (fg_r * fg_a + bg_r * inv_alpha) >> 4;
    uint8_t out_g = (fg_g * fg_a + bg_g * inv_alpha) >> 4;
    
    // Pack back into BARG format
    *result = (out_b << 12) | (out_a << 8) | (out_r << 4) | out_g;
}

int millis() {
    return clock() / (CLOCKS_PER_SEC / 1000);
}

int random_range(int min, int max) {
    return min + rand() / (RAND_MAX / (max - min + 1) + 1);
}

void draw_sprite(int sourceX, int sourceY, int sourceW, int sourceH, int targetX, int targetY, int targetW, int targetH) {
    int clipStartX = targetX < 0 ? -targetX : 0;
    int clipStartY = targetY < 0 ? -targetY : 0;
    int clipEndX = (targetX + targetW > TB_SCREEN_WIDTH) ? TB_SCREEN_WIDTH - targetX : targetW;
    int clipEndY = (targetY + targetH > TB_SCREEN_HEIGHT) ? TB_SCREEN_HEIGHT - targetY : targetH;
    
    if (clipStartX >= clipEndX || clipStartY >= clipEndY) return;
    
    int scaleX_fixed = (sourceW << 16) / targetW;
    int scaleY_fixed = (sourceH << 16) / targetH;
    
    uint16_t* dst = (uint16_t*)&tinybit_memory->display[((targetY + clipStartY) * TB_SCREEN_WIDTH + targetX + clipStartX) * 2];
    
    for (int y = clipStartY; y < clipEndY; ++y) {
        int sourcePixelY = sourceY + ((y * scaleY_fixed) >> 16);
        uint16_t* src_row = (uint16_t*)&tinybit_memory->spritesheet[sourcePixelY * TB_SCREEN_WIDTH * 2];
        
        for (int x = clipStartX; x < clipEndX; ++x) {
            int sourcePixelX = sourceX + ((x * scaleX_fixed) >> 16);
            uint16_t srcColor = src_row[sourcePixelX];
            
            if (((srcColor >> 8) & 0xF) > 0) {
                blend(dst, srcColor, *dst);
            }
            dst++;
        }
        dst += TB_SCREEN_WIDTH - (clipEndX - clipStartX);
    }
}

void draw_rect(int x, int y, int w, int h) {
    int clipX = x < 0 ? 0 : x;
    int clipY = y < 0 ? 0 : y;
    int clipW = (x + w > TB_SCREEN_WIDTH) ? TB_SCREEN_WIDTH - x : w;
    int clipH = (y + h > TB_SCREEN_HEIGHT) ? TB_SCREEN_HEIGHT - y : h;
    
    if (clipX >= TB_SCREEN_WIDTH || clipY >= TB_SCREEN_HEIGHT || clipW <= 0 || clipH <= 0) return;
    
    uint16_t* display = (uint16_t*)&tinybit_memory->display;
    
    if (strokeWidth > 0) {
        for (int i = 0; i < strokeWidth && i < clipH; i++) {
            uint16_t* top_row = &display[(clipY + i) * TB_SCREEN_WIDTH + clipX];
            uint16_t* bottom_row = &display[(clipY + clipH - 1 - i) * TB_SCREEN_WIDTH + clipX];
            
            for (int j = 0; j < clipW; j++) {
                blend(&top_row[j], strokeColor, top_row[j]);
                if (clipH - 1 - i != i) {
                    blend(&bottom_row[j], strokeColor, bottom_row[j]);
                }
            }
        }
        
        for (int j = strokeWidth; j < clipH - strokeWidth; j++) {
            for (int i = 0; i < strokeWidth && i < clipW; i++) {
                uint16_t* left_pixel = &display[(clipY + j) * TB_SCREEN_WIDTH + clipX + i];
                uint16_t* right_pixel = &display[(clipY + j) * TB_SCREEN_WIDTH + clipX + clipW - 1 - i];
                
                blend(left_pixel, strokeColor, *left_pixel);
                if (clipW - 1 - i != i) {
                    blend(right_pixel, strokeColor, *right_pixel);
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
            uint16_t* row = &display[(clipY + fillStartY + j) * TB_SCREEN_WIDTH + clipX + fillStartX];
            for (int i = 0; i < fillW; i++) {
                blend(&row[i], fillColor, row[i]);
            }
        }
    }
}

// draw an oval with outline, specified by x, y, w, h
void draw_oval(int x, int y, int w, int h) {
    int rx = w >> 1;
    int ry = h >> 1;
    int rx2 = rx * rx;
    int ry2 = ry * ry;
    
    int strokeRx = rx - strokeWidth;
    int strokeRy = ry - strokeWidth;
    int strokeRx2 = strokeRx * strokeRx;
    int strokeRy2 = strokeRy * strokeRy;
    
    uint16_t* display = (uint16_t*)&tinybit_memory->display;
    
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
                uint16_t* pixel = &display[py * TB_SCREEN_WIDTH + px];
                
                if (strokeWidth > 0 && strokeRx > 0 && strokeRy > 0) {
                    int inner_test = dx2 * strokeRy2 + dy2 * strokeRx2;
                    if (inner_test > strokeRx2 * strokeRy2) {
                        blend(pixel, strokeColor, *pixel);
                    } else {
                        blend(pixel, fillColor, *pixel);
                    }
                } else {
                    blend(pixel, fillColor, *pixel);
                }
            }
        }
    }
}


void set_stroke(int width, int r, int g, int b, int a) {
    strokeWidth = width >= 0 ? width : 0;
    uint32_t color32 = (r & 0xFF) << 24 | (g & 0xFF) << 16 | (b & 0xFF) << 8 | (a & 0xFF);
    strokeColor = rgb8888_to_rgb4444(color32);
}

void set_fill(int r, int g, int b, int a) {
    uint32_t color32 = (r & 0xFF) << 24 | (g & 0xFF) << 16 | (b & 0xFF) << 8 | (a & 0xFF);
    fillColor = rgb8888_to_rgb4444(color32);
}

void draw_pixel(int x, int y) {
    if(x < 0 || x >= TB_SCREEN_WIDTH || y < 0 || y >= TB_SCREEN_HEIGHT) {
        return;
    }
    uint16_t* pixel = (uint16_t*)&tinybit_memory->display[(y * TB_SCREEN_WIDTH + x) * 2];
    blend(pixel, fillColor, *pixel);
}

void draw_sprite_rotated(int sourceX, int sourceY, int sourceW, int sourceH, int targetX, int targetY, int targetW, int targetH, int angleDegrees) {
    int cosA = fast_cos(angleDegrees);
    int sinA = fast_sin(angleDegrees);
    
    int centerX = targetW >> 1;
    int centerY = targetH >> 1;
    
    int clipStartX = targetX < 0 ? -targetX : 0;
    int clipStartY = targetY < 0 ? -targetY : 0;
    int clipEndX = (targetX + targetW > TB_SCREEN_WIDTH) ? TB_SCREEN_WIDTH - targetX : targetW;
    int clipEndY = (targetY + targetH > TB_SCREEN_HEIGHT) ? TB_SCREEN_HEIGHT - targetY : targetH;
    
    if (clipStartX >= clipEndX || clipStartY >= clipEndY) return;
    
    int scaleX_fixed = (sourceW << 16) / targetW;
    int scaleY_fixed = (sourceH << 16) / targetH;
    
    for (int y = clipStartY; y < clipEndY; ++y) {
        for (int x = clipStartX; x < clipEndX; ++x) {
            int relX = x - centerX;
            int relY = y - centerY;
            
            int rotX = ((relX * cosA - relY * sinA) >> 16) + centerX;
            int rotY = ((relX * sinA + relY * cosA) >> 16) + centerY;
            
            if (rotX >= 0 && rotX < targetW && rotY >= 0 && rotY < targetH) {
                int sourcePixelX = sourceX + ((rotX * scaleX_fixed) >> 16);
                int sourcePixelY = sourceY + ((rotY * scaleY_fixed) >> 16);
                
                if (sourcePixelX >= sourceX && sourcePixelX < sourceX + sourceW &&
                    sourcePixelY >= sourceY && sourcePixelY < sourceY + sourceH) {
                    
                    uint16_t* src = (uint16_t*)&tinybit_memory->spritesheet[(sourcePixelY * TB_SCREEN_WIDTH + sourcePixelX) * 2];
                    uint16_t srcColor = *src;
                    
                    if (((srcColor >> 8) & 0xF) > 0) {
                        uint16_t* dst = (uint16_t*)&tinybit_memory->display[((targetY + y) * TB_SCREEN_WIDTH + targetX + x) * 2];
                        blend(dst, srcColor, *dst);
                    }
                }
            }
        }
    }
}

void draw_cls() {
    memset(&tinybit_memory->display, 0, TB_MEM_DISPLAY_SIZE);
}