#include "tinybit.h"

#include <inttypes.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

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

#ifdef _POSIX_C_SOURCE
static struct timespec start_time = { 0, 0 };
#else
static clock_t start_time = 0;
#endif

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

void tinybit_set_log(void (*log_func_ptr)(const char*)){
    if (!log_func_ptr) {
        return; // Error: null pointer
    }
    
    log_func = log_func_ptr;
}

bool tinybit_start(){

    pngle_destroy(pngle);

     // load lua file
    if (luaL_dostring(L, (char*)tinybit_memory->script) == LUA_OK) {
        lua_pop(L, lua_gettop(L));
    } else{
        return false; // error in lua code
    }

    return true;
}

long tinybit_get_frame_time() {
    #ifdef _POSIX_C_SOURCE
        struct timespec current_time;
        clock_gettime(CLOCK_MONOTONIC, &current_time);

        if (start_time.tv_sec == 0 && start_time.tv_nsec == 0) {
            start_time = current_time;
            return 0;
        }

        long elapsed_sec = current_time.tv_sec - start_time.tv_sec;
        long elapsed_nsec = current_time.tv_nsec - start_time.tv_nsec;

        return (elapsed_sec * 1000) + (elapsed_nsec / 1000000);
    #else
        clock_t current_time = clock();
        
        if (start_time == 0) {
            start_time = current_time;
            return 0;
        }
        
        return (current_time - start_time) * 1000 / CLOCKS_PER_SEC;
    #endif
}

bool tinybit_frame() {

    frame_time = tinybit_get_frame_time();

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