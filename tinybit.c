#include "tinybit.h"

#include <inttypes.h>
#include <stdbool.h>
#include <string.h>

#include "lua_functions.h"
#include "graphics.h"
#include "memory.h"
#include "audio.h"
#include "input.h"
#include "font.h"

#include "lua/lua.h"
#include "lua/lualib.h"
#include "lua/lauxlib.h"

#include "pngle/pngle.h"

struct TinyBitMemory* tinybit_memory;

size_t cartridge_index = 0; // index for cartridge buffer
//uint8_t source_buffer[TB_CARTRIDGE_WIDTH * TB_CARTRIDGE_HEIGHT * 4 - TB_SCREEN_WIDTH * TB_SCREEN_HEIGHT * 4]; // max source size
char source_buffer[4096];
int frame_timer = 0;
lua_State* L;

pngle_t *pngle;

void decode_pixel(pngle_t *pngle, uint32_t x, uint32_t y, uint32_t w, uint32_t h, uint8_t rgba[4])
{
    // spritesheet data
    if (cartridge_index < TB_SCREEN_WIDTH * TB_SCREEN_HEIGHT * 4) {
        tinybit_memory->spritesheet[cartridge_index] = (rgba[0] & 0x3) << 6 | (rgba[1] & 0x3) << 4 | (rgba[2] & 0x3) << 2 | (rgba[3] & 0x3) << 0;
    }

    // source code
    else if (cartridge_index - TB_SCREEN_WIDTH * TB_SCREEN_HEIGHT * 4 < TB_CARTRIDGE_WIDTH * TB_CARTRIDGE_HEIGHT * 4) {
        if(cartridge_index - TB_SCREEN_WIDTH * TB_SCREEN_HEIGHT * 4 < sizeof(source_buffer)) {
            source_buffer[cartridge_index - TB_SCREEN_WIDTH * TB_SCREEN_HEIGHT * 4] = (rgba[0] & 0x3) << 6 | (rgba[1] & 0x3) << 4 | (rgba[2] & 0x3) << 2 | (rgba[3] & 0x3) << 0;
        }
    }

    // increment cartridge index
    cartridge_index++;
}

void tinybit_init(struct TinyBitMemory* memory, uint8_t* bs) {

    tinybit_memory = memory;
    button_state = bs;

    pngle = pngle_new();
    pngle_set_draw_callback(pngle, decode_pixel);

    // initialize memory
    memory_init();
    
    // set up lua VM
    L = luaL_newstate();
    lua_setup(L);
}

bool tinybit_feed_catridge(uint8_t* buffer, size_t size){
    return pngle_feed(pngle, buffer, size) != -2; // -2 means error
}

bool tinybit_start(){

    pngle_destroy(pngle);

     // load lua file
    if (luaL_dostring(L, source_buffer) == LUA_OK) {
        lua_pop(L, lua_gettop(L));
    } else{
        return false; // error in lua code
    }

    return true;
}

bool tinybit_frame() {
    // perform lua draw function every frame
    lua_getglobal(L, "_draw");
    if (lua_pcall(L, 0, 1, 0) == LUA_OK) {
        lua_pop(L, lua_gettop(L));
    } else {
        return false; // runtime error in lua code
    }

    // save current button state
    save_button_state();

    return true;
}