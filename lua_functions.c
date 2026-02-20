
#include "lua/lua.h"
#include "lua/lualib.h"
#include "lua/lauxlib.h"

#include <string.h>

#include "lua_functions.h"
#include "graphics.h"
#include "memory.h"
#include "font.h"
#include "input.h"
#include "audio.h"
#include "tinybit.h"

char log_buffer[256];
void (*log_func)(const char*);
long frame_time = 0;

// Initialize Lua state with TinyBit libraries and global variables
void lua_setup(lua_State* L) {
    
    // load lua libraries
    static const luaL_Reg loadedlibs[] = {
        {LUA_GNAME, luaopen_base},
        {LUA_COLIBNAME, luaopen_coroutine},
        {LUA_TABLIBNAME, luaopen_table},
        {LUA_STRLIBNAME, luaopen_string},
        {LUA_MATHLIBNAME, luaopen_math},
        {NULL, NULL}
    };

    const luaL_Reg* lib;
    for (lib = loadedlibs; lib->func; lib++) {
        luaL_requiref(L, lib->name, lib->func, 1);
        lua_pop(L, 1);  /* remove lib */
    }

    // set lua waveforms
    lua_pushinteger(L, SINE);
    lua_setglobal(L, "SINE");
    lua_pushinteger(L, SAW);
    lua_setglobal(L, "SAW");
    lua_pushinteger(L, SQUARE);
    lua_setglobal(L, "SQUARE");
    lua_pushinteger(L, NOISE);
    lua_setglobal(L, "NOISE");

    lua_pushinteger(L, TB_SCREEN_WIDTH);
    lua_setglobal(L, "TB_SCREEN_WIDTH");
    lua_pushinteger(L, TB_SCREEN_HEIGHT);
    lua_setglobal(L, "TB_SCREEN_HEIGHT");

    lua_pushinteger(L, TB_BUTTON_A);
	lua_setglobal(L, "A");
	lua_pushinteger(L, TB_BUTTON_B);
	lua_setglobal(L, "B");
	lua_pushinteger(L, TB_BUTTON_UP);
	lua_setglobal(L, "UP");
	lua_pushinteger(L, TB_BUTTON_DOWN);
	lua_setglobal(L, "DOWN");
	lua_pushinteger(L, TB_BUTTON_LEFT);
	lua_setglobal(L, "LEFT");
	lua_pushinteger(L, TB_BUTTON_RIGHT);
	lua_setglobal(L, "RIGHT");
	lua_pushinteger(L, TB_BUTTON_START);
	lua_setglobal(L, "START");
    lua_pushinteger(L, TB_BUTTON_SELECT);
	lua_setglobal(L, "SELECT");

    lua_pushcfunction(L, lua_sprite);
    lua_setglobal(L, "sprite");
    lua_pushcfunction(L, lua_copy_disp);
    lua_setglobal(L, "duplicate");
    lua_pushcfunction(L, lua_line);
    lua_setglobal(L, "line");
    lua_pushcfunction(L, lua_millis);
    lua_setglobal(L, "millis");
    lua_pushcfunction(L, lua_stroke);
    lua_setglobal(L, "stroke");
    lua_pushcfunction(L, lua_fill);
    lua_setglobal(L, "fill");
    lua_pushcfunction(L, lua_rect);
    lua_setglobal(L, "rect");
    lua_pushcfunction(L, lua_oval);
    lua_setglobal(L, "oval");
    lua_pushcfunction(L, lua_bpm);
    lua_setglobal(L, "bpm");
    lua_pushcfunction(L, lua_btn);
    lua_setglobal(L, "btn");
    lua_pushcfunction(L, lua_btnp);
    lua_setglobal(L, "btnp");
    lua_pushcfunction(L, lua_mycopy);
    lua_setglobal(L, "copy");
    lua_pushcfunction(L, lua_cls);
    lua_setglobal(L, "cls");
    lua_pushcfunction(L, lua_peek);
    lua_setglobal(L, "peek");
    lua_pushcfunction(L, lua_poke);
    lua_setglobal(L, "poke");
    lua_pushcfunction(L, lua_random);
    lua_setglobal(L, "random");
    lua_pushcfunction(L, lua_cursor);
    lua_setglobal(L, "cursor");
    lua_pushcfunction(L, lua_print);
    lua_setglobal(L, "print");
    lua_pushcfunction(L, lua_text);
    lua_setglobal(L, "text");
    lua_pushcfunction(L, lua_log);
    lua_setglobal(L, "log");
    lua_pushcfunction(L, lua_poly_add);
    lua_setglobal(L, "poly_add");
    lua_pushcfunction(L, lua_poly_clear);
    lua_setglobal(L, "poly_clear");
    lua_pushcfunction(L, poly);
    lua_setglobal(L, "draw_polygon");
    lua_pushcfunction(L, lua_music);
    lua_setglobal(L, "music");
    lua_pushcfunction(L, lua_sfx);
    lua_setglobal(L, "sfx");
    lua_pushcfunction(L, lua_sfx_active);
    lua_setglobal(L, "sfx_active");
    lua_pushcfunction(L, lua_rgba);
    lua_setglobal(L, "rgba");
    lua_pushcfunction(L, lua_rgb);
    lua_setglobal(L, "rgb");
    lua_pushcfunction(L, lua_hsb);
    lua_setglobal(L, "hsb");
    lua_pushcfunction(L, lua_hsba);
    lua_setglobal(L, "hsba");
}

// Lua function to log messages to the console
int lua_log(lua_State* L) {

    if (log_func == NULL) {
        return 0; // No log function set
    }

    int log_buffer_index = 0;
    int nargs = lua_gettop(L);

    for (int i = 1; i <= nargs; i++) {
        if (lua_isstring(L, i)) {
            const char* str = lua_tostring(L, i);
            size_t str_len = strlen(str);
            size_t written = 0;
            while (written < str_len) {
                size_t space_left = sizeof(log_buffer) - log_buffer_index - 2; // reserve for space and null
                size_t chunk = (str_len - written > space_left) ? space_left : (str_len - written);
                if (chunk > 0) {
                    memcpy(log_buffer + log_buffer_index, str + written, chunk);
                    log_buffer_index += chunk;
                    written += chunk;
                }
                if (written < str_len) {
                    // Buffer full, flush and continue
                    log_buffer[log_buffer_index] = '\0';
                    log_func(log_buffer);
                    log_buffer_index = 0;
                }
            }
            // Always add a space after each argument
            if (log_buffer_index < sizeof(log_buffer) - 2) {
                log_buffer[log_buffer_index++] = ' ';
            } else {
                // Buffer full, flush and add space
                log_buffer[log_buffer_index] = '\0';
                log_func(log_buffer);
                log_buffer_index = 0;
                log_buffer[log_buffer_index++] = ' ';
            }
        }
    }

    // Add newline and flush
    if (log_buffer_index < sizeof(log_buffer) - 1) {
        log_buffer[log_buffer_index++] = '\n';
    } else {
        log_buffer[sizeof(log_buffer) - 2] = '\n';
        log_buffer_index = sizeof(log_buffer) - 1;
    }
    log_buffer[log_buffer_index] = '\0';
    log_func(log_buffer);

    return 0;
}

// Lua function to draw a line between two points
int lua_line(lua_State* L) {
    if (lua_gettop(L) != 4) {
        return 0;
    }

    int sourceX1 = (int)luaL_checknumber(L, 1);
    int sourceY1 = (int)luaL_checknumber(L, 2);
    int sourceX2 = (int)luaL_checknumber(L, 3);
    int sourceY2 = (int)luaL_checknumber(L, 4);

    draw_line(sourceX1, sourceY1, sourceX2, sourceY2);
    return 0;
}

int lua_sprite_copy(lua_State* L, TARGET target) {
    if (lua_gettop(L) != 8 && lua_gettop(L) != 9) {
        return 0;
    }

    int sourceX = (int)luaL_checknumber(L, 1);
    int sourceY = (int)luaL_checknumber(L, 2);
    int sourceW = (int)luaL_checknumber(L, 3);
    int sourceH = (int)luaL_checknumber(L, 4);

    int targetX = (int)luaL_checknumber(L, 5);
    int targetY = (int)luaL_checknumber(L, 6);
    int targetW = (int)luaL_checknumber(L, 7);
    int targetH = (int)luaL_checknumber(L, 8);

    if(lua_gettop(L) == 8) {
        draw_sprite(sourceX, sourceY, sourceW, sourceH, targetX, targetY, targetW, targetH, target);
        return 0;
    }

    int targetR = (int)luaL_checknumber(L, 9);

    draw_sprite_rotated(sourceX, sourceY, sourceW, sourceH, targetX, targetY, targetW, targetH, targetR, target);
    return 0;
}

// Lua function to draw a sprite (with optional rotation)
int lua_sprite(lua_State* L) {
    return lua_sprite_copy(L, TARGET_SPRITESHEET);
}

int lua_copy_disp(lua_State* L) {
    return lua_sprite_copy(L, TARGET_DISPLAY);
}

// Lua function to get current frame time in milliseconds
int lua_millis(lua_State* L) {
    lua_Integer m = frame_time;
    lua_pushinteger(L, m);
    return 1;
}

// Lua function to generate random number within range
int lua_random(lua_State* L) {
    if (lua_gettop(L) != 2) {
        return 0;
    }

    int min = (int)luaL_checknumber(L, 1);
    int max = (int)luaL_checknumber(L, 2);

    lua_pushinteger(L, random_range(min, max));
    
    return 1;
}

// Convert RGBA8888 uint32_t to internal RGBA4444 uint16_t
static uint16_t rgba8888_to_4444(uint32_t color) {
    int r = (color >> 24) & 0xFF;
    int g = (color >> 16) & 0xFF;
    int b = (color >> 8) & 0xFF;
    int a = color & 0xFF;
    return pack_color(r, g, b, a);
}

// Lua function: stroke(width, color) - set stroke width and color
int lua_stroke(lua_State* L) {
    if (lua_gettop(L) != 2) {
        return 0;
    }

    int width = (int)luaL_checknumber(L, 1);
    uint32_t color = (uint32_t)luaL_checknumber(L, 2);

    set_stroke(width, rgba8888_to_4444(color));
    return 0;
}

// Lua function: fill(color) - set fill color
int lua_fill(lua_State* L) {
    if (lua_gettop(L) != 1) {
        return 0;
    }

    uint32_t color = (uint32_t)luaL_checknumber(L, 1);
    set_fill(rgba8888_to_4444(color));
    return 0;
}

// Lua function: text(color) - set text color
int lua_text(lua_State* L) {
    if (lua_gettop(L) != 1) {
        return 0;
    }

    uint32_t color = (uint32_t)luaL_checknumber(L, 1);
    font_text_color(rgba8888_to_4444(color));
    return 0;
}

// Lua function: rgba(r, g, b, a) - pack into RGBA8888
int lua_rgba(lua_State* L) {
    int r = (int)luaL_checknumber(L, 1) & 0xFF;
    int g = (int)luaL_checknumber(L, 2) & 0xFF;
    int b = (int)luaL_checknumber(L, 3) & 0xFF;
    int a = (int)luaL_checknumber(L, 4) & 0xFF;
    uint32_t color = ((uint32_t)r << 24) | ((uint32_t)g << 16) | ((uint32_t)b << 8) | (uint32_t)a;
    lua_pushnumber(L, color);
    return 1;
}

// Lua function: rgb(r, g, b) - pack into RGBA8888 with alpha=255
int lua_rgb(lua_State* L) {
    int r = (int)luaL_checknumber(L, 1) & 0xFF;
    int g = (int)luaL_checknumber(L, 2) & 0xFF;
    int b = (int)luaL_checknumber(L, 3) & 0xFF;
    uint32_t color = ((uint32_t)r << 24) | ((uint32_t)g << 16) | ((uint32_t)b << 8) | 0xFF;
    lua_pushnumber(L, color);
    return 1;
}

// Helper: convert HSB (all 0-255) to RGB (0-255)
static void hsb_to_rgb(float h, float s, float b, int* r, int* g, int* bl) {
    h = h * 360.0f / 255.0f;
    s /= 255.0f;
    b /= 255.0f;

    float c = b * s;
    float hh = h / 60.0;
    float rem = hh - (int)(hh / 2.0) * 2.0;
    float diff = rem - 1.0;
    float x = c * (1.0 - (diff < 0 ? -diff : diff));
    float m = b - c;

    float rr, gg, bb;
    if (h < 60)       { rr = c; gg = x; bb = 0; }
    else if (h < 120) { rr = x; gg = c; bb = 0; }
    else if (h < 180) { rr = 0; gg = c; bb = x; }
    else if (h < 240) { rr = 0; gg = x; bb = c; }
    else if (h < 300) { rr = x; gg = 0; bb = c; }
    else              { rr = c; gg = 0; bb = x; }

    *r = (int)((rr + m) * 255.0 + 0.5);
    *g = (int)((gg + m) * 255.0 + 0.5);
    *bl = (int)((bb + m) * 255.0 + 0.5);
}

// Lua function: hsba(h, s, b, a) - convert HSB + alpha to RGBA8888
int lua_hsba(lua_State* L) {
    float h = luaL_checknumber(L, 1);
    float s = luaL_checknumber(L, 2);
    float b = luaL_checknumber(L, 3);
    int a = (int)luaL_checknumber(L, 4) & 0xFF;

    int r, g, bl;
    hsb_to_rgb(h, s, b, &r, &g, &bl);
    uint32_t color = ((uint32_t)(r & 0xFF) << 24) | ((uint32_t)(g & 0xFF) << 16) | ((uint32_t)(bl & 0xFF) << 8) | (uint32_t)a;
    lua_pushnumber(L, color);
    return 1;
}

// Lua function: hsb(h, s, b) - convert HSB to RGBA8888 with alpha=255
int lua_hsb(lua_State* L) {
    float h = luaL_checknumber(L, 1);
    float s = luaL_checknumber(L, 2);
    float b = luaL_checknumber(L, 3);

    int r, g, bl;
    hsb_to_rgb(h, s, b, &r, &g, &bl);
    uint32_t color = ((uint32_t)(r & 0xFF) << 24) | ((uint32_t)(g & 0xFF) << 16) | ((uint32_t)(bl & 0xFF) << 8) | 0xFF;
    lua_pushnumber(L, color);
    return 1;
}

// Lua function to draw a rectangle
int lua_rect(lua_State* L) {
    if (lua_gettop(L) != 4) {
        return 0;
    }

    int x = (int)luaL_checknumber(L, 1);
    int y = (int)luaL_checknumber(L, 2);
    int w = (int)luaL_checknumber(L, 3);
    int h = (int)luaL_checknumber(L, 4);

    draw_rect(x, y, w, h);
    return 0;
}

// Lua function to draw an oval
int lua_oval(lua_State* L) {
    if (lua_gettop(L) != 4) {
        return 0;
    }

    int x = (int)luaL_checknumber(L, 1);
    int y = (int)luaL_checknumber(L, 2);
    int w = (int)luaL_checknumber(L, 3);
    int h = (int)luaL_checknumber(L, 4);

    draw_oval(x, y, w, h);
    return 0;
}

// Lua function to add a point to polygon vertex list
int lua_poly_add(lua_State* L) {
    if (lua_gettop(L) != 2) {
        return 0;
    }

    int x = (int)luaL_checknumber(L, 1);
    int y = (int)luaL_checknumber(L, 2);

    poly_add(x, y);
    return 0;
}

// Lua function to clear polygon vertex list
int lua_poly_clear(lua_State* L) {
    poly_clear();
    return 0;
}

// Lua function to draw the current polygon
int poly(lua_State* L) {
    draw_polygon();
    return 0;
}

// Lua function to set beats per minute for audio timing
int lua_bpm(lua_State* L) {
    int new_bpm = luaL_checkinteger(L, 1);
    set_bpm(new_bpm);
    return 0;
}

// Lua function to check if button is currently pressed
int lua_btn(lua_State* L) {
    enum TinyBitButton btn = luaL_checkinteger(L, 1);
    lua_pushboolean(L, input_btn(btn));
    return 1;
}

// Lua function to check if button was just pressed this frame
int lua_btnp(lua_State* L) {
    enum TinyBitButton btn = luaL_checkinteger(L, 1);
    lua_pushboolean(L, input_btnp(btn));
    return 1;
}

// Lua function to clear the display
int lua_cls(lua_State* L) {
    draw_cls();
    return 0;
}

// Lua function to copy memory between addresses
int lua_mycopy(lua_State* L) {
    if (lua_gettop(L) != 3) {
        return 0;
    }

    int dst = luaL_checkinteger(L, 1);
    int src = luaL_checkinteger(L, 2);
    int size = luaL_checkinteger(L, 3);

    mem_copy(dst, src, size);
    return 0;
}

// Lua function to read a byte from memory
int lua_peek(lua_State* L) {
    if (lua_gettop(L) != 1) {
        return 0;
    }

    int dst = luaL_checkinteger(L, 1);

    lua_pushinteger(L, mem_peek(dst));
    return 1;
}

// Lua function to write a byte to memory
int lua_poke(lua_State* L) {
    if (lua_gettop(L) != 2) {
        return 0;
    }

    int dst = luaL_checkinteger(L, 1);
    int val = luaL_checkinteger(L, 2);

    mem_poke(dst, val);
    return 0;
}

// Lua function to set text cursor position
int lua_cursor(lua_State* L) {
    if (lua_gettop(L) != 2) {
        return 0;
    }

    int x = (int)luaL_checknumber(L, 1);
    int y = (int)luaL_checknumber(L, 2);

    font_cursor(x, y);
    return 0;
}

// Lua function to print text at cursor position
int lua_print(lua_State* L) {

    if (lua_gettop(L) != 1) {
        return 0;
    }

    const char* str = luaL_checkstring(L, 1);

    font_print(str);
    return 0;
}

// Lua function to play a music track
int lua_music(lua_State* L) {

    if (lua_gettop(L) != 1) {
        return 0;
    }

    const char* str = luaL_checkstring(L, 1);

    audio_load_abc(0, str, SINE, true);
    return 0;
}

// Lua function to play a short sfx
int lua_sfx(lua_State* L) {

    if (lua_gettop(L) != 1) {
        return 0;
    }

    const char* str = luaL_checkstring(L, 1);

    audio_load_abc(1, str, SINE, false);
    return 0;
}

int lua_sfx_active(lua_State* L) {
    lua_pushboolean(L, is_channel_active(1));
    return 1;
}