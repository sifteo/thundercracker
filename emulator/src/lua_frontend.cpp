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
