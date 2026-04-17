#include "cartridge.h"

#include <stdio.h>
#include <string.h>
#include <time.h>

int (*gamecount_func)();
void (*gameload_func)(int index);

#include "lua/lua.h"
#include "lua/lualib.h"
#include "lua/lauxlib.h"

#include "pngle/pngle.h"
#include "tinybit.h"
#include "memory.h"
#include "lua_functions.h"  // extern log_func

static size_t cartridge_index = 0;
static pngle_t *pngle;
static int pending_gameload = -1;

static uint8_t header_bytes[TB_HEADER_SIZE];
static struct TinyBitHeader header;
static bool header_parsed = false;

static uint16_t read_u16_le(const uint8_t* p) {
    return (uint16_t)p[0] | ((uint16_t)p[1] << 8);
}

static uint32_t read_u32_le(const uint8_t* p) {
    return (uint32_t)p[0]
         | ((uint32_t)p[1] << 8)
         | ((uint32_t)p[2] << 16)
         | ((uint32_t)p[3] << 24);
}

static void log_line(const char* s) {
    if (log_func) {
        log_func(s);
    }
}

static void parse_and_log_header(void) {
    header.format_version = read_u16_le(&header_bytes[0]);
    header.flags          = read_u16_le(&header_bytes[2]);
    header.script_size    = read_u32_le(&header_bytes[4]);
    header.checksum       = read_u32_le(&header_bytes[8]);
    memcpy(header.title,   &header_bytes[12], TB_HEADER_TITLE_SIZE);
    memcpy(header.author,  &header_bytes[76], TB_HEADER_AUTHOR_SIZE);
    header.title[TB_HEADER_TITLE_SIZE - 1]   = '\0';
    header.author[TB_HEADER_AUTHOR_SIZE - 1] = '\0';
    header.game_version = read_u16_le(&header_bytes[140]);
    header.package_date = read_u32_le(&header_bytes[142]);

    if (!log_func) return;

    time_t ts = (time_t)header.package_date;
    struct tm* tm_info = gmtime(&ts);
    char date_str[32];
    if (tm_info == NULL || strftime(date_str, sizeof(date_str), "%Y-%m-%d %H:%M:%S UTC", tm_info) == 0) {
        date_str[0] = '\0';
    }

    char line[160];
    log_line("[TinyBit] Cartridge header:\n");
    snprintf(line, sizeof(line), "  title:          \"%s\"\n", header.title);           log_line(line);
    snprintf(line, sizeof(line), "  author:         \"%s\"\n", header.author);          log_line(line);
    snprintf(line, sizeof(line), "  game version:   %u\n", header.game_version);        log_line(line);
    snprintf(line, sizeof(line), "  package date:   %u (%s)\n", header.package_date, date_str); log_line(line);
    snprintf(line, sizeof(line), "  format version: %u\n", header.format_version);      log_line(line);
    snprintf(line, sizeof(line), "  flags:          0x%04x\n", header.flags);           log_line(line);
    snprintf(line, sizeof(line), "  script size:    %u bytes\n", header.script_size);   log_line(line);
    snprintf(line, sizeof(line), "  checksum:       0x%08x (crc32)\n", header.checksum); log_line(line);
}

// Decode PNG pixel data and load game assets (header, spritesheet, script) into memory
static void decode_pixel_load_game(pngle_t *pngle, uint32_t x, uint32_t y, uint32_t w, uint32_t h, uint8_t rgba[4])
{
    if (!rgba || !tinybit_memory) {
        return;
    }

    uint8_t decoded = (rgba[0] & 0x3) << 6 | (rgba[1] & 0x3) << 4 | (rgba[2] & 0x3) << 2 | (rgba[3] & 0x3) << 0;

    // header (first TB_HEADER_SIZE pixels)
    if (cartridge_index < TB_HEADER_SIZE) {
        header_bytes[cartridge_index] = decoded;
        cartridge_index++;
        if (cartridge_index == TB_HEADER_SIZE && !header_parsed) {
            parse_and_log_header();
            header_parsed = true;
        }
        return;
    }

    size_t payload_index = cartridge_index - TB_HEADER_SIZE;
    size_t spritesheet_bytes = sizeof(tinybit_memory->spritesheet);

    // spritesheet data (byte-level access for steganography decoding)
    if (payload_index < spritesheet_bytes) {
        ((uint8_t*)tinybit_memory->spritesheet)[payload_index] = decoded;
    }
    // source code
    else {
        size_t script_offset = payload_index - spritesheet_bytes;
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

    cartridge_reset();
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

    cartridge_reset();
    pngle_reset(pngle);
    pngle_set_draw_callback(pngle, decode_pixel_load_game);
    gameload_func(index);
    tinybit_restart();
    return true;
}

void cartridge_init(void) {
    pngle = pngle_init(tinybit_memory->pngle_data, TB_MEM_PNGLE_SIZE);
    pngle_set_draw_callback(pngle, decode_pixel_load_game);
    cartridge_reset();
}

void cartridge_destroy(void) {
    pngle_destroy(pngle);
}

void cartridge_reset(void) {
    cartridge_index = 0;
    header_parsed = false;
    memset(header_bytes, 0, sizeof(header_bytes));
    memset(&header, 0, sizeof(header));
}

bool cartridge_feed(const uint8_t* buffer, size_t size) {
    return pngle_feed(pngle, buffer, size) != -2;
}

const struct TinyBitHeader* cartridge_header(void) {
    return header_parsed ? &header : NULL;
}

void cartridge_register_lua(lua_State* L) {
    lua_pushcfunction(L, lua_gamecount);
    lua_setglobal(L, "gamecount");
    lua_pushcfunction(L, lua_gamecover);
    lua_setglobal(L, "gamecover");
    lua_pushcfunction(L, lua_gameload);
    lua_setglobal(L, "gameload");
}
