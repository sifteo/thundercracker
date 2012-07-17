/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * Sifteo Thundercracker simulator
 * M. Elizabeth Scott <beth@sifteo.com>
 *
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#include "lua_script.h"
#include "lua_runtime.h"
#include "svmmemory.h"

const char LuaRuntime::className[] = "Runtime";

Lunar<LuaRuntime>::RegType LuaRuntime::methods[] = {
    LUNAR_DECLARE_METHOD(LuaRuntime, poke),
    LUNAR_DECLARE_METHOD(LuaRuntime, flashToVirtAddr),
    LUNAR_DECLARE_METHOD(LuaRuntime, virtToFlashAddr),
    {0,0}
};


LuaRuntime::LuaRuntime(lua_State *L)
{
    /* Nothing to do; the SVM runtime is a singleton */
}

int LuaRuntime::poke(lua_State *L)
{
    SvmMemory::VirtAddr va = luaL_checkinteger(L, 1);
    SvmMemory::PhysAddr pa;
    uint32_t value = luaL_checkinteger(L, 2);

    if (!SvmMemory::mapRAM(va, sizeof value, pa)) {
        lua_pushfstring(L, "invalid RAM address");
        lua_error(L);
        return 0;
    }

    *reinterpret_cast<uint32_t*>(pa) = value;
    return 0;
}

int LuaRuntime::virtToFlashAddr(lua_State *L)
{
    SvmMemory::VirtAddr va = luaL_checkinteger(L, 1);
    lua_pushinteger(L, SvmMemory::virtToFlashAddr(va));
    return 1;
}

int LuaRuntime::flashToVirtAddr(lua_State *L)
{
    uint32_t fa = luaL_checkinteger(L, 1);
    lua_pushinteger(L, SvmMemory::flashToVirtAddr(fa));
    return 1;
}
