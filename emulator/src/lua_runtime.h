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

    static bool onFault(unsigned fault);

private:
    static const char callbackHostField[];
    static lua_State *callbackHost();

    int onFault(lua_State *L);

    int faultString(lua_State *L);
    int formatAddress(lua_State *L);

    int poke(lua_State *L);
    int peek(lua_State *L);

    int getPC(lua_State *L);
    int getSP(lua_State *L);
    int getFP(lua_State *L);
    int getGPR(lua_State *L);

    int branch(lua_State *L);
    int setGPR(lua_State *L);

    int runningVolume(lua_State *L);
    int previousVolume(lua_State *L);

    int virtToFlashAddr(lua_State *L);
    int flashToVirtAddr(lua_State *L);
};

#endif
