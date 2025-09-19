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

struct TinyBitMemory* tinybit_memory;
uint8_t* button_state;

size_t cartridge_index = 0; // index for cartridge buffer
int frame_timer = 0;
bool running = true;

lua_State* L;

pngle_t *pngle;

int (*tb_gamecount_func)();
void (*tb_gameload_func)(int index);

void (*tb_frame_func)();
void (*tb_input_func)();
void (*tb_sleep_func)();
int (*tb_get_ticks_ms_func)();
int (*tb_get_ticks_ms_func)();

// Decode PNG pixel data and load game assets (spritesheet and script) into memory
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

// Decode PNG pixel data and load cover image into spritesheet memory
void decode_pixel_load_cover(pngle_t *pngle, uint32_t x, uint32_t y, uint32_t w, uint32_t h, uint8_t rgba[4])
{
    if (!rgba || !tinybit_memory) {
        return; // Safety check for null pointers
    }

    // write to spritesheet buffer
    if(x >= TB_COVER_X && x < TB_COVER_X + TB_SCREEN_WIDTH && y >= TB_COVER_Y && y < TB_COVER_Y + TB_SCREEN_HEIGHT) {
        size_t display_offset = ((y - TB_COVER_Y) * TB_SCREEN_WIDTH + (x - TB_COVER_X)) * 2;
        if (display_offset < TB_MEM_DISPLAY_SIZE) {
            tinybit_memory->spritesheet[display_offset] = (rgba[0] & 0xF0) | (rgba[1] & 0xF0) >> 4; // first byte
            tinybit_memory->spritesheet[display_offset + 1] = (rgba[2] & 0xF0) | (rgba[3] & 0xF0) >> 4; // second byte
        }
    }
}

// Lua function to get the count of available games
int lua_gamecount(lua_State* L) {

    if (!tb_gamecount_func) {
        return lua_error(L);
    }

    int count = tb_gamecount_func();
    lua_pushinteger(L, count);
    return 1;
}

// Lua function to load a game's cover image for preview
int lua_gamecover(lua_State* L) {

    if (!tb_gameload_func) {
        return lua_error(L);
    }

    if (lua_gettop(L) != 1) {
        lua_pushstring(L, "Expected 1 argument");
        return lua_error(L);
    }

    int index = luaL_checkinteger(L, 1);

    pngle_reset(pngle);
    pngle_set_draw_callback(pngle, decode_pixel_load_cover);
    tb_gameload_func(index);

    // the the game loader as default callback
    pngle_set_draw_callback(pngle, decode_pixel_load_game);

    return 0;
}

// Lua function to load and start a game by index
int lua_gameload(lua_State* L) {

    if (!tb_gameload_func) {
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
    tb_gameload_func(index);

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

// Initialize the TinyBit system with memory and input state pointers
void tinybit_init(struct TinyBitMemory* memory, uint8_t* button_state_ptr) {
    if (!memory || !button_state_ptr) {
        return; // Error: null pointer
    }

    tinybit_memory = memory;
    button_state = button_state_ptr;

    pngle = pngle_new();

    // t=set load game as the default callback
    pngle_set_draw_callback(pngle, decode_pixel_load_game);

    // initialize memory
    memory_init();
    
    // set up lua VM
    L = luaL_newstate();
    lua_setup(L);

    // add special functions to lua for reading game files
    lua_pushcfunction(L, lua_gamecount);
    lua_setglobal(L, "gamecount");
    lua_pushcfunction(L, lua_gamecover);
    lua_setglobal(L, "gamecover");
    lua_pushcfunction(L, lua_gameload);
    lua_setglobal(L, "gameload");

    // initialize the game loader as the default "game"
    const char* s = 
        "counter = 0\n"
        "log(\"files found: \" .. gamecount())\n"
        "gamecover(0)\n"
        "function _draw()\n"
        "    -- draw the spritesheet\n"
        "    sprite(0,0,128,128,14,14,100,100)\n"
        "    if btnp(LEFT) then\n"
        "        counter = (counter + 1) % gamecount()\n"
        "        gamecover(counter % gamecount())\n"
        "    end\n"
        "    if btnp(RIGHT) then\n"
        "        counter = (counter - 1) % gamecount()\n"
        "        gamecover(counter % gamecount())\n"
        "    end\n"
        "    if btnp(A) then\n"
        "        gameload(counter)\n"
        "    end\n"
        "end\n";
        
    memcpy(tinybit_memory->script, s, strlen(s) + 1); // copy script to memory
}

// Start executing the Lua script currently loaded in memory
bool tinybit_start(){
    // load lua file
    if (luaL_dostring(L, (char*)tinybit_memory->script) == LUA_OK) {
        lua_pop(L, lua_gettop(L));
        return true; // success
    } 
    else {
        printf("[TinyBit] Lua error: %s\n", lua_tostring(L, -1));
        lua_pop(L, lua_gettop(L)); // pop error message
        return false; // runtime error in lua code
    }
}

// Signal the emulation loop to quit
void tinybit_quit() {
    running = false;
}

// Feed cartridge PNG data to the TinyBit decoder
bool tinybit_feed_cartridge(uint8_t* buffer, size_t size){
    return pngle_feed(pngle, buffer, size) != -2; // -2 means error
}

// Main emulation loop - handles input, executes Lua draw function, and renders frames
void tinybit_loop() {

    while(running){
        frame_time = tb_get_ticks_ms_func();

        // get button input
        tb_input_func();

        // perform lua draw function every frame
        lua_getglobal(L, "_draw");
        if (lua_pcall(L, 0, 1, 0) == LUA_OK) {
            lua_pop(L, lua_gettop(L));
        } else {
            lua_pop(L, lua_gettop(L)); // pop error message
            printf("[TinyBit] Lua error: %s\n", lua_tostring(L, -1));
            break; // runtime error in lua code
        }

        // save current button state
        save_button_state();

        tb_frame_func();

        // cap to ~60fps
        int delay = (16 - (tb_get_ticks_ms_func() - frame_time));
        if (delay > 0) {
            tb_sleep_func(delay);
        }
    }

    lua_close(L);
    L = NULL;

    pngle_destroy(pngle);
    pngle = NULL;
}

// Set callback function for counting available games
void tinybit_gamecount_cb(int (*gamecount_func_ptr)()) {
    if (!gamecount_func_ptr) {
        return; // Error: null pointer
    }
    
    tb_gamecount_func = gamecount_func_ptr;
}

// Set callback function for loading games by index
void tinybit_gameload_cb(void (*gameload_func_ptr)(int index)) {
    if (!gameload_func_ptr) {
        return; // Error: null pointer
    }
    
    tb_gameload_func = gameload_func_ptr;
}

// Set callback function that gets called when a new frame should be drawn
void tinybit_render_cb(void (*render_func_ptr)()) {
    if (!render_func_ptr) {
        return; // Error: null pointer
    }
    
    tb_frame_func = render_func_ptr;
}

// Set callback function that gets called to read button state
void tinybit_poll_input_cb(void (*poll_input_func_ptr)()) {
    if (!poll_input_func_ptr) {
        return; // Error: null pointer
    }
    
    tb_input_func = poll_input_func_ptr;
}

// Set callback function for sleeping/delaying execution
void tinybit_sleep_cb(void (*sleep_func_ptr)(int ms)) {
    if (!sleep_func_ptr) {
        return; // Error: null pointer
    }
    
    tb_sleep_func = sleep_func_ptr;
}

void tinybit_log_cb(void (*log_func_ptr)(const char*)){
    if (!log_func_ptr) {
        return; // Error: null pointer
    }
    
    log_func = log_func_ptr;
}

void tinybit_get_ticks_ms_cb(int (*get_ticks_ms_func_ptr)()) {
    if (!get_ticks_ms_func_ptr) {
        return; // Error: null pointer
    }
    
    tb_get_ticks_ms_func = get_ticks_ms_func_ptr;
}