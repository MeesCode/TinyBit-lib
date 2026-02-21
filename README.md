# TinyBit Library

TinyBit is a lightweight, Lua-powered game engine and runtime designed for creating retro-style 2D games. This library provides the core logic, memory management, graphics rendering, audio synthesis, and Lua integration for running TinyBit games on various platforms.

## Features

- **Lua Scripting Engine:** Complete Lua 5.4 interpreter with game-specific APIs
- **128x128 Pixel Display:** 4-bit RGBA color depth (RGBA4444) with alpha blending
- **PNG Cartridge System:** Load games from PNG files with steganographically embedded assets and scripts
- **Graphics Library:** Sprites with scaling/rotation, primitives, polygons, and per-pixel alpha blending
- **Input System:** 8-button gamepad with press/hold detection
- **Audio Engine:** ABC notation music and sound effects with sine, saw, square, and noise waveforms
- **Memory Management:** Organized ~200KB memory layout with peek/poke access
- **Platform Agnostic:** Callback-based architecture for easy porting

## Directory Structure

```
tinybit/
├── lua/                 # Lua 5.4 interpreter and standard libraries
├── pngle/              # Lightweight PNG decoder (pngle + miniz)
├── ABC-parser/         # ABC music notation parser
├── tinybit.h/.c        # Public API and core system loop
├── graphics.h/.c       # 2D graphics rendering and alpha blending
├── input.h/.c          # Input handling and button states
├── memory.h/.c         # Memory management and peek/poke
├── font.h/.c           # 8x8 bitmap text rendering
├── audio.h/.c          # Audio synthesis with ABC notation
├── cartridge.h/.c      # Cartridge loading and game selector support
├── lua_functions.h/.c  # Lua API bindings
├── lua_pool.c          # Lua VM state management
└── helpers.c           # Utility functions
```

## Core API Reference

### System Initialization

```c
#include "tinybit.h"

struct TinyBitMemory tb_mem = {0};

// Initialize the TinyBit system
tinybit_init(&tb_mem);

// Set up callbacks for your platform
tinybit_render_cb(my_render_function);
tinybit_poll_input_cb(my_input_function);
tinybit_get_ticks_ms_cb(my_timer_function);
tinybit_audio_queue_cb(my_audio_function);
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

// Restart the current game
tinybit_restart();

// Clean up when done
tinybit_stop();
```

### Callback Functions

Your platform must provide these callback implementations:

```c
// Render the display buffer to screen
void my_render_function() {
    // Copy tb_mem.display to your screen/framebuffer
    // Display format: 128x128 pixels, uint16_t per pixel (RGBA4444)
}

// Poll input and update button states
void my_input_function() {
    // Read your platform's input and update tb_mem.button_input
    tb_mem.button_input[TB_BUTTON_A] = read_button_a();
    tb_mem.button_input[TB_BUTTON_B] = read_button_b();
    // ... etc for all buttons
}

// Get milliseconds since startup
int my_timer_function() {
    // Return current time in milliseconds
}

// Queue audio samples for playback
void my_audio_function() {
    // Read tb_mem.audio_buffer (367 int16_t samples) and queue to audio device
}
```

## Memory Layout

The `TinyBitMemory` structure organizes ~200KB of system memory:

```c
struct TinyBitMemory {
    uint16_t spritesheet[16384];    // 32KB - Game sprite/texture data
    uint16_t display[16384];        // 32KB - Screen buffer (128x128 RGBA4444)
    uint8_t  script[12288];         // 12KB - Lua script storage
    uint8_t  lua_state[61440];      // 60KB - Lua VM state
    uint8_t  audio_data[12288];     // 12KB - Audio channel data
    uint8_t  pngle_data[49152];     // 48KB - PNG decoder buffer
    int16_t  audio_buffer[367];     // Audio samples per frame (22kHz @ 60fps)
    uint8_t  button_input[8];       // Button states
    uint8_t  user[10240];           // 10KB - User accessible memory
};
```

### Color Format

Colors use a 16-bit RGBA4444 format packed into `uint16_t`:
- Low byte: `RRRRGGGG` (4 bits red, 4 bits green)
- High byte: `BBBBAAAA` (4 bits blue, 4 bits alpha)

In Lua, colors are passed as packed RGBA8888 `uint32_t` values (automatically converted internally). Use the `rgba()`, `rgb()`, `hsb()`, or `hsba()` helper functions to create colors.

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
- `cls()` - Clear the display
- `sprite(sx, sy, sw, sh, dx, dy, dw, dh [, rotation])` - Draw sprite with optional rotation
- `duplicate(sx, sy, sw, sh, dx, dy, dw, dh [, rotation])` - Copy display region
- `rect(x, y, w, h)` - Draw rectangle
- `oval(x, y, w, h)` - Draw oval
- `line(x1, y1, x2, y2)` - Draw line
- `poly_add(x, y)` - Add vertex to polygon
- `poly_clear()` - Clear polygon vertices
- `draw_polygon()` - Draw the current polygon

### Colors and Styles
- `fill(color)` - Set fill color (RGBA8888 packed value)
- `stroke(width, color)` - Set stroke width and color
- `text(color)` - Set text color
- `rgba(r, g, b, a)` - Create color from RGBA components (0-255)
- `rgb(r, g, b)` - Create color from RGB components (alpha = 255)
- `hsb(h, s, b)` - Create color from HSB components (0-255)
- `hsba(h, s, b, a)` - Create color from HSBA components (0-255)

### Text
- `cursor(x, y)` - Set text cursor position
- `print(text)` - Print text at cursor position

### Input
- `btn(button)` - Check if button is currently held
- `btnp(button)` - Check if button was just pressed this frame

Button constants: `A`, `B`, `UP`, `DOWN`, `LEFT`, `RIGHT`, `START`, `SELECT`

### Audio
- `music(abc_string)` - Play looping music from ABC notation
- `sfx(abc_string)` - Play one-shot sound effect from ABC notation
- `sfx_active()` - Check if a sound effect is currently playing
- `bpm(beats_per_minute)` - Set tempo

Waveform constants: `SINE`, `SAW`, `SQUARE`, `NOISE`

### Memory
- `peek(address)` - Read byte from user memory
- `poke(address, value)` - Write byte to user memory
- `copy(dest, src, size)` - Copy memory region

### Utilities
- `millis()` - Get current frame time in milliseconds
- `random(min, max)` - Generate random integer in range
- `log(message)` - Print debug message to console
- `sleep(ms)` - Delay execution

### Game Management (Selector Only)
- `gamecount()` - Get number of available games
- `gamecover(index)` - Load game cover for preview
- `gameload(index)` - Load and start game by index

### Global Constants
- `TB_SCREEN_WIDTH` (128) - Screen width in pixels
- `TB_SCREEN_HEIGHT` (128) - Screen height in pixels

## PNG Cartridge Format

TinyBit uses a steganographic approach to embed game data in PNG files:

1. **Base Image:** 200x230 pixel cartridge template
2. **Cover Art:** 128x128 game preview overlaid at position (35, 34)
3. **Hidden Data:** Spritesheet and script embedded in LSBs of pixel data
4. **File Extension:** `.tb.png` identifies TinyBit cartridges

## Integration Examples

### Embedded Systems (ESP32)
```c
void esp32_render() {
    tft_display_buffer(tb_mem.display, 128, 128);
}

void esp32_input() {
    tb_mem.button_input[TB_BUTTON_A] = !digitalRead(PIN_BUTTON_A);
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
    tb_mem.button_input[TB_BUTTON_A] = keys[SDL_SCANCODE_A];
    // ... map other keys
}
```

## Building

The TinyBit library is built as part of the project via CMake:

```bash
mkdir build && cd build
cmake ..
cmake --build .
```

To embed in your own project, include the source files and add `src/tinybit` to your include path.

## Audio Details

- **Sample Rate:** 22kHz
- **Format:** 16-bit signed PCM, mono
- **Channels:** 2 (music + SFX)
- **Samples per Frame:** 367 (at 60 FPS)
- **Music Format:** ABC notation strings
- **Waveforms:** Sine, Sawtooth, Square, Noise

## Performance Characteristics

- **Memory Usage:** ~200KB total system memory
- **Target Frame Rate:** 60 FPS
- **Display:** 128x128 pixels, RGBA4444
- **Script Limit:** 12KB Lua source per cartridge

## Platform Requirements

- **C99 Compiler:** Standard C with stdint.h
- **Memory:** ~200KB RAM minimum
- **Display:** Any pixel-addressable output
- **Input:** 8 digital buttons (minimum A + directional)
- **Audio:** 22kHz 16-bit PCM output (optional)
- **Storage:** Access to cartridge PNG files
