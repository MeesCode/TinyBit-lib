#include "cartridge.h"

#include <string.h>

int (*gamecount_func)();
void (*gameload_func)(int index);

#include "lua/lua.h"
#include "lua/lualib.h"
#include "lua/lauxlib.h"

#include "pngle/pngle.h"
#include "tinybit.h"
#include "memory.h"

static size_t cartridge_index = 0;
static pngle_t *pngle;
static int pending_gameload = -1;

// Decode PNG pixel data and load game assets (spritesheet and script) into memory
static void decode_pixel_load_game(pngle_t *pngle, uint32_t x, uint32_t y, uint32_t w, uint32_t h, uint8_t rgba[4])
{
    if (!rgba || !tinybit_memory) {
        return;
    }

    uint8_t decoded = (rgba[0] & 0x3) << 6 | (rgba[1] & 0x3) << 4 | (rgba[2] & 0x3) << 2 | (rgba[3] & 0x3) << 0;

    // spritesheet data (byte-level access for steganography decoding)
    size_t spritesheet_bytes = sizeof(tinybit_memory->spritesheet);
    if (cartridge_index < spritesheet_bytes) {
        ((uint8_t*)tinybit_memory->spritesheet)[cartridge_index] = decoded;
    }
    // source code
    else {
        size_t script_offset = cartridge_index - spritesheet_bytes;
        if (script_offset < TB_MEM_SCRIPT_SIZE) {
            tinybit_memory->script[script_offset] = decoded;
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
        size_t pixel_offset = (y - TB_COVER_Y) * TB_SCREEN_WIDTH + (x - TB_COVER_X);
        if (pixel_offset < TB_MEM_DISPLAY_SIZE) {
            uint8_t rg = (rgba[0] & 0xF0) | ((rgba[1] >> 4) & 0x0F);
            uint8_t ba = (rgba[2] & 0xF0) | ((rgba[3] >> 4) & 0x0F);
            tinybit_memory->spritesheet[pixel_offset] = (uint16_t)rg | ((uint16_t)ba << 8);
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

    pending_gameload = luaL_checkinteger(L, 1);
    return 0;
}

bool cartridge_load_pending(void) {
    if (pending_gameload < 0) {
        return false;
    }

    int index = pending_gameload;
    pending_gameload = -1;

    pngle_reset(pngle);
    pngle_set_draw_callback(pngle, decode_pixel_load_game);
    gameload_func(index);
    tinybit_restart();
    return true;
}

void cartridge_init(void) {
    pngle = pngle_init(tinybit_memory->pngle_data, TB_MEM_PNGLE_SIZE);
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