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

int (*gamecount_func)();
void (*gamecover_func)(int index);

void decode_pixel_load_game(pngle_t *pngle, uint32_t x, uint32_t y, uint32_t w, uint32_t h, uint8_t rgba[4])
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

void decode_pixel_load_cover(pngle_t *pngle, uint32_t x, uint32_t y, uint32_t w, uint32_t h, uint8_t rgba[4])
{
    // printf("[TinyBit] decode_pixel_load_cover: x=%" PRIu32 ", y=%" PRIu32 ", w=%" PRIu32 ", h=%" PRIu32 "\n", x, y, w, h);
    if (!rgba || !tinybit_memory) {
        return; // Safety check for null pointers
    }

    // write to spritesheet buffer
    if(x >= 35 && x < 35 + TB_SCREEN_WIDTH && y >= 34 && y < 34 + TB_SCREEN_HEIGHT) {
        size_t display_offset = ((y - 34) * TB_SCREEN_WIDTH + (x - 35)) * 2;
        if (display_offset < TB_MEM_DISPLAY_SIZE) {
            tinybit_memory->spritesheet[display_offset] = (rgba[0] & 0xF0) | (rgba[1] & 0xF0) >> 4; // first byte
            tinybit_memory->spritesheet[display_offset + 1] = (rgba[2] & 0xF0) | (rgba[3] & 0xF0) >> 4; // second byte
        }
    }
}

void tinybit_init(struct TinyBitMemory* memory, uint8_t* button_state_ptr) {
    if (!memory || !button_state_ptr) {
        return; // Error: null pointer
    }

    tinybit_memory = memory;
    button_state = button_state_ptr;

    pngle = pngle_new();
    pngle_set_draw_callback(pngle, decode_pixel_load_game);

    // initialize memory
    memory_init();
    
    // set up lua VM
    L = luaL_newstate();
    lua_setup(L);
}

int lua_gamecount(lua_State* L) {
    int count = gamecount_func();
    lua_pushinteger(L, count);
    return 1;
}

int lua_gamecover(lua_State* L) {
    if (lua_gettop(L) != 1) {
        lua_pushstring(L, "Expected 1 argument");
        return lua_error(L);
    }

    int index = luaL_checkinteger(L, 1);
    size_t pointer = 0; // Assuming pointer is not used in this context

    printf("Loading game cover for index: %d\n", index);
    gamecover_func(index);

    return 0;
}

void tinybit_start_ui(){
    if(!tinybit_memory || !button_state || !gamecount_func || !gamecover_func) {
        return; // Error: null pointer
    }

    pngle_set_draw_callback(pngle, decode_pixel_load_cover);

    // // add special functions to lua for reading game files
    lua_pushcfunction(L, lua_gamecount);
    lua_setglobal(L, "gamecount");
    lua_pushcfunction(L, lua_gamecover);
    lua_setglobal(L, "gamecover");

    printf("Script\n");
    const char* s = 
        "log(\"[Lua] Hello from this string\")\n"
        "counter = 0\n"
        "log(\"game count: \" .. counter)\n"
        "gamecover(0)\n"
        "frame_time = 0\n"
        "log(\"[Lua] Game cover loaded\")\n"
        "function _draw()\n"
        "    -- draw the spritesheet\n"
        "    sprite(0,0,128,128,0,0,128,128)\n"
        "    if millis() - frame_time > 1000 then\n"
        "        frame_time = millis()\n"
        "        counter = (counter + 1) % gamecount()\n"
        "        gamecover(counter % gamecount())\n"
        "        log(\"[Lua] Game cover updated\")\n"
        "    end\n"
        "end\n"
        "log(\"[Lua] _draw function defined\")\n";

    // load lua file
    if (luaL_dostring(L, s) == LUA_OK) {
        lua_pop(L, lua_gettop(L));
    } else {
        printf("Error in lua code: %s\n", lua_tostring(L, -1));
    }

    // pngle_destroy(pngle);

    printf("Exit start function...\n");

}

bool tinybit_feed_cartridge(uint8_t* buffer, size_t size){
    printf("[TinyBit] Feed cartridge with %zu bytes\n", size);
    int ret = pngle_feed(pngle, buffer, size) != -2; // -2 means error
    if (!ret) {
        printf("[TinyBit] Error feeding cartridge data to pngle\n");
        return false; // error in pngle feed
    }
    printf("[TinyBit] Successfully fed cartridge data to pngle\n");
    pngle_reset(pngle);

    return true;
}

void tinybit_log_cb(void (*log_func_ptr)(const char*)){
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

void tinybit_gamecount_cb(int (*gamecount_func_ptr)()) {
    if (!gamecount_func_ptr) {
        return; // Error: null pointer
    }
    
    gamecount_func = gamecount_func_ptr;
}

void tinybit_gamecover_cb(void (*gamecover_func_ptr)(int index)) {
    if (!gamecover_func_ptr) {
        return; // Error: null pointer
    }
    
    gamecover_func = gamecover_func_ptr;
}