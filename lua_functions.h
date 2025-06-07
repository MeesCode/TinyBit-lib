#ifndef LUA_FUNCTIONS_H
#define LUA_FUNCTIONS_H

#include "lua/lua.h"
#include "lua/lualib.h"
#include "lua/lauxlib.h"

extern void (*log_func)(const char*);
extern long frame_time;

void lua_setup();

int lua_log(lua_State* L);
int lua_sprite(lua_State* L);
int lua_millis(lua_State* L);
int lua_random(lua_State* L);
int lua_stroke(lua_State* L);
int lua_fill(lua_State* L);
int lua_rect(lua_State* L);
int lua_line(lua_State* L);
int lua_oval(lua_State* L);
int lua_pset(lua_State* L);
int lua_tone(lua_State* L);
int lua_noise(lua_State* L);
int lua_volume(lua_State* L);
int lua_channel(lua_State* L);
int lua_bpm(lua_State* L);
int lua_btn(lua_State* L);
int lua_btnp(lua_State* L);
int lua_mycopy(lua_State* L);
int lua_cls(lua_State* L);
int lua_peek(lua_State* L);
int lua_poke(lua_State* L);
int lua_cursor(lua_State* L);
int lua_print(lua_State* L);
int lua_text(lua_State* L);
int lua_poly_add(lua_State* L);
int lua_poly_clear(lua_State* L);
int poly(lua_State* L);

#endif