#ifndef TINYBIT_H
#define TINYBIT_H

#include <stdint.h>
#include <stdbool.h>

#include "lua/lua.h"
#include "lua/lualib.h"
#include "lua/lauxlib.h"

#if defined(_MSC_VER)
    // Microsoft Visual Studio
    #define PACKED_STRUCT(name) __pragma(pack(push, 1)) struct name __pragma(pack(pop))
#elif defined(__GNUC__) || defined(__clang__)
    // GCC or Clang
    #define PACKED_STRUCT(name) struct __attribute__((__packed__)) name
#else
    // Fallback (may need adjustment per compiler)
    #define PACKED_STRUCT(name) struct name
    #pragma message("Packing is not defined for this compiler. Please define PACKED_STRUCT manually.")
#endif

// cartridge dimensions
#define TB_CARTRIDGE_WIDTH 200
#define TB_CARTRIDGE_HEIGHT 230

// Screen dimensions
#define TB_SCREEN_WIDTH 128
#define TB_SCREEN_HEIGHT 128

// Memory sizes
#define TB_MEM_SPRITESHEET_START 0x00000
#define TB_MEM_SPRITESHEET_SIZE  0x08000 // 32Kb
#define TB_MEM_DISPLAY_START     (TB_MEM_SPRITESHEET_START + TB_MEM_SPRITESHEET_SIZE)
#define TB_MEM_DISPLAY_SIZE      0x08000 // 32Kb
#define TB_MEM_SCRIPT_START      (TB_MEM_DISPLAY_START + TB_MEM_DISPLAY_SIZE)
#define TB_MEM_SCRIPT_SIZE       0x03000 // 12Kb
#define TB_MEM_USER_START        (TB_MEM_SCRIPT_START + TB_MEM_SCRIPT_SIZE)
#define TB_MEM_USER_SIZE         0x01000 // 4Kb
#define TB_MEM_SIZE              (TB_MEM_SPRITESHEET_SIZE + TB_MEM_DISPLAY_SIZE + TB_MEM_SCRIPT_SIZE + TB_MEM_USER_SIZE) // 80Kb

// define cover location
#define TB_COVER_X 35
#define TB_COVER_Y 34

PACKED_STRUCT(TinyBitMemory) {
    uint8_t spritesheet[TB_MEM_SPRITESHEET_SIZE]; 
    uint8_t display[TB_MEM_DISPLAY_SIZE];
    uint8_t script[TB_MEM_SCRIPT_SIZE];
    uint8_t user[TB_MEM_USER_SIZE];
};

enum TinyBitButton {
    TB_BUTTON_A,
    TB_BUTTON_B,
    TB_BUTTON_UP,
    TB_BUTTON_DOWN,
    TB_BUTTON_LEFT,
    TB_BUTTON_RIGHT,
    TB_BUTTON_START,
    TB_BUTTON_SELECT
};

extern struct TinyBitMemory* tinybit_memory;
extern int (*gamecount_func)();
extern void (*gamecover_func)(int index);
extern void (*gameload_func)(int index);

void tinybit_init(struct TinyBitMemory* memory, uint8_t* button_state_ptr);
bool tinybit_feed_cartridge(uint8_t* cartridge_buffer, size_t bytes);
bool tinybit_start();
bool tinybit_frame();

void tinybit_log_cb(void (*log_func_ptr)(const char*));
void tinybit_gamecount_cb(int (*gamecount_func_ptr)());
void tinybit_gameload_cb(void (*gameload_func_ptr)(int index));

#endif