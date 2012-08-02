/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * Sifteo Thundercracker simulator
 * M. Elizabeth Scott <beth@sifteo.com>
 *
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */
 
#include <stdio.h>
#include "lua_script.h"
#include "lua_cube.h"
#include "lua_system.h"
#include "lodepng.h"
#include "color.h"
#include "svmmemory.h"
#include "cubeslots.h"
#include "ostime.h"

const char LuaCube::className[] = "Cube";

Lunar<LuaCube>::RegType LuaCube::methods[] = {
    LUNAR_DECLARE_METHOD(LuaCube, reset),
    LUNAR_DECLARE_METHOD(LuaCube, isDebugging),
    LUNAR_DECLARE_METHOD(LuaCube, lcdFrameCount),
    LUNAR_DECLARE_METHOD(LuaCube, lcdPixelCount),
    LUNAR_DECLARE_METHOD(LuaCube, exceptionCount),
    LUNAR_DECLARE_METHOD(LuaCube, getNeighborID),
    LUNAR_DECLARE_METHOD(LuaCube, getRadioAddress),
    LUNAR_DECLARE_METHOD(LuaCube, handleRadioPacket),
    LUNAR_DECLARE_METHOD(LuaCube, saveScreenshot),
    LUNAR_DECLARE_METHOD(LuaCube, testScreenshot),
    LUNAR_DECLARE_METHOD(LuaCube, testSetEnabled),
    LUNAR_DECLARE_METHOD(LuaCube, testGetACK),
    LUNAR_DECLARE_METHOD(LuaCube, testWrite),
    LUNAR_DECLARE_METHOD(LuaCube, xbPoke),
    LUNAR_DECLARE_METHOD(LuaCube, xwPoke),
    LUNAR_DECLARE_METHOD(LuaCube, xbPeek),
    LUNAR_DECLARE_METHOD(LuaCube, xwPeek),
    LUNAR_DECLARE_METHOD(LuaCube, ibPoke),
    LUNAR_DECLARE_METHOD(LuaCube, ibPeek),
    LUNAR_DECLARE_METHOD(LuaCube, fwPoke),
    LUNAR_DECLARE_METHOD(LuaCube, fbPoke),
    LUNAR_DECLARE_METHOD(LuaCube, fwPeek),
    LUNAR_DECLARE_METHOD(LuaCube, fbPeek),
    LUNAR_DECLARE_METHOD(LuaCube, nbPoke),
    LUNAR_DECLARE_METHOD(LuaCube, nbPeek),
    {0,0}
};


LuaCube::LuaCube(lua_State *L)
    : id(0)
{
    lua_Integer i = lua_tointeger(L, -1);

    if (i < 0 || i >= (lua_Integer)System::MAX_CUBES) {
        lua_pushfstring(L, "cube ID %d is not valid. Must be between 0 and %d", System::MAX_CUBES);
        lua_error(L);
    } else {
        id = (unsigned) i;
    }
}

int LuaCube::reset(lua_State *L)
{
    LuaSystem::sys->resetCube(id);

    return 0;
}

int LuaCube::isDebugging(lua_State *L)
{
    lua_pushboolean(L, LuaSystem::sys->cubes[id].isDebugging());
    return 1;
}

int LuaCube::lcdFrameCount(lua_State *L)
{
    lua_pushinteger(L, LuaSystem::sys->cubes[id].lcd.getFrameCount());
    return 1;
}

int LuaCube::lcdPixelCount(lua_State *L)
{
    lua_pushinteger(L, LuaSystem::sys->cubes[id].lcd.getPixelCount());
    return 1;
}

int LuaCube::exceptionCount(lua_State *L)
{
    lua_pushinteger(L, LuaSystem::sys->cubes[id].getExceptionCount());
    return 1;
}

int LuaCube::getNeighborID(lua_State *L)
{
    lua_pushinteger(L, LuaSystem::sys->cubes[id].getNeighborID());
    return 1;
}

int LuaCube::xbPoke(lua_State *L)
{
    uint8_t *mem = &LuaSystem::sys->cubes[id].cpu.mExtData[0];
    mem[(XDATA_SIZE - 1) & luaL_checkinteger(L, 1)] = luaL_checkinteger(L, 2);
    return 0;
}

int LuaCube::xwPoke(lua_State *L)
{
    uint16_t *mem = (uint16_t*) &LuaSystem::sys->cubes[id].cpu.mExtData[0];
    mem[(XDATA_SIZE/2 - 1) & luaL_checkinteger(L, 1)] = luaL_checkinteger(L, 2);
    return 0;
}

int LuaCube::xbPeek(lua_State *L)
{
    uint8_t *mem = &LuaSystem::sys->cubes[id].cpu.mExtData[0];
    lua_pushinteger(L, mem[(XDATA_SIZE - 1) & luaL_checkinteger(L, 1)]);
    return 1;
}

int LuaCube::xwPeek(lua_State *L)
{
    uint16_t *mem = (uint16_t*) &LuaSystem::sys->cubes[id].cpu.mExtData[0];
    lua_pushinteger(L, mem[(XDATA_SIZE/2 - 1) & luaL_checkinteger(L, 1)]);
    return 1;
}
    
int LuaCube::ibPoke(lua_State *L)
{
    uint8_t *mem = &LuaSystem::sys->cubes[id].cpu.mData[0];
    mem[0xFF & luaL_checkinteger(L, 1)] = luaL_checkinteger(L, 2);
    return 0;
}

int LuaCube::ibPeek(lua_State *L)
{
    uint8_t *mem = &LuaSystem::sys->cubes[id].cpu.mData[0];
    lua_pushinteger(L, mem[0xFF & luaL_checkinteger(L, 1)]);
    return 1;
}

int LuaCube::fwPoke(lua_State *L)
{
    uint16_t *mem = (uint16_t*) &LuaSystem::sys->cubes[id].flash.getStorage()->ext;
    mem[(Cube::FlashModel::SIZE/2 - 1) & luaL_checkinteger(L, 1)] = luaL_checkinteger(L, 2);
    return 0;
}

int LuaCube::fbPoke(lua_State *L)
{
    uint8_t *mem = (uint8_t*) &LuaSystem::sys->cubes[id].flash.getStorage()->ext;
    mem[(Cube::FlashModel::SIZE - 1) & luaL_checkinteger(L, 1)] = luaL_checkinteger(L, 2);
    return 0;
}

int LuaCube::fwPeek(lua_State *L)
{
    uint16_t *mem = (uint16_t*) &LuaSystem::sys->cubes[id].flash.getStorage()->ext;
    lua_pushinteger(L, mem[(Cube::FlashModel::SIZE/2 - 1) & luaL_checkinteger(L, 1)]);
    return 1;
}

int LuaCube::fbPeek(lua_State *L)
{
    uint8_t *mem = (uint8_t*) &LuaSystem::sys->cubes[id].flash.getStorage()->ext;
    lua_pushinteger(L, mem[(Cube::FlashModel::SIZE - 1) & luaL_checkinteger(L, 1)]);
    return 1;
}

int LuaCube::nbPeek(lua_State *L)
{
    uint8_t *mem = (uint8_t*) &LuaSystem::sys->cubes[id].flash.getStorage()->nvm;
    lua_pushinteger(L, mem[0x3ff & luaL_checkinteger(L, 1)]);
    return 1;
}

int LuaCube::nbPoke(lua_State *L)
{
    uint8_t *mem = (uint8_t*) &LuaSystem::sys->cubes[id].flash.getStorage()->nvm;
    mem[0x3ff & luaL_checkinteger(L, 1)] = luaL_checkinteger(L, 2);
    return 0;
}

int LuaCube::saveScreenshot(lua_State *L)
{    
    const char *filename = luaL_checkstring(L, 1);
    Cube::LCD &lcd = LuaSystem::sys->cubes[id].lcd;
    std::vector<uint8_t> pixels;
    
    for (unsigned i = 0; i < lcd.FB_SIZE; i++) {
        RGB565 color = lcd.fb_mem[i];
        pixels.push_back(color.red());
        pixels.push_back(color.green());
        pixels.push_back(color.blue());
        pixels.push_back(0xFF);
    }

    LodePNG::Encoder encoder;
    std::vector<uint8_t> pngData;
    encoder.encode(pngData, pixels, lcd.WIDTH, lcd.HEIGHT);
    
    LodePNG::saveFile(pngData, filename);
    
    return 0;
}

int LuaCube::testScreenshot(lua_State *L)
{
    const char *filename = luaL_checkstring(L, 1);
    const lua_Integer tolerance = lua_tointeger(L, 2);

    Cube::LCD &lcd = LuaSystem::sys->cubes[id].lcd;
    std::vector<uint8_t> pngData;
    std::vector<uint8_t> pixels;
    LodePNG::Decoder decoder;
    
    LodePNG::loadFile(pngData, filename);
    if (!pngData.empty())
        decoder.decode(pixels, pngData);
    
    if (pixels.empty()) {
        lua_pushfstring(L, "error loading PNG file \"%s\"", filename);
        lua_error(L);
    }
    
    for (unsigned i = 0; i < lcd.FB_SIZE; i++) {
        RGB565 fbColor = lcd.fb_mem[i];
        RGB565 refColor = &pixels[i*4];

        int dR = int(fbColor.red()) - int(refColor.red());
        int dG = int(fbColor.green()) - int(refColor.green());
        int dB = int(fbColor.blue()) - int(refColor.blue());
        int error = dR*dR + dG*dG + dB*dB;

        if (error > tolerance) {
            // Image mismatch. Return (x, y, lcdPixel, refPixel, error)
            lua_pushinteger(L, i % lcd.WIDTH);
            lua_pushinteger(L, i / lcd.WIDTH);
            lua_pushinteger(L, fbColor.value);
            lua_pushinteger(L, refColor.value);
            lua_pushinteger(L, error);
            return 5;
        }
    }
    
    return 0;
}

int LuaCube::handleRadioPacket(lua_State *L)
{
    /*
     * Argument is a radio packet, represented as a string.
     * Returns a string ACK (which may be empty), or no argument
     * if the packet failed to acknowledge.
     */
     
    Cube::Radio &radio = LuaSystem::sys->cubes[id].spi.radio;    
    
    Cube::Radio::Packet packet, reply;
    memset(&packet, 0, sizeof packet);
    memset(&reply, 0, sizeof reply);

    size_t packetStrLen = 0;
    const char *packetStr = lua_tolstring(L, 1, &packetStrLen);
    
    if (packetStrLen > sizeof packet.payload) {
        lua_pushfstring(L, "packet too long");
        lua_error(L);
        return 0;
    }
    
    packet.len = packetStrLen;
    memcpy(packet.payload, packetStr, packetStrLen);
    
    if (radio.handlePacket(packet, reply)) {
        lua_pushlstring(L, (const char *) reply.payload, reply.len);
        return 1;
    }

    return 0;
}

int LuaCube::getRadioAddress(lua_State *L)
{
    /*
     * Takes no arguments, returns the radio's RX address formatted as a string.
     * Our string format matches the debug logging in the master firmware:
     *
     *    <channel> / <byte0> <byte1> <byte2> <byte3> <byte4>
     */

    Cube::Radio &radio = LuaSystem::sys->cubes[id].spi.radio;   
    char buf[20];
    uint64_t addr = radio.getPackedRXAddr();

    snprintf(buf, sizeof buf, "%02x/%02x%02x%02x%02x%02x",
        (uint8_t)(addr >> 56),
        (uint8_t)(addr >> 0),
        (uint8_t)(addr >> 8),
        (uint8_t)(addr >> 16),
        (uint8_t)(addr >> 24),
        (uint8_t)(addr >> 32));
        
    lua_pushstring(L, buf);
    return 1;
}

int LuaCube::testSetEnabled(lua_State *L)
{
    Cube::I2CTestJig &test = LuaSystem::sys->cubes[id].i2c.testjig;
    test.setEnabled(lua_toboolean(L, 1));
    return 0;
}

int LuaCube::testGetACK(lua_State *L)
{
    Cube::I2CTestJig &test = LuaSystem::sys->cubes[id].i2c.testjig;
    std::vector<uint8_t> buffer;
    test.getACK(buffer);
    lua_pushlstring(L, (const char *) &buffer[0], buffer.size());
    return 1;
}

int LuaCube::testWrite(lua_State *L)
{
    Cube::I2CTestJig &test = LuaSystem::sys->cubes[id].i2c.testjig;
    size_t dataStrLen = 0;
    const char *dataStr = lua_tolstring(L, 1, &dataStrLen);
    test.write((const uint8_t*) dataStr, dataStrLen);
    return 0;
}
