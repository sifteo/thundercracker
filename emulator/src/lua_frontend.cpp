/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * Sifteo Thundercracker simulator
 * M. Elizabeth Scott <beth@sifteo.com>
 *
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */
 
#include "lua_script.h"
#include "lua_frontend.h"
#include "lua_system.h"

const char LuaFrontend::className[] = "Frontend";

Lunar<LuaFrontend>::RegType LuaFrontend::methods[] = {
    LUNAR_DECLARE_METHOD(LuaFrontend, init),
    LUNAR_DECLARE_METHOD(LuaFrontend, runFrame),
    LUNAR_DECLARE_METHOD(LuaFrontend, exit),
    LUNAR_DECLARE_METHOD(LuaFrontend, postMessage),    
    {0,0}
};


LuaFrontend::LuaFrontend(lua_State *L) {}

int LuaFrontend::init(lua_State *L)
{
    if (!fe.init(LuaSystem::sys)) {
        lua_pushfstring(L, "failed to initialize Frontend");
        lua_error(L);
    }
    return 0;
}
 
int LuaFrontend::runFrame(lua_State *L)
{
    lua_pushboolean(L, fe.runFrame());
    return 1;
}

int LuaFrontend::exit(lua_State *L)
{
    fe.exit();
    return 0;
}

int LuaFrontend::postMessage(lua_State *L)
{
    fe.postMessage(lua_tostring(L, -1));
    return 0;
}
