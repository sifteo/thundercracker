/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * Sifteo Thundercracker simulator
 * Micah Elizabeth Scott <micah@misc.name>
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
 
#include "lua_script.h"
#include "lua_system.h"
#include "ostime.h"
#include "assetloader.h"

System *LuaSystem::sys = NULL;
const char LuaSystem::className[] = "System";


Lunar<LuaSystem>::RegType LuaSystem::methods[] = {
    LUNAR_DECLARE_METHOD(LuaSystem, init),
    LUNAR_DECLARE_METHOD(LuaSystem, start),
    LUNAR_DECLARE_METHOD(LuaSystem, exit),
    LUNAR_DECLARE_METHOD(LuaSystem, setOptions),
    LUNAR_DECLARE_METHOD(LuaSystem, setTraceMode),
    LUNAR_DECLARE_METHOD(LuaSystem, setAssetLoaderBypass),
    LUNAR_DECLARE_METHOD(LuaSystem, vclock),
    LUNAR_DECLARE_METHOD(LuaSystem, vsleep),
    LUNAR_DECLARE_METHOD(LuaSystem, sleep),
    LUNAR_DECLARE_METHOD(LuaSystem, numCubes),
    {0,0}
};


LuaSystem::LuaSystem(lua_State *L)
{
    /* Nothing to do; our System object is a singleton provided by main.cpp */
}

int LuaSystem::setOptions(lua_State *L)
{
    if (!LuaScript::argBegin(L, className))
        return 0;

    if (LuaScript::argMatch(L, "numCubes"))
        sys->opt_numCubes = lua_tointeger(L, -1);

    if (LuaScript::argMatch(L, "cubeFirmware"))
        sys->opt_cubeFirmware = lua_tostring(L, -1);

    if (LuaScript::argMatch(L, "turbo"))
        sys->opt_turbo = lua_toboolean(L, -1);

    if (LuaScript::argMatch(L, "continueOnException"))
        sys->opt_continueOnException = lua_toboolean(L, -1);

    if (LuaScript::argMatch(L, "cube0Debug"))
        sys->opt_cube0Debug = lua_toboolean(L, -1);

    if (LuaScript::argMatch(L, "cube0Profile"))
        sys->opt_cube0Profile = lua_tostring(L, -1);

    if (LuaScript::argMatch(L, "paintTrace"))
        sys->opt_paintTrace = lua_toboolean(L, -1);

    if (LuaScript::argMatch(L, "radioTrace"))
        sys->opt_radioTrace = lua_toboolean(L, -1);

    if (LuaScript::argMatch(L, "svmTrace"))
        sys->opt_svmTrace = lua_toboolean(L, -1);

    if (LuaScript::argMatch(L, "svmFlashStats"))
        sys->opt_svmFlashStats = lua_toboolean(L, -1);

    if (LuaScript::argMatch(L, "svmStackMonitor"))
        sys->opt_svmStackMonitor = lua_toboolean(L, -1);

    if (LuaScript::argMatch(L, "noCubeReconnect"))
        sys->opt_noCubeReconnect = lua_toboolean(L, -1);

    if (!LuaScript::argEnd(L))
        return 0;

    return 0;
}

int LuaSystem::numCubes(lua_State *L)
{
    lua_pushinteger(L, sys->opt_numCubes);
    return 1;
}

int LuaSystem::setTraceMode(lua_State *L)
{
    sys->tracer.setEnabled(lua_toboolean(L, 1));
    return 0;
}

int LuaSystem::setAssetLoaderBypass(lua_State *L)
{
    AssetLoader::simBypass = lua_toboolean(L, 1);
    return 0;
}

int LuaSystem::vclock(lua_State *L)
{
    /*
     * Read the virtual-time clock, in seconds
     */
    lua_pushnumber(L, sys->time.elapsedSeconds());
    return 1;
}

int LuaSystem::sleep(lua_State *L)
{
    OSTime::sleep(luaL_checknumber(L, 1));
    return 0;
}
    
int LuaSystem::vsleep(lua_State *L)
{
    /*
     * Sleep, in virtual time
     */

    double deadline = sys->time.elapsedSeconds() + luaL_checknumber(L, 1);
    double remaining;
    
    while ((remaining = deadline - sys->time.elapsedSeconds()) > 0)
        OSTime::sleep(remaining * 0.1);

    return 0;
}

int LuaSystem::init(lua_State *L)
{
    /*
     * Initialize the system, with current options
     */
    if (!sys->init()) {
        lua_pushfstring(L, "failed to initialize System");
        lua_error(L);
    }
    return 0;
}
 
int LuaSystem::start(lua_State *L)
{
    sys->start();
    return 0;
}

int LuaSystem::exit(lua_State *L)
{
    sys->exit();
    return 0;
}
