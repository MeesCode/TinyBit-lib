
#include <stdint.h>
#include <string.h>
#include "memory.h"
#include "tinybit.h"

struct TinyBitMemory* tinybit_memory;

// Initialize TinyBit memory by clearing all sections (preserving lua_state)
void memory_init() {
    memset(tinybit_memory, 0, TB_MEM_LUA_STATE_START);
    memset((uint8_t*)tinybit_memory + TB_MEM_LUA_STATE_START + TB_MEM_LUA_STATE_SIZE, 0,
           TB_MEM_SIZE - TB_MEM_LUA_STATE_START - TB_MEM_LUA_STATE_SIZE);
}

// Copy memory from source to destination within TinyBit memory space
void mem_copy(int dst, int src, int size) {
    if (dst + size > TB_MEM_SIZE || src + size > TB_MEM_SIZE) {
        return;
    }
    memcpy(&tinybit_memory[dst], &tinybit_memory[src], size);
}

// Read a byte from TinyBit memory at specified address
uint8_t mem_peek(int dst) {
    if (dst < 0 || dst > TB_MEM_SIZE) {
        return 0;
    }
    return *(uint8_t*)&tinybit_memory[dst];
}

// Write a byte to TinyBit memory at specified address
void mem_poke(int dst, int val){
    if (dst < 0 || dst > TB_MEM_SIZE) {
        return;
    }
    *(uint8_t*)&tinybit_memory[dst] = val & 0xff;
}