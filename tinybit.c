#include "tinybit.h"

#include <inttypes.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

#include "lua_functions.h"
#include "lua_pool.h"
#include "cartridge.h"
#include "graphics.h"
#include "memory.h"
#include "audio.h"
#include "input.h"
#include "font.h"

#include "lua/lua.h"
#include "lua/lualib.h"
#include "lua/lauxlib.h"

static bool running = true;

static lua_State* L;

static void (*frame_func)();
static void (*input_func)();
static void (*sleep_func)();
static int (*get_ticks_ms_func)();
static void (*audio_queue_func)();

// Initialize the TinyBit system with memory, input state, and audio buffer pointers
void tinybit_init(struct TinyBitMemory* memory, bool* button_state_ptr) {
    if (!memory || !button_state_ptr) {
        return; // Error: null pointer
    }

    tinybit_memory = memory;

    // initialize memory
    memory_init();
    tb_audio_init();
    cartridge_init();

    // set up lua VM
    L = lua_pool_newstate();

    // add special functions to lua for reading game files
    cartridge_register_lua(L);

    // initialize the game loader as the default "game"
    const char* s =
        "counter = 0\n"
        "game_counter = gamecount()\n"
        "log(\"files found: \" .. game_counter)\n"
        "gamecover(0)\n"
        "function _draw()\n"
        "    -- draw the spritesheet\n"
        "    sprite(0,0,128,128,14,14,100,100)\n"
        "    if btnp(LEFT) then\n"
        "        counter = (counter + 1) % game_counter\n"
        "        gamecover(counter % game_counter)\n"
        "    end\n"
        "    if btnp(RIGHT) then\n"
        "        counter = (counter - 1) % game_counter\n"
        "        gamecover(counter % game_counter)\n"
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
        lua_pop(L, lua_gettop(L)); // pop error message
        return false; // runtime error in lua code
    }
}

// Signal the emulation loop to quit
void tinybit_stop() {
    running = false;

    lua_close(L);
    L = NULL;

    cartridge_destroy();
}

// Feed cartridge PNG data to the TinyBit decoder
bool tinybit_feed_cartridge(const uint8_t* buffer, size_t size){
    return cartridge_feed(buffer, size);
}

// Main emulation loop - handles input, executes Lua draw function, and renders frames
void tinybit_loop() {

    uint32_t start_time;
    uint32_t render_time;
    uint32_t input_time;
    uint32_t display_time;
    uint32_t audio_time;

    frame_time = get_ticks_ms_func();
    start_time = frame_time;

    // INPUT
    input_func();
    input_time = get_ticks_ms_func() - start_time;
    start_time += input_time;

    // LOGIC
    lua_getglobal(L, "_draw");
    if (lua_pcall(L, 0, 1, 0) == LUA_OK) {
        lua_pop(L, lua_gettop(L));
    } else {
        lua_pop(L, lua_gettop(L)); // pop error message
        printf("[TinyBit] Lua error loop: %s\n", lua_tostring(L, -1));
        return; // runtime error in lua code
    }
    render_time = get_ticks_ms_func() - start_time;
    start_time += render_time;

    // save current button state
    save_button_state();

    // AUDIO
    process_audio();
    if (audio_queue_func) {
        audio_queue_func();
    }
    audio_time = get_ticks_ms_func() - start_time;
    start_time += audio_time;

    // RENDER
    if (frame_func) {
        frame_func();
    }
    display_time = get_ticks_ms_func() - start_time;
    start_time += display_time;

    // printf("used memory: %zu bytes\n", lua_pool_get_used());

    // printf("[TinyBit] Frame time: %d ms (render: %d ms, display: %d ms, audio: %d ms)\n", get_ticks_ms_func() - frame_time, render_time, display_time, audio_time);
}

// Set callback function that gets called when a new frame should be drawn
void tinybit_render_cb(void (*render_func_ptr)()) {
    if (!render_func_ptr) {
        return; // Error: null pointer
    }

    frame_func = render_func_ptr;
}

// Set callback function that gets called to read button state
void tinybit_poll_input_cb(void (*poll_input_func_ptr)()) {
    if (!poll_input_func_ptr) {
        return; // Error: null pointer
    }

    input_func = poll_input_func_ptr;
}

// Set callback function for sleeping/delaying execution
void tinybit_sleep_cb(void (*sleep_func_ptr)(int ms)) {
    if (!sleep_func_ptr) {
        return; // Error: null pointer
    }

    sleep_func = sleep_func_ptr;
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

    get_ticks_ms_func = get_ticks_ms_func_ptr;
}

// Set callback function for queuing audio each frame
void tinybit_audio_queue_cb(void (*audio_queue_func_ptr)()) {
    if (!audio_queue_func_ptr) {
        return; // Error: null pointer
    }

    audio_queue_func = audio_queue_func_ptr;
}
