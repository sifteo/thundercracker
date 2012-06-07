/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * Sifteo Thundercracker simulator
 * M. Elizabeth Scott <beth@sifteo.com>
 *
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#ifndef _LUA_RUNTIME_H
#define _LUA_RUNTIME_H

#include "lua_script.h"


class LuaRuntime {
public:
    static const char className[];
    static Lunar<LuaRuntime>::RegType methods[];

    LuaRuntime(lua_State *L);

private:
    int poke(lua_State *L);
    int virtToFlashAddr(lua_State *L);
    int flashToVirtAddr(lua_State *L);
};

#endif
