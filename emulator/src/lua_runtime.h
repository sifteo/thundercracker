/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * Sifteo Thundercracker simulator
 * M. Elizabeth Scott <beth@sifteo.com>
 *
 * Copyright <c> 2012 Sifteo, Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
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
