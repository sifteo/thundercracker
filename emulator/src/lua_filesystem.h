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

private:
    int newVolume(lua_State *L);
    int listVolumes(lua_State *L);
    int deleteVolume(lua_State *L);

    int volumeType(lua_State *L);
    int volumeMap(lua_State *L);
    int volumeEraseCounts(lua_State *L);

    int simulatedSectorEraseCounts(lua_State *L);

    int rawRead(lua_State *L);
    int rawWrite(lua_State *L);
    int rawErase(lua_State *L);

    int invalidateCache(lua_State *L);
};


#endif
