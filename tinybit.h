#ifndef TINYBIT_H
#define TINYBIT_H

#include <stdint.h>
#include <stdbool.h>

#include "lua/lua.h"
#include "lua/lualib.h"
#include "lua/lauxlib.h"

// cartridge dimensions
#define TB_CARTRIDGE_WIDTH 336
#define TB_CARTRIDGE_HEIGHT 376

// Screen dimensions
#define TB_SCREEN_WIDTH 128
#define TB_SCREEN_HEIGHT 128

// Memory sizes
#define TB_MEM_SIZE 0x30000 // 192 KiB
#define TB_MEM_SPRITESHEET_START 0x00000
#define TB_MEM_SPRITESHEET_SIZE 0x10000
#define TB_MEM_DISPLAY_START 0x10000
#define TB_MEM_DISPLAY_SIZE 0x10000
#define TB_MEM_USER_START 0x20000
#define TB_MEM_USER_SIZE 0x10000

struct TinyBitMemory {
    uint8_t spritesheet[TB_MEM_SPRITESHEET_SIZE]; 
    uint8_t display[TB_MEM_DISPLAY_SIZE];
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

void tinybit_init(struct TinyBitMemory* memory, uint8_t* bs);
bool tinybit_feed_catridge(uint8_t* cartridge_buffer, size_t bytes);
bool tinybit_frame();
bool tinybit_start();
char* tinybit_get_source();

#endif