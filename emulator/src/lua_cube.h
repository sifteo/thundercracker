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

#ifndef _LUA_CUBE_H
#define _LUA_CUBE_H

#include "lua_script.h"


class LuaCube {
public:
    static const char className[];
    static Lunar<LuaCube>::RegType methods[];
    
    LuaCube(lua_State *L);
    unsigned id;
    
private:
    int reset(lua_State *L);
    int isDebugging(lua_State *L);
    int lcdFrameCount(lua_State *L);
    int lcdPixelCount(lua_State *L);
    int exceptionCount(lua_State *L);
    int getNeighborID(lua_State *L);

    /*
     * Radio
     */
    
    int getRadioAddress(lua_State *L);
    int handleRadioPacket(lua_State *L);
      
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
     * Factory test interface
     */

    int testSetEnabled(lua_State *L);
    int testGetACK(lua_State *L);
    int testWrite(lua_State *L);

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

    // nvm
    int nbPoke(lua_State *L);
    int nbPeek(lua_State *L);
};

#endif
