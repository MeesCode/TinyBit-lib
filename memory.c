
#include <stdint.h>
#include <string.h>
#include "memory.h"
#include "tinybit.h"

void memory_init() {
    memset(tinybit_memory, 0, TB_MEM_SIZE);
}

void mem_copy(int dst, int src, int size) {
    if (dst + size > TB_MEM_SIZE || src + size > TB_MEM_SIZE) {
        return;
    }
    memcpy(&tinybit_memory[dst], &tinybit_memory[src], size);
}

uint8_t mem_peek(int dst) {
    if (dst < 0 || dst > TB_MEM_SIZE) {
        return 0;
    }
    return *(uint8_t*)&tinybit_memory[dst];
}

void mem_poke(int dst, int val){
    if (dst < 0 || dst > TB_MEM_SIZE) {
        return;
    }
    *(uint8_t*)&tinybit_memory[dst] = val & 0xff;
}