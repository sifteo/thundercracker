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

#ifndef _LUA_FILESYSTEM_H
#define _LUA_FILESYSTEM_H

#include "lua_script.h"


class LuaFilesystem {
public:
    static const char className[];
    static Lunar<LuaFilesystem>::RegType methods[];

    LuaFilesystem(lua_State *L);

    static void onRawRead(uint32_t address, const uint8_t *buf, unsigned len);
    static void onRawWrite(uint32_t address, const uint8_t *buf, unsigned len);
    static void onRawErase(uint32_t address);

private:
    static const char callbackHostField[];
    static lua_State *callbackHost();
    static bool callbacksEnabled;

    int newVolume(lua_State *L);
    int listVolumes(lua_State *L);
    int deleteVolume(lua_State *L);

    int volumeType(lua_State *L);
    int volumeParent(lua_State *L);
    int volumeMap(lua_State *L);
    int volumeEraseCounts(lua_State *L);
    int volumePayload(lua_State *L);

    int simulatedBlockEraseCounts(lua_State *L);

    int rawRead(lua_State *L);
    int rawWrite(lua_State *L);
    int rawErase(lua_State *L);

    int invalidateCache(lua_State *L);

    int setCallbacksEnabled(lua_State *L);
    int onRawRead(lua_State *L);
    int onRawWrite(lua_State *L);
    int onRawErase(lua_State *L);

    int readMetadata(lua_State *L);
    int readObject(lua_State *L);
    int writeObject(lua_State *L);
};


#endif
