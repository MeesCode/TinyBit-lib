
#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"

#include "lua_functions.h"
#include "graphics.h"
#include "memory.h"
#include "font.h"
#include "input.h"
#include "audio.h"

void lua_setup_functions(lua_State* L) {
    lua_pushcfunction(L, lua_sprite);
    lua_setglobal(L, "sprite");
    lua_pushcfunction(L, lua_millis);
    lua_setglobal(L, "millis");
    lua_pushcfunction(L, lua_stroke);
    lua_setglobal(L, "stroke");
    lua_pushcfunction(L, lua_fill);
    lua_setglobal(L, "fill");
    lua_pushcfunction(L, lua_rect);
    lua_setglobal(L, "rect");
    lua_pushcfunction(L, lua_oval);
    lua_setglobal(L, "oval");
    lua_pushcfunction(L, lua_tone);
    lua_setglobal(L, "tone");
    lua_pushcfunction(L, lua_noise);
    lua_setglobal(L, "noise");
    lua_pushcfunction(L, lua_bpm);
    lua_setglobal(L, "bpm");
    lua_pushcfunction(L, lua_btn);
    lua_setglobal(L, "btn");
    lua_pushcfunction(L, lua_btnp);
    lua_setglobal(L, "btnp");
    lua_pushcfunction(L, lua_mycopy);
    lua_setglobal(L, "copy");
    lua_pushcfunction(L, lua_cls);
    lua_setglobal(L, "cls");
    lua_pushcfunction(L, lua_peek);
    lua_setglobal(L, "peek");
    lua_pushcfunction(L, lua_poke);
    lua_setglobal(L, "poke");
    lua_pushcfunction(L, lua_random);
    lua_setglobal(L, "random");
    lua_pushcfunction(L, lua_cursor);
    lua_setglobal(L, "cursor");
    lua_pushcfunction(L, lua_prints);
    lua_setglobal(L, "prints");
    lua_pushcfunction(L, lua_text);
    lua_setglobal(L, "text");
}

int lua_sprite(lua_State* L) {
    if (lua_gettop(L) != 8 && lua_gettop(L) != 9) {
        return 0;
    }

    int sourceX = (int)luaL_checknumber(L, 1);
    int sourceY = (int)luaL_checknumber(L, 2);
    int sourceW = (int)luaL_checknumber(L, 3);
    int sourceH = (int)luaL_checknumber(L, 4);

    int targetX = (int)luaL_checknumber(L, 5);
    int targetY = (int)luaL_checknumber(L, 6);
    int targetW = (int)luaL_checknumber(L, 7);
    int targetH = (int)luaL_checknumber(L, 8);

    if(lua_gettop(L) == 8) {
        draw_sprite(sourceX, sourceY, sourceW, sourceH, targetX, targetY, targetW, targetH);
        return 0;
    }

    int targetR = (int)luaL_checknumber(L, 9);

    draw_sprite_rotated(sourceX, sourceY, sourceW, sourceH, targetX, targetY, targetW, targetH, targetR);
    return 0;
}

int lua_millis(lua_State* L) {
    lua_Integer m = millis();
    lua_pushinteger(L, m);
    return 1;
}

int lua_random(lua_State* L) {
    if (lua_gettop(L) != 2) {
        return 0;
    }

    int min = (int)luaL_checknumber(L, 1);
    int max = (int)luaL_checknumber(L, 2);

    lua_pushinteger(L, random_range(min, max));
    
    return 1;
}

int lua_stroke(lua_State* L) {
    if (lua_gettop(L) != 5) {
        return 0;
    }

    int width = (int)luaL_checknumber(L, 1);
    int r = (int)luaL_checknumber(L, 2);
    int g = (int)luaL_checknumber(L, 3);
    int b = (int)luaL_checknumber(L, 4);
    int a = (int)luaL_checknumber(L, 5);

    set_stroke(width, r, g, b, a);
    return 0;
}

int lua_fill(lua_State* L) {
    if (lua_gettop(L) != 4 && lua_gettop(L) != 8) {
        return 0;
    }

    int r = (int)luaL_checknumber(L, 1);
    int g = (int)luaL_checknumber(L, 2);
    int b = (int)luaL_checknumber(L, 3);
    int a = (int)luaL_checknumber(L, 4);
    
    set_fill(r, g, b, a);

    return 0;
}

int lua_text(lua_State* L) {
    if (lua_gettop(L) != 4) {
        return 0;
    }

    int r = (int)luaL_checknumber(L, 1);
    int g = (int)luaL_checknumber(L, 2);
    int b = (int)luaL_checknumber(L, 3);
    int a = (int)luaL_checknumber(L, 4);

    font_text_color(r, g, b, a);
    return 0;
}

int lua_rect(lua_State* L) {
    if (lua_gettop(L) != 4) {
        return 0;
    }

    int x = (int)luaL_checknumber(L, 1);
    int y = (int)luaL_checknumber(L, 2);
    int w = (int)luaL_checknumber(L, 3);
    int h = (int)luaL_checknumber(L, 4);

    draw_rect(x, y, w, h);
    return 0;
}

int lua_oval(lua_State* L) {
    if (lua_gettop(L) != 4) {
        return 0;
    }

    int x = (int)luaL_checknumber(L, 1);
    int y = (int)luaL_checknumber(L, 2);
    int w = (int)luaL_checknumber(L, 3);
    int h = (int)luaL_checknumber(L, 4);

    draw_oval(x, y, w, h);
    return 0;
}

int lua_tone(lua_State* L) {
    if (lua_gettop(L) < 4 && lua_gettop(L) > 6) {
        return 0;
    }

    TONE tone = luaL_checkinteger(L, 1);
    int octave = luaL_checkinteger(L, 2);
    int eights = luaL_checkinteger(L, 3);
    WAVEFORM wf = luaL_checkinteger(L, 4);

    if(lua_gettop(L) == 4) {
        play_tone(tone, octave, eights, wf, volume, channel);
        return 0;
    }

    int vol = luaL_checkinteger(L, 5);

    if(lua_gettop(L) == 5) {
        play_tone(tone, octave, eights, wf, vol, channel);
        return 0;
    }

    int chan = luaL_checkinteger(L, 6);

    if(lua_gettop(L) == 6) {
        play_tone(tone, octave, eights, wf, vol, chan);
    }

    
    return 0;
}

int lua_noise(lua_State* L) {
    if (lua_gettop(L) == 1) {
        int eights = luaL_checkinteger(L, 1);
        play_noise(eights, volume, channel);
        return 0;
    }

    if (lua_gettop(L) == 2) {
        int eights = luaL_checkinteger(L, 1);
        int vol = luaL_checkinteger(L, 2);
        play_noise(eights, vol, channel);
        return 0;
    }

    if (lua_gettop(L) == 3) {
        int eights = luaL_checkinteger(L, 1);
        int vol = luaL_checkinteger(L, 2);
        int chan = luaL_checkinteger(L, 3);
        play_noise(eights, vol, chan);
        return 0;
    }

    return 0;
}

int lua_volume(lua_State* L) {
    if (lua_gettop(L) != 1) {
        return 0;
    }

    int new_volume = luaL_checkinteger(L, 1);
    set_volume(new_volume);
    return 0;
}

int lua_channel(lua_State* L) {
    if (lua_gettop(L) != 1) {
        return 0;
    }

    int new_channel = luaL_checkinteger(L, 1);
    set_channel(new_channel);
    return 0;
}

int lua_bpm(lua_State* L) {
    int new_bpm = luaL_checkinteger(L, 1);
    set_bpm(new_bpm);
    return 0;
}

int lua_btn(lua_State* L) {
    BUTTON btn = luaL_checkinteger(L, 1);
    lua_pushboolean(L, input_btn(btn));
    return 1;
}

int lua_btnp(lua_State* L) {
    BUTTON btn = luaL_checkinteger(L, 1);
    lua_pushboolean(L, input_btnp(btn));
    return 1;
}

int lua_cls(lua_State* L) {
    draw_cls();
    return 0;
}

int lua_mycopy(lua_State* L) {
    if (lua_gettop(L) != 3) {
        return 0;
    }

    int dst = luaL_checkinteger(L, 1);
    int src = luaL_checkinteger(L, 2);
    int size = luaL_checkinteger(L, 3);

    mem_copy(dst, src, size);
    return 0;
}

int lua_peek(lua_State* L) {
    if (lua_gettop(L) != 1) {
        return 0;
    }

    int dst = luaL_checkinteger(L, 1);

    lua_pushinteger(L, mem_peek(dst));
    return 1;
}

int lua_poke(lua_State* L) {
    if (lua_gettop(L) != 2) {
        return 0;
    }

    int dst = luaL_checkinteger(L, 1);
    int val = luaL_checkinteger(L, 2);

    mem_poke(dst, val);
    return 0;
}

int lua_cursor(lua_State* L) {
    if (lua_gettop(L) != 2) {
        return 0;
    }

    int x = (int)luaL_checknumber(L, 1);
    int y = (int)luaL_checknumber(L, 2);

    font_cursor(x, y);
    return 0;
}

int lua_prints(lua_State* L) {

    if (lua_gettop(L) != 1) {
        return 0;
    }

    const char* str = luaL_checkstring(L, 1);

    font_prints(str);
    return 0;
}