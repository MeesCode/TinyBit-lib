#include "cartridge.h"

#include <string.h>

#include "lua/lua.h"
#include "lua/lualib.h"
#include "lua/lauxlib.h"

#include "pngle/pngle.h"
#include "tinybit.h"
#include "memory.h"

static size_t cartridge_index = 0;
static pngle_t *pngle;

static int (*gamecount_func)();
static void (*gameload_func)(int index);

// Decode PNG pixel data and load game assets (spritesheet and script) into memory
static void decode_pixel_load_game(pngle_t *pngle, uint32_t x, uint32_t y, uint32_t w, uint32_t h, uint8_t rgba[4])
{
    if (!rgba || !tinybit_memory) {
        return;
    }

    // spritesheet data
    if (cartridge_index < TB_MEM_SPRITESHEET_SIZE) {
        tinybit_memory->spritesheet[cartridge_index] = (rgba[0] & 0x3) << 6 | (rgba[1] & 0x3) << 4 | (rgba[2] & 0x3) << 2 | (rgba[3] & 0x3) << 0;
    }
    // source code
    else {
        size_t script_offset = cartridge_index - TB_MEM_SPRITESHEET_SIZE;
        if (script_offset < TB_MEM_SCRIPT_SIZE) {
            tinybit_memory->script[script_offset] = (rgba[0] & 0x3) << 6 | (rgba[1] & 0x3) << 4 | (rgba[2] & 0x3) << 2 | (rgba[3] & 0x3) << 0;
        }
    }

    cartridge_index++;
}

// Decode PNG pixel data and load cover image into spritesheet memory
static void decode_pixel_load_cover(pngle_t *pngle, uint32_t x, uint32_t y, uint32_t w, uint32_t h, uint8_t rgba[4])
{
    if (!rgba || !tinybit_memory) {
        return;
    }

    if(x >= TB_COVER_X && x < TB_COVER_X + TB_SCREEN_WIDTH && y >= TB_COVER_Y && y < TB_COVER_Y + TB_SCREEN_HEIGHT) {
        size_t display_offset = ((y - TB_COVER_Y) * TB_SCREEN_WIDTH + (x - TB_COVER_X)) * 2;
        if (display_offset < TB_MEM_DISPLAY_SIZE) {
            tinybit_memory->spritesheet[display_offset] = (rgba[0] & 0xF0) | (rgba[1] & 0xF0) >> 4;
            tinybit_memory->spritesheet[display_offset + 1] = (rgba[2] & 0xF0) | (rgba[3] & 0xF0) >> 4;
        }
    }
}

static int lua_gamecount(lua_State* L) {
    if (!gamecount_func) {
        return lua_error(L);
    }

    int count = gamecount_func();
    lua_pushinteger(L, count);
    return 1;
}

static int lua_gamecover(lua_State* L) {
    if (!gameload_func) {
        return lua_error(L);
    }

    if (lua_gettop(L) != 1) {
        lua_pushstring(L, "Expected 1 argument");
        return lua_error(L);
    }

    int index = luaL_checkinteger(L, 1);

    pngle_reset(pngle);
    pngle_set_draw_callback(pngle, decode_pixel_load_cover);
    gameload_func(index);

    pngle_set_draw_callback(pngle, decode_pixel_load_game);

    return 0;
}

static int lua_gameload(lua_State* L) {
    if (!gameload_func) {
        return lua_error(L);
    }

    if (lua_gettop(L) != 1) {
        lua_pushstring(L, "Expected 1 argument");
        return lua_error(L);
    }

    pngle_reset(pngle);
    pngle_set_draw_callback(pngle, decode_pixel_load_game);

    int index = luaL_checkinteger(L, 1);
    gameload_func(index);

    // restart game
    lua_pushnil(L);
    lua_setglobal(L, "gamecount");
    lua_pushnil(L);
    lua_setglobal(L, "gamecover");
    lua_pushnil(L);
    lua_setglobal(L, "gameload");

    tinybit_start();

    return 0;
}

void cartridge_init(void) {
    pngle = pngle_init();
    pngle_set_draw_callback(pngle, decode_pixel_load_game);
}

void cartridge_destroy(void) {
    pngle_destroy(pngle);
}

bool cartridge_feed(const uint8_t* buffer, size_t size) {
    return pngle_feed(pngle, buffer, size) != -2;
}

void cartridge_register_lua(lua_State* L) {
    lua_pushcfunction(L, lua_gamecount);
    lua_setglobal(L, "gamecount");
    lua_pushcfunction(L, lua_gamecover);
    lua_setglobal(L, "gamecover");
    lua_pushcfunction(L, lua_gameload);
    lua_setglobal(L, "gameload");
}

void tinybit_gamecount_cb(int (*gamecount_func_ptr)()) {
    if (!gamecount_func_ptr) {
        return;
    }
    gamecount_func = gamecount_func_ptr;
}

void tinybit_gameload_cb(void (*gameload_func_ptr)(int index)) {
    if (!gameload_func_ptr) {
        return;
    }
    gameload_func = gameload_func_ptr;
}
