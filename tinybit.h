#ifndef TINYBIT_H
#define TINYBIT_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

// cartridge dimensions
#define TB_CARTRIDGE_WIDTH 200
#define TB_CARTRIDGE_HEIGHT 230

// Screen dimensions
#define TB_SCREEN_WIDTH 128
#define TB_SCREEN_HEIGHT 128

// Audio configuration
#define TB_AUDIO_SAMPLE_RATE 22000
#define TB_AUDIO_FRAME_SAMPLES 367 // samples per 60fps frame

// define cover location
#define TB_COVER_X 35
#define TB_COVER_Y 34

// Memory sizes
#define TB_MEM_SPRITESHEET_SIZE     (TB_SCREEN_WIDTH * TB_SCREEN_HEIGHT * 2) // 32Kb
#define TB_MEM_DISPLAY_SIZE         (TB_SCREEN_WIDTH * TB_SCREEN_HEIGHT * 2) // 32Kb
#define TB_MEM_SCRIPT_SIZE          (12 * 1024) // 12Kb
#define TB_MEM_LUA_STATE_SIZE       (60 * 1024) // 60Kb
#define TB_MEM_AUDIO_DATA_SIZE      (12 * 1024) // 12Kb
#define TB_MEM_PNGLE_SIZE           (48 * 1024) // 48Kb
#define TB_MEM_AUDIO_BUFFER_SIZE    (TB_AUDIO_FRAME_SAMPLES * 2) // 734 bytes (367 16-bit samples)
#define TB_MEM_BUTTON_INPUT_SIZE    8 // 8 bytes (button inputs)
#define TB_MEM_USER_SIZE            (10 * 1024) // 10Kb

struct TinyBitMemory {
    uint16_t spritesheet[TB_SCREEN_WIDTH * TB_SCREEN_HEIGHT];
    uint16_t display[TB_SCREEN_WIDTH * TB_SCREEN_HEIGHT];
    uint8_t script[TB_MEM_SCRIPT_SIZE];
    uint8_t lua_state[TB_MEM_LUA_STATE_SIZE];
    uint8_t audio_data[TB_MEM_AUDIO_DATA_SIZE];
    uint8_t pngle_data[TB_MEM_PNGLE_SIZE];
    int16_t audio_buffer[TB_AUDIO_FRAME_SAMPLES];
    uint8_t button_input[TB_MEM_BUTTON_INPUT_SIZE];
    uint8_t user[TB_MEM_USER_SIZE];
};

#define TB_MEM_SIZE (sizeof(struct TinyBitMemory))

enum TinyBitButton {
    TB_BUTTON_A,
    TB_BUTTON_B,
    TB_BUTTON_UP,
    TB_BUTTON_DOWN,
    TB_BUTTON_LEFT,
    TB_BUTTON_RIGHT,
    TB_BUTTON_START,
    TB_BUTTON_SELECT,
    TB_BUTTON_COUNT
};

// Core TinyBit API functions
void tinybit_init(struct TinyBitMemory* memory);
bool tinybit_feed_cartridge(const uint8_t* cartridge_buffer, size_t bytes);
bool tinybit_start();
bool tinybit_restart();
void tinybit_loop();
void tinybit_stop();
void tinybit_sleep(int ms);

// Callback function setters
void tinybit_log_cb(void (*log_func_ptr)(const char*));
void tinybit_get_ticks_ms_cb(int (*get_ticks_ms_func_ptr)());
void tinybit_render_cb(void (*render_func_ptr)());
void tinybit_poll_input_cb(void (*poll_input_func_ptr)());
void tinybit_audio_queue_cb(void (*audio_queue_func_ptr)());
void tinybit_gamecount_cb(int (*gamecount_func_ptr)());
void tinybit_gameload_cb(void (*gameload_func_ptr)(int index));

#endif