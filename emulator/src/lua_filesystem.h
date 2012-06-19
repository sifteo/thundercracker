/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * Sifteo Thundercracker simulator
 * M. Elizabeth Scott <beth@sifteo.com>
 *
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
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

    int simulatedSectorEraseCounts(lua_State *L);

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
