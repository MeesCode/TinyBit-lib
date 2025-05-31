#ifndef MEMORY_H
#define MEMORY_H

#include <stdint.h>

void memory_init();
void mem_copy(int dst, int src, int size);
uint8_t mem_peek(int);
void mem_poke(int, int);

#endif