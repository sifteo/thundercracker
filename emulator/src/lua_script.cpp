/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * Sifteo Thundercracker simulator
 * M. Elizabeth Scott <beth@sifteo.com>
 *
 * Copyright <c> 2011 Sifteo, Inc. All rights reserved.
 */
 
#include "lua_script.h"
#include "lua_system.h"
#include "lua_frontend.h"
#include "lua_cube.h"
#include "lua_runtime.h"
#include "lua_filesystem.h"


LuaScript::LuaScript(System &sys)
{    
    L = lua_open();
    luaL_openlibs(L);

    LuaSystem::sys = &sys;

    Lunar<LuaSystem>::Register(L);
    Lunar<LuaFrontend>::Register(L);
    Lunar<LuaCube>::Register(L);
    Lunar<LuaRuntime>::Register(L);
    Lunar<LuaFilesystem>::Register(L);
}

LuaScript::~LuaScript()
{
    lua_close(L);
}

int LuaScript::runFile(const char *filename)
{
    if (luaL_dofile(L, filename)) {
        fprintf(stderr, "-!- Error: %s\n", lua_tostring(L, -1));
        lua_pop(L, 1);
        return 1;
    }

    return 0;
}

int LuaScript::runString(const char *str)
{
    if (luaL_dostring(L, str)) {
        fprintf(stderr, "-!- Error: %s\n", lua_tostring(L, -1));
        lua_pop(L, 1);
        return 1;
    }

    return 0;
}

bool LuaScript::argBegin(lua_State *L, const char *className)
{
    /*
     * Before iterating over table arguments, see if the table is even valid
     */

    if (!lua_istable(L, 1)) {
        luaL_error(L, "%s{} argument is not a table. Did you use () instead of {}?",
                   className);
        return false;
    }

    return true;
}

bool LuaScript::argMatch(lua_State *L, const char *arg)
{
    /*
     * See if we can extract an argument named 'arg' from the table.
     * If we can, returns 'true' with the named argument at the top of
     * the stack. If not, returns 'false'.
     */

    lua_pushstring(L, arg);
    lua_gettable(L, 1);

    if (lua_isnil(L, -1)) {
        lua_pop(L, 1);
        return false;
    }

    lua_pushstring(L, arg);
    lua_pushnil(L);
    lua_settable(L, 1);

    return true;
}
 
bool LuaScript::argMatch(lua_State *L, lua_Integer arg)
{
    lua_pushinteger(L, arg);
    lua_gettable(L, 1);

    if (lua_isnil(L, -1)) {
        lua_pop(L, 1);
        return false;
    }

    lua_pushinteger(L, arg);
    lua_pushnil(L);
    lua_settable(L, 1);

    return true;
}
  
bool LuaScript::argEnd(lua_State *L)
{
    /*
     * Finish iterating over an argument table.
     * If there are any unused arguments, turn them into errors.
     */

    bool success = true;

    lua_pushnil(L);
    while (lua_next(L, 1)) {
        lua_pushvalue(L, -2);   // Make a copy of the key object
        luaL_error(L, "Unrecognized parameter (%s)", lua_tostring(L, -1));
        lua_pop(L, 2);          // Pop value and key-copy.
        success = false;
    }

    return success;
}
