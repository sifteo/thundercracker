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
#include "svmruntime.h"
#include "svmloader.h"
#include "svmdebugpipe.h"

const char LuaRuntime::className[] = "Runtime";
const char LuaRuntime::callbackHostField[] = "__runtime_callbackHost";

Lunar<LuaRuntime>::RegType LuaRuntime::methods[] = {
    LUNAR_DECLARE_METHOD(LuaRuntime, onFault),
    LUNAR_DECLARE_METHOD(LuaRuntime, faultString),
    LUNAR_DECLARE_METHOD(LuaRuntime, formatAddress),
    LUNAR_DECLARE_METHOD(LuaRuntime, poke),
    LUNAR_DECLARE_METHOD(LuaRuntime, peek),
    LUNAR_DECLARE_METHOD(LuaRuntime, getPC),
    LUNAR_DECLARE_METHOD(LuaRuntime, getSP),
    LUNAR_DECLARE_METHOD(LuaRuntime, getFP),
    LUNAR_DECLARE_METHOD(LuaRuntime, getGPR),
    LUNAR_DECLARE_METHOD(LuaRuntime, branch),
    LUNAR_DECLARE_METHOD(LuaRuntime, setGPR),
    LUNAR_DECLARE_METHOD(LuaRuntime, runningVolume),
    LUNAR_DECLARE_METHOD(LuaRuntime, previousVolume),
    LUNAR_DECLARE_METHOD(LuaRuntime, flashToVirtAddr),
    LUNAR_DECLARE_METHOD(LuaRuntime, virtToFlashAddr),
    {0,0}
};


LuaRuntime::LuaRuntime(lua_State *L)
{
    /*
     * The system objects backing the Runtime are singletons, so there's
     * nothing to do here for the system yet. We do, however, want to mark
     * this as the default instance for callback dispatch, though.
     */

    Lunar<LuaRuntime>::push(L, this, true);
    lua_setfield(L, LUA_GLOBALSINDEX, callbackHostField);
}

lua_State *LuaRuntime::callbackHost()
{
    // Get the current LuaRuntime instance to use for callback dispatch, if any.
    // On success, returns a lua_State with the Runtime instance at top of stack.

    LuaScript *script = LuaScript::callbackHost();
    if (!script)
        return 0;

    lua_getfield(script->L, LUA_GLOBALSINDEX, callbackHostField);
    if (lua_isnil(script->L, -1)) {
        lua_pop(script->L, 1);
        return 0;
    }
    return script->L;
}

bool LuaRuntime::onFault(unsigned fault)
{
    // Returns 'true' if the fault is handled (absorbed)
    lua_State *L = callbackHost();
    if (L) {
        lua_pushinteger(L, fault);
        int ret = Lunar<LuaRuntime>::call(L, "onFault", 1, 1);
        ASSERT(ret == 1 || ret == -1);
        if (ret < 0) {
            LuaScript::handleError(L, "callback");
        } else {
            bool result = lua_toboolean(L, 1);
            lua_pop(L, 1);
            return result;
        }
    }
    return false;
}

int LuaRuntime::onFault(lua_State *L)
{
    // Default implementation (no-op)
    return 0;
}

int LuaRuntime::faultString(lua_State *L)
{
    Svm::FaultCode code = Svm::FaultCode(luaL_checkinteger(L, 1));
    lua_pushstring(L, Svm::faultString(code));
    return 1;
}

int LuaRuntime::formatAddress(lua_State *L)
{
    std::string str = SvmDebugPipe::formatAddress(luaL_checkinteger(L, 1));
    lua_pushstring(L, str.c_str());
    return 1;
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

int LuaRuntime::peek(lua_State *L)
{
    SvmMemory::VirtAddr va = luaL_checkinteger(L, 1);
    SvmMemory::PhysAddr pa;
    uint32_t value;

    if (!SvmMemory::mapRAM(va, sizeof value, pa)) {
        lua_pushfstring(L, "invalid RAM address");
        lua_error(L);
        return 0;
    }

    value = *reinterpret_cast<uint32_t*>(pa);
    lua_pushinteger(L, value);

    return 1;
}

int LuaRuntime::getPC(lua_State *L)
{
    lua_pushinteger(L, SvmRuntime::reconstructCodeAddr(SvmCpu::reg(REG_PC)));
    return 1;
}

int LuaRuntime::getSP(lua_State *L)
{
    lua_pushinteger(L, SvmMemory::physToVirtRAM(SvmCpu::reg(REG_SP)));
    return 1;
}

int LuaRuntime::getFP(lua_State *L)
{
    lua_pushinteger(L, SvmMemory::physToVirtRAM(SvmCpu::reg(REG_FP)));
    return 1;
}

int LuaRuntime::getGPR(lua_State *L)
{
    lua_pushinteger(L, SvmCpu::reg07(luaL_checkinteger(L, 1)));
    return 1;
}

int LuaRuntime::branch(lua_State *L)
{
    SvmRuntime::branch(luaL_checkinteger(L, 1));
    return 0;
}

int LuaRuntime::setGPR(lua_State *L)
{
    SvmCpu::setReg07(luaL_checkinteger(L, 1), luaL_checkinteger(L, 2));
    return 0;
}

int LuaRuntime::runningVolume(lua_State *L)
{
    lua_pushinteger(L, SvmLoader::getRunningVolume().block.code);
    return 1;
}

int LuaRuntime::previousVolume(lua_State *L)
{
    lua_pushinteger(L, SvmLoader::getPreviousVolume().block.code);
    return 1;
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
