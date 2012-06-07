/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * Sifteo Thundercracker simulator
 * M. Elizabeth Scott <beth@sifteo.com>
 *
 * Copyright <c> 2011 Sifteo, Inc. All rights reserved.
 */
 
/*
 * Lua scripting support
 */

#ifndef _LUA_SCRIPT_H
#define _LUA_SCRIPT_H

extern "C" {
#   include "lua/lua.h"
#   include "lua/lauxlib.h"
#   include "lua/lualib.h"
}

#include "lunar.h"
#include "system.h"


class LuaScript {
public:
    LuaScript(System &sys);
    ~LuaScript();

    int runFile(const char *filename);
    int runString(const char *str);

    // Utilities for foolproof table argument unpacking
    static bool argBegin(lua_State *L, const char *className);
    static bool argMatch(lua_State *L, const char *arg);
    static bool argMatch(lua_State *L, lua_Integer arg);
    static bool argEnd(lua_State *L);

    // Get the default LuaScript instance for callbacks to run in
    static LuaScript *callbackHost() {
        return mCallbackHost;
    }

    static void handleError(lua_State *L, const char *context);

    lua_State *L;

 private:
    static LuaScript *mCallbackHost;
};

#endif
