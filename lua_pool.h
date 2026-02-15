#ifndef LUA_POOL_H
#define LUA_POOL_H

#include <stddef.h>
#include "lua/lua.h"

lua_State* lua_pool_newstate(void);
size_t lua_pool_get_used(void);

#endif
