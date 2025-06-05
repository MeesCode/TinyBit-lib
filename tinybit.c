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
int frame_timer = 0;
lua_State* L;

pngle_t *pngle;

void decode_pixel(pngle_t *pngle, uint32_t x, uint32_t y, uint32_t w, uint32_t h, uint8_t rgba[4])
{
    if (!rgba || !tinybit_memory) {
        return; // Safety check for null pointers
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

    // increment cartridge index
    cartridge_index++;
}

void tinybit_init(struct TinyBitMemory* memory, uint8_t* button_state_ptr) {
    if (!memory || !button_state_ptr) {
        return; // Error: null pointer
    }

    tinybit_memory = memory;
    button_state = button_state_ptr;

    pngle = pngle_new();
    pngle_set_draw_callback(pngle, decode_pixel);

    // initialize memory
    memory_init();
    
    // set up lua VM
    L = luaL_newstate();
    lua_setup(L);
}

bool tinybit_feed_cartridge(uint8_t* buffer, size_t size){
    return pngle_feed(pngle, buffer, size) != -2; // -2 means error
}

bool tinybit_start(){

    pngle_destroy(pngle);

     // load lua file
    if (luaL_dostring(L, tinybit_memory->script) == LUA_OK) {
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