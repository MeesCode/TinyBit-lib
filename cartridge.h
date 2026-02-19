#ifndef CARTRIDGE_H
#define CARTRIDGE_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#include "lua/lua.h"

void cartridge_init(void);
void cartridge_destroy(void);
bool cartridge_feed(const uint8_t* buffer, size_t size);
void cartridge_register_lua(lua_State* L);
bool cartridge_load_pending(void);

extern int (*gamecount_func)();
extern void (*gameload_func)(int index);

#endif
