# TinyBit Library

TinyBit is a lightweight, Lua-powered game engine and runtime designed for embedded and resource-constrained systems. This library provides the core logic, memory management, and Lua integration for running TinyBit games on platforms such as the ESP32.

## Features

- **Game Logic in Lua:** Easily script game logic using the Lua language.
- **Frame Buffer Management:** Efficiently manages display and sprite memory.
- **PNG Cartridge Loading:** Supports loading game assets from PNG files.
- **User Memory:** Provides a dedicated memory region for user data.
- **Simple API:** Minimal, easy-to-use C API for integration with your platform.

## Directory Structure

```
tinybit/
├── lua/             # Lua interpreter and libraries
├── tinybit.h        # Public API header
├── tinybit.c        # Core implementation
└── ...              # Additional source files
```

## API Overview

### Initialization

```c
#include "tinybit.h"

struct TinyBitMemory tb_mem = {0};
uint8_t button_state = 0;

tinybit_init(&tb_mem, &button_state);
```

### Feeding Cartridge Data

```c
tinybit_feed_catridge(png_data, data_length);
```

### Game Loop

```c
tinybit_start_game();
while (1) {
    tinybit_frame();
    // Render tb_mem.display to your screen
}
```

## Memory Layout

- **spritesheet:** Game graphics and assets
- **display:** Frame buffer for rendering
- **user:** User-defined persistent data

All memory is managed in a single `struct TinyBitMemory` instance.

## Integration

TinyBit is platform-agnostic and can be integrated into any C project. You are responsible for providing display, input, and storage drivers.

## License

MIT License

---

**TinyBit — Make retro games portable and fun!**