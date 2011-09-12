/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * STIR -- Sifteo Tiled Image Reducer
 * M. Elizabeth Scott <beth@sifteo.com>
 *
 * Copyright <c> 2011 Sifteo, Inc. All rights reserved.
 */

#include "script.h"

namespace Stir {

Script::Script(Logger &l)
    : log(l)
{    
    L = lua_open();
    luaopen_io(L);
    luaopen_base(L);
    luaopen_table(L);
    luaopen_string(L);
    luaopen_math(L);
}

Script::~Script()
{
    lua_close(L);
}

bool Script::run(const char *filename)
{
    int s = luaL_loadfile(L, filename);

    if (!s) {
	s = lua_pcall(L, 0, LUA_MULTRET, 0);
    }

    if (s) {
	log.error("Script error: %s", lua_tostring(L, -1));
	lua_pop(L, 1);
	return false;
    }

    return true;
}

void Script::setVariable(const char *key, const char *value)
{
    lua_pushstring(L, value);
    lua_setglobal(L, key);
}

};  // namespace Stir
