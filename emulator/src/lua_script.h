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

#ifndef _lUA_SCRIPT_H
#define _LUA_SCRIPT_H

extern "C" {
#   include "lua/lua.h"
#   include "lua/lauxlib.h"
#   include "lua/lualib.h"
}

#include "frontend.h"
#include "lunar.h"
#include "system.h"


class LuaScript {
public:
    LuaScript(System &sys);
    ~LuaScript();

    int run(const char *filename);

    // Utilities for foolproof table argument unpacking
    static bool argBegin(lua_State *L, const char *className);
    static bool argMatch(lua_State *L, const char *arg);
    static bool argMatch(lua_State *L, lua_Integer arg);
    static bool argEnd(lua_State *L);

 private:
    lua_State *L;
};


class LuaSystem {
public:
    static const char className[];
    static Lunar<LuaSystem>::RegType methods[];

    LuaSystem(lua_State *L);
    static System *sys;
    
private:
    int setOptions(lua_State *L);
    int init(lua_State *L);
    int start(lua_State *L);
    int exit(lua_State *L);
};


class LuaFrontend {
public:
    static const char className[];
    static Lunar<LuaFrontend>::RegType methods[];
    
    LuaFrontend(lua_State *L);
    Frontend fe;

private:
    int init(lua_State *L);
    int runFrame(lua_State *L);
    int exit(lua_State *L);
    int postMessage(lua_State *L);
};

#endif
