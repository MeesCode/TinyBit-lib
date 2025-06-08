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
void (*gameload_func)(int index);

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

    if (!gamecount_func) {
        return lua_error(L);
    }

    int count = gamecount_func();
    lua_pushinteger(L, count);
    return 1;
}

int lua_gamecover(lua_State* L) {

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

    return 0;
}

int lua_gameload(lua_State* L) {

    if (!gameload_func) {
        return lua_error(L);
    }

    if (lua_gettop(L) != 1) {
        lua_pushstring(L, "Expected 1 argument");
        return lua_error(L);
    }

    pngle_reset(pngle);
    pngle_set_draw_callback(pngle, decode_pixel_load_game);
    memset(tinybit_memory, 0, sizeof(struct TinyBitMemory));

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

void tinybit_start_ui(){
    if(!tinybit_memory || !button_state) {
        return; // Error: null pointer
    }

    // // add special functions to lua for reading game files
    lua_pushcfunction(L, lua_gamecount);
    lua_setglobal(L, "gamecount");
    lua_pushcfunction(L, lua_gamecover);
    lua_setglobal(L, "gamecover");
    lua_pushcfunction(L, lua_gameload);
    lua_setglobal(L, "gameload");

    const char* s = 
        "log(\"[Lua] Hello from this string\")\n"
        "counter = 0\n"
        "log(\"files found: \" .. gamecount())\n"
        "gamecover(0)\n"
        "log(\"[Lua] Game cover loaded\")\n"
        "function _draw()\n"
        "    -- draw the spritesheet\n"
        "    sprite(0,0,128,128,0,0,128,128)\n"
        "    if btnp(UP) then\n"
        "        counter = (counter + 1) % gamecount()\n"
        "        gamecover(counter % gamecount())\n"
        "        log(\"[Lua] Game cover updated\")\n"
        "    end\n"
        "    if btnp(DOWN) then\n"
        "        gameload(counter)\n"
        "    end\n"
        "end\n"
        "log(\"[Lua] _draw function defined\")\n";

    memcpy(tinybit_memory->script, s, strlen(s) + 1); // copy script to memory
    tinybit_start();
}

bool tinybit_feed_cartridge(uint8_t* buffer, size_t size){
    return pngle_feed(pngle, buffer, size) != -2; // -2 means error
}

void tinybit_log_cb(void (*log_func_ptr)(const char*)){
    if (!log_func_ptr) {
        return; // Error: null pointer
    }
    
    log_func = log_func_ptr;
}

bool tinybit_start(){

    printf("%s", (char*)tinybit_memory->script);

    // load lua file
    if (luaL_dostring(L, (char*)tinybit_memory->script) == LUA_OK) {
        lua_pop(L, lua_gettop(L));
    } else{
        printf("[TinyBit] Lua error");
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
        lua_pop(L, lua_gettop(L)); // pop error message
        printf("[TinyBit] Lua error: %s\n", lua_tostring(L, -1));
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

void tinybit_gameload_cb(void (*gameload_func_ptr)(int index)) {
    if (!gameload_func_ptr) {
        return; // Error: null pointer
    }
    
    gameload_func = gameload_func_ptr;
}