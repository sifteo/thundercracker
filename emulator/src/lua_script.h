/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * Sifteo Thundercracker simulator
 * M. Elizabeth Scott <beth@sifteo.com>
 *
 * Copyright <c> 2011 Sifteo, Inc. All rights reserved.
 */
 
/*
 * Lua scripting support
 */

#ifndef _lUA_SCRIPT_H
#define _LUA_SCRIPT_H

extern "C" {
#   include "lua/lua.h"
#   include "lua/lauxlib.h"
#   include "lua/lualib.h"
}

#include "frontend.h"
#include "lunar.h"
#include "system.h"


class LuaScript {
public:
    LuaScript(System &sys);
    ~LuaScript();

    int run(const char *filename);

    // Utilities for foolproof table argument unpacking
    static bool argBegin(lua_State *L, const char *className);
    static bool argMatch(lua_State *L, const char *arg);
    static bool argMatch(lua_State *L, lua_Integer arg);
    static bool argEnd(lua_State *L);

 private:
    lua_State *L;
};


class LuaSystem {
public:
    static const char className[];
    static Lunar<LuaSystem>::RegType methods[];

    LuaSystem(lua_State *L);
    static System *sys;
    
private:
    int init(lua_State *L);
    int start(lua_State *L);
    int exit(lua_State *L);
    
    int setOptions(lua_State *L);
    int setTraceMode(lua_State *L);
    
    int vclock(lua_State *L);
    int vsleep(lua_State *L);
    int sleep(lua_State *L);
};


class LuaFrontend {
public:
    static const char className[];
    static Lunar<LuaFrontend>::RegType methods[];
    
    LuaFrontend(lua_State *L);
    Frontend fe;

private:
    int init(lua_State *L);
    int runFrame(lua_State *L);
    int exit(lua_State *L);
    int postMessage(lua_State *L);
};


class LuaCube {
public:
    static const char className[];
    static Lunar<LuaCube>::RegType methods[];
    
    LuaCube(lua_State *L);
    unsigned id;
    
private:
    int reset(lua_State *L);
    int isDebugging(lua_State *L);
    int lcdWriteCount(lua_State *L);
    int lcdPixelCount(lua_State *L);
    int exceptionCount(lua_State *L);
      
    /*
     * LCD screenshots
     *
     * We can save a screenshot to PNG, or compare a PNG with the
     * current LCD contents. Requires an exact match. On success,
     * returns nil. On error, returns (x, y, lcdColor, refColor)
     * to describe the mismatch.
     */
     
    int saveScreenshot(lua_State *L);
    int testScreenshot(lua_State *L);
    
    /*
     * Peek/poke operators for different memory regions and sizes.
     * Note that these are all asynchronous, since we run in a
     * different thread than the emulator.
     *
     * The word-wide functions always take word addresses, and
     * byte-wide functions take byte addresses.
     */
    
    // xram
    int xbPoke(lua_State *L);
    int xwPoke(lua_State *L);
    int xbPeek(lua_State *L);
    int xwPeek(lua_State *L);
    
    // iram
    int ibPoke(lua_State *L);
    int ibPeek(lua_State *L);
    
    // flash
    int fwPoke(lua_State *L);
    int fbPoke(lua_State *L);
    int fwPeek(lua_State *L);
    int fbPeek(lua_State *L);
};


#endif
