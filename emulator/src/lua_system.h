/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * Sifteo Thundercracker simulator
 * M. Elizabeth Scott <beth@sifteo.com>
 *
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#ifndef _LUA_SYSTEM_H
#define _LUA_SYSTEM_H

#include "lua_script.h"


class LuaSystem {
public:
    static const char className[];
    static Lunar<LuaSystem>::RegType methods[];

    LuaSystem(lua_State *L);
    static System *sys;
    
private:
    int init(lua_State *L);
    int start(lua_State *L);
    int exit(lua_State *L);
    
    int setOptions(lua_State *L);
    int setTraceMode(lua_State *L);
    int setAssetLoaderBypass(lua_State *L);

    int numCubes(lua_State *L);

    int vclock(lua_State *L);
    int vsleep(lua_State *L);
    int sleep(lua_State *L);
};

#endif
