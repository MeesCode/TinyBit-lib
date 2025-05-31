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

#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"

struct TinyBitMemory* tinybit_memory;

size_t cartridge_index = 0; // index for cartridge buffer
//uint8_t source_buffer[TB_CARTRIDGE_WIDTH * TB_CARTRIDGE_HEIGHT * 4 - TB_SCREEN_WIDTH * TB_SCREEN_HEIGHT * 4]; // max source size
char source_buffer[16000];
int frame_timer = 0;
lua_State* L;

bool tinybit_init(struct TinyBitMemory* memory, uint8_t* bs) {

    tinybit_memory = memory;
    button_state = bs;

    // initialize memory
    memory_init();
    
    // set up lua VM
    L = luaL_newstate();

    // load lua libraries
    static const luaL_Reg loadedlibs[] = {
        {LUA_GNAME, luaopen_base},
        {LUA_COLIBNAME, luaopen_coroutine},
        {LUA_TABLIBNAME, luaopen_table},
        {LUA_STRLIBNAME, luaopen_string},
        {LUA_MATHLIBNAME, luaopen_math},
        {NULL, NULL}
    };

    const luaL_Reg* lib;
    for (lib = loadedlibs; lib->func; lib++) {
        luaL_requiref(L, lib->name, lib->func, 1);
        lua_pop(L, 1);  /* remove lib */
    }

    // setup global variables
    lua_pushinteger(L, TB_MEM_SIZE);
    lua_setglobal(L, "MEM_SIZE");
    lua_pushinteger(L, TB_MEM_SPRITESHEET_START);
    lua_setglobal(L, "TB_MEM_SPRITESHEET_START");
    lua_pushinteger(L, TB_MEM_SPRITESHEET_SIZE);
    lua_setglobal(L, "TB_MEM_SPRITESHEET_SIZE");
    lua_pushinteger(L, TB_MEM_DISPLAY_START);
    lua_setglobal(L, "TB_MEM_DISPLAY_START");
    lua_pushinteger(L, TB_MEM_DISPLAY_SIZE);
    lua_setglobal(L, "TB_MEM_DISPLAY_SIZE");
    lua_pushinteger(L, TB_MEM_USER_START);
    lua_setglobal(L, "TB_MEM_USER_START");

    // set lua tone variables
    lua_pushinteger(L, Ab);
    lua_setglobal(L, "Ab");
    lua_pushinteger(L, A);
    lua_setglobal(L, "A");
    lua_pushinteger(L, As);
    lua_setglobal(L, "As");
    lua_pushinteger(L, Bb);
    lua_setglobal(L, "Bb");
    lua_pushinteger(L, B);
    lua_setglobal(L, "B");
    lua_pushinteger(L, C);
    lua_setglobal(L, "C");
    lua_pushinteger(L, Cs);
    lua_setglobal(L, "Cs");
    lua_pushinteger(L, Db);
    lua_setglobal(L, "Db");
    lua_pushinteger(L, D);
    lua_setglobal(L, "D");
    lua_pushinteger(L, Ds);
    lua_setglobal(L, "Ds");
    lua_pushinteger(L, Eb);
    lua_setglobal(L, "Eb");
    lua_pushinteger(L, E);
    lua_setglobal(L, "E");
    lua_pushinteger(L, F);
    lua_setglobal(L, "F");
    lua_pushinteger(L, Fs);
    lua_setglobal(L, "Fs");
    lua_pushinteger(L, Gb);
    lua_setglobal(L, "Gb");
    lua_pushinteger(L, G);
    lua_setglobal(L, "G");
    lua_pushinteger(L, Gs);
    lua_setglobal(L, "Gs");

    // set lua waveforms
    lua_pushinteger(L, SINE);
    lua_setglobal(L, "SINE");
    lua_pushinteger(L, SAW);
    lua_setglobal(L, "SAW");
    lua_pushinteger(L, SQUARE);
    lua_setglobal(L, "SQUARE");

    lua_pushinteger(L, TB_SCREEN_WIDTH);
    lua_setglobal(L, "TB_SCREEN_WIDTH");
    lua_pushinteger(L, TB_SCREEN_HEIGHT);
    lua_setglobal(L, "TB_SCREEN_HEIGHT");

    lua_pushinteger(L, BUTTON_A);
	lua_setglobal(L, "X");
	lua_pushinteger(L, BUTTON_B);
	lua_setglobal(L, "Z");
	lua_pushinteger(L, BUTTON_UP);
	lua_setglobal(L, "UP");
	lua_pushinteger(L, BUTTON_DOWN);
	lua_setglobal(L, "DOWN");
	lua_pushinteger(L, BUTTON_LEFT);
	lua_setglobal(L, "LEFT");
	lua_pushinteger(L, BUTTON_RIGHT);
	lua_setglobal(L, "RIGHT");
	lua_pushinteger(L, BUTTON_START);
	lua_setglobal(L, "START");
    lua_pushinteger(L, BUTTON_SELECT);
	lua_setglobal(L, "SELECT");

    lua_setup_functions(L);

    // TODO: load font

    return true;
}

bool tinybit_feed_catridge(uint8_t* cartridge_buffer, size_t pixels){
    
    // TODO: fix this
    // check if cartridge index is within bounds
    // if (cartridge_index + (pixels/4) > (TB_CARTRIDGE_WIDTH * TB_CARTRIDGE_HEIGHT)/4) {
    //     return false; 
    // }

    for(int i = 0; i < pixels; i += 4) {
        uint8_t r = cartridge_buffer[i];
        uint8_t g = cartridge_buffer[i + 1];
        uint8_t b = cartridge_buffer[i + 2];
        uint8_t a = cartridge_buffer[i + 3];

        // spritesheet data
        if (cartridge_index < TB_SCREEN_WIDTH * TB_SCREEN_HEIGHT * 4) {
            tinybit_memory->spritesheet[cartridge_index] = (r & 0x3) << 6 | (g & 0x3) << 4 | (b & 0x3) << 2 | (a & 0x3) << 0;
        }

        // source code
        else if (cartridge_index - TB_SCREEN_WIDTH * TB_SCREEN_HEIGHT * 4 < TB_CARTRIDGE_WIDTH * TB_CARTRIDGE_HEIGHT * 4) {
            if(cartridge_index - TB_SCREEN_WIDTH * TB_SCREEN_HEIGHT * 4 < sizeof(source_buffer)) {
                source_buffer[cartridge_index - TB_SCREEN_WIDTH * TB_SCREEN_HEIGHT * 4] = (r & 0x3) << 6 | (g & 0x3) << 4 | (b & 0x3) << 2 | (a & 0x3) << 0;
            } else {
                // buffer overflow, ignore the rest
                return false;
            }
        }

        // increment cartridge index
        cartridge_index++;
    }

    return true;
}

bool tinybit_start(){
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
        return false; // error in lua code
    }

    // save current button state
    save_button_state();

    return true;
}