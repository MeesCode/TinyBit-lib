# TinyBit Library

TinyBit is a lightweight, Lua-powered game engine and runtime designed for creating retro-style 2D games. This library provides the core logic, memory management, graphics rendering, and Lua integration for running TinyBit games on various platforms.

## Features

- **Lua Scripting Engine:** Complete Lua 5.4 interpreter with game-specific APIs
- **128x128 Pixel Display:** 4-bit RGBA color depth with efficient rendering
- **PNG Cartridge System:** Load games from PNG files with embedded assets and scripts
- **Graphics Library:** Sprites, primitives, polygons with alpha blending
- **Input System:** 8-button gamepad with press/hold detection
- **Memory Management:** Organized 80KB memory layout with peek/poke access
- **Audio Support:** Musical tones and sound effects (framework ready)
- **Platform Agnostic:** Callback-based architecture for easy porting

## Directory Structure

```
tinybit/
├── lua/                 # Lua 5.4 interpreter and standard libraries
├── pngle/              # PNG decoder for cartridge loading
├── assets/             # Font and bitmap assets
├── tinybit.h           # Public API header
├── tinybit.c           # Core system implementation
├── graphics.h/.c       # 2D graphics and rendering
├── input.h/.c          # Input handling and button states
├── memory.h/.c         # Memory management functions
├── font.h/.c           # Text rendering system
├── audio.h/.c          # Audio system (placeholder)
└── lua_functions.h/.c  # Lua API bindings
```

## Core API Reference

### System Initialization

```c
#include "tinybit.h"

struct TinyBitMemory tb_mem = {0};
bool button_state[TB_BUTTON_COUNT] = {0};

// Initialize the TinyBit system
tinybit_init(&tb_mem, button_state);

// Set up callbacks for your platform
tinybit_render_cb(my_render_function);
tinybit_poll_input_cb(my_input_function);
tinybit_sleep_cb(my_sleep_function);
tinybit_get_ticks_ms_cb(my_timer_function);
tinybit_log_cb(printf);
```

### Loading and Running Games

```c
// Feed PNG cartridge data in chunks
while (bytes_read = read_cartridge_data(buffer, sizeof(buffer))) {
    tinybit_feed_cartridge(buffer, bytes_read);
}

// Start executing the Lua script
if (tinybit_start()) {
    // Run the main game loop
    tinybit_loop();
}
```

### Callback Functions

Your platform must provide these callback implementations:

```c
// Render the display buffer to screen
void my_render_function() {
    // Copy tb_mem.display to your screen/framebuffer
    // Display format: 128x128 pixels, 2 bytes per pixel (4-bit RGBA)
}

// Poll input and update button states
void my_input_function() {
    // Read your platform's input and update button_state array
    button_state[TB_BUTTON_A] = read_button_a();
    button_state[TB_BUTTON_B] = read_button_b();
    // ... etc for all buttons
}

// Sleep/delay function
void my_sleep_function(int ms) {
    // Platform-specific sleep implementation
}

// Get milliseconds since startup
long my_timer_function() {
    // Return current time in milliseconds
}
```

## Memory Layout

The TinyBitMemory structure organizes 80KB of game memory:

```c
struct TinyBitMemory {
    uint8_t spritesheet[32768];  // 0x00000-0x07FFF: Game assets
    uint8_t display[32768];      // 0x08000-0x0FFFF: Screen buffer  
    uint8_t script[12288];       // 0x10000-0x12FFF: Lua script
    uint8_t user[4096];          // 0x13000-0x13FFF: User data
};
```

### Memory Regions

- **Spritesheet (32KB):** Compressed 4-bit sprite and texture data
- **Display (32KB):** 128x128 screen buffer (2 bytes per pixel, 4-bit RGBA)
- **Script (12KB):** Lua source code loaded from cartridges
- **User (4KB):** User memory accessible via peek/poke

## Button Constants

```c
enum TinyBitButton {
    TB_BUTTON_A,        // Primary action
    TB_BUTTON_B,        // Secondary action  
    TB_BUTTON_UP,       // Directional pad
    TB_BUTTON_DOWN,
    TB_BUTTON_LEFT,
    TB_BUTTON_RIGHT,
    TB_BUTTON_START,    // Menu/pause
    TB_BUTTON_SELECT,   // Alt function
    TB_BUTTON_COUNT     // Total count
};
```

## Lua Game API

Games access TinyBit features through Lua functions:

### Graphics
- `cls()` - Clear display
- `sprite(sx,sy,sw,sh,dx,dy,dw,dh[,rot])` - Draw sprite
- `rect(x,y,w,h)` - Draw rectangle
- `oval(x,y,w,h)` - Draw oval
- `line(x1,y1,x2,y2)` - Draw line
- `fill(r,g,b,a)` - Set fill color
- `stroke(width,r,g,b,a)` - Set stroke

### Input
- `btn(button)` - Check button held
- `btnp(button)` - Check button pressed

### Memory
- `peek(addr)` - Read memory byte
- `poke(addr,val)` - Write memory byte
- `copy(dst,src,size)` - Copy memory

### Utilities
- `millis()` - Current time
- `random(min,max)` - Random number
- `log(msg)` - Debug output

## PNG Cartridge Format

TinyBit uses a clever steganographic approach to embed game data in PNG files:

1. **Base Image:** 200x230 pixel cartridge template
2. **Cover Art:** 128x128 game preview overlaid at position (35,34)
3. **Hidden Data:** Spritesheet and script embedded in LSBs of pixel data
4. **File Extension:** `.tb.png` identifies TinyBit cartridges

## Integration Examples

### Embedded Systems (ESP32)
```c
// Platform-specific implementations
void esp32_render() {
    tft_display_buffer(tb_mem.display, 128, 128);
}

void esp32_input() {
    button_state[TB_BUTTON_A] = !digitalRead(PIN_BUTTON_A);
    // ... map other pins
}
```

### Desktop (SDL2)
```c
void sdl_render() {
    SDL_UpdateTexture(texture, NULL, tb_mem.display, 256);
    SDL_RenderCopy(renderer, texture, NULL, NULL);
    SDL_RenderPresent(renderer);
}

void sdl_input() {
    const Uint8* keys = SDL_GetKeyboardState(NULL);
    button_state[TB_BUTTON_A] = keys[SDL_SCANCODE_A];
    // ... map other keys
}
```

## Building

The TinyBit library is designed to be embedded in larger projects. Include the source files in your build system:

```makefile
TINYBIT_SOURCES = \
    tinybit/tinybit.c \
    tinybit/graphics.c \
    tinybit/input.c \
    tinybit/memory.c \
    tinybit/font.c \
    tinybit/audio.c \
    tinybit/lua_functions.c \
    tinybit/lua/*.c \
    tinybit/pngle/*.c

CFLAGS += -Isrc/tinybit
```

## Performance Characteristics

- **Memory Usage:** 80KB game RAM + ~50KB code/stack
- **CPU Usage:** ~60 FPS on 160MHz ESP32
- **Storage:** Games typically 50-200KB (PNG compressed)
- **Display:** 30-60 FPS depending on complexity

## Platform Requirements

- **C99 Compiler:** Standard C with stdint.h
- **Memory:** ~130KB RAM minimum
- **Display:** Any pixel-addressable output
- **Input:** 8 digital buttons (minimum A + directional)
- **Storage:** Access to cartridge PNG files

---

**TinyBit Library — Portable retro gaming made simple!**