/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * Sifteo Thundercracker simulator
 * M. Elizabeth Scott <beth@sifteo.com>
 *
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#ifndef _LUA_FRONTEND_H
#define _LUA_FRONTEND_H

#include "lua_script.h"
#include "frontend.h"


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
