/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * Sifteo Thundercracker simulator
 * M. Elizabeth Scott <beth@sifteo.com>
 *
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#include "lua_script.h"
#include "lua_filesystem.h"
#include "flash_device.h"
#include "flash_blockcache.h"
#include "flash_volume.h"
#include "flash_volumeheader.h"

const char LuaFilesystem::className[] = "Filesystem";
const char LuaFilesystem::callbackHostField[] = "__filesystem_callbackHost";
bool LuaFilesystem::callbacksEnabled;

Lunar<LuaFilesystem>::RegType LuaFilesystem::methods[] = {
    LUNAR_DECLARE_METHOD(LuaFilesystem, newVolume),
    LUNAR_DECLARE_METHOD(LuaFilesystem, listVolumes),
    LUNAR_DECLARE_METHOD(LuaFilesystem, deleteVolume),
    LUNAR_DECLARE_METHOD(LuaFilesystem, volumeType),
    LUNAR_DECLARE_METHOD(LuaFilesystem, volumeMap),
    LUNAR_DECLARE_METHOD(LuaFilesystem, volumeEraseCounts),
    LUNAR_DECLARE_METHOD(LuaFilesystem, simulatedSectorEraseCounts),
    LUNAR_DECLARE_METHOD(LuaFilesystem, rawRead),
    LUNAR_DECLARE_METHOD(LuaFilesystem, rawWrite),
    LUNAR_DECLARE_METHOD(LuaFilesystem, rawErase),
    LUNAR_DECLARE_METHOD(LuaFilesystem, invalidateCache),
    LUNAR_DECLARE_METHOD(LuaFilesystem, setCallbacksEnabled),
    LUNAR_DECLARE_METHOD(LuaFilesystem, onRawRead),
    LUNAR_DECLARE_METHOD(LuaFilesystem, onRawWrite),
    LUNAR_DECLARE_METHOD(LuaFilesystem, onRawErase),
    {0,0}
};


LuaFilesystem::LuaFilesystem(lua_State *L)
{
    /*
     * The system objects backing the Filesystem are singletons, so there's
     * nothing to do here for the system yet. We do, however, want to mark
     * this as the default Filesystem instance for callback dispatch, though.
     */

    Lunar<LuaFilesystem>::push(L, this, true);
    lua_setfield(L, LUA_GLOBALSINDEX, callbackHostField);
    callbacksEnabled = false;
}

lua_State *LuaFilesystem::callbackHost()
{
    // Get the current LuaFilesystem instance to use for callback dispatch, if any.
    // On success, returns a lua_State with the Filesystem instance at top of stack.

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

void LuaFilesystem::onRawRead(uint32_t address, const uint8_t *buf, unsigned len)
{
    if (!callbacksEnabled)
        return;

    lua_State *L = callbackHost();
    if (L) {
        lua_pushinteger(L, address);
        lua_pushlstring(L, (const char*)buf, len);
        int ret = Lunar<LuaFilesystem>::call(L, "onRawRead", 2, 0);
        ASSERT(ret == 0 || ret == -1);
        if (ret < 0)
            LuaScript::handleError(L, "callback");
    }
}

void LuaFilesystem::onRawWrite(uint32_t address, const uint8_t *buf, unsigned len)
{
    if (!callbacksEnabled)
        return;

    lua_State *L = callbackHost();
    if (L) {
        lua_pushinteger(L, address);
        lua_pushlstring(L, (const char*)buf, len);
        int ret = Lunar<LuaFilesystem>::call(L, "onRawWrite", 2, 0);
        ASSERT(ret == 0 || ret == -1);
        if (ret < 0)
            LuaScript::handleError(L, "callback");
    }
}

void LuaFilesystem::onRawErase(uint32_t address)
{
    if (!callbacksEnabled)
        return;

    lua_State *L = callbackHost();
    if (L) {
        lua_pushinteger(L, address);
        int ret = Lunar<LuaFilesystem>::call(L, "onRawErase", 1, 0);
        ASSERT(ret == 0 || ret == -1);
        if (ret < 0)
            LuaScript::handleError(L, "callback");
    }
}

int LuaFilesystem::newVolume(lua_State *L)
{
    /*
     * Arguments: (type, payload data, type-specific data, parent).
     * Type-specific data is optional, and will be zero-length if omitted.
     * Parent is optional, will be unset (zero) if omitted.
     *
     * Creates, writes, and commits the new volume. Returns its block code
     * on success, or nil on failure.
     */

    size_t payloadStrLen = 0;
    size_t dataStrLen = 0;
    unsigned type = luaL_checkinteger(L, 1);
    const char *payloadStr = lua_tolstring(L, 2, &payloadStrLen);
    const char *dataStr = lua_tolstring(L, 3, &dataStrLen);
    FlashMapBlock parent = FlashMapBlock::fromCode(lua_tointeger(L, 4));

    FlashVolumeWriter writer;
    if (!writer.begin(type, payloadStrLen, dataStrLen, parent))
        return 0;

    writer.appendPayload((const uint8_t*)payloadStr, payloadStrLen);
    writer.commit();

    lua_pushinteger(L, writer.volume.block.code);
    return 1;
}

int LuaFilesystem::listVolumes(lua_State *L)
{
    /*
     * Takes no arguments. Returns an array of volume block codes.
     */

    lua_newtable(L);

    FlashVolumeIter vi;
    FlashVolume vol;
    unsigned index = 0;

    vi.begin();
    while (vi.next(vol)) {
        lua_pushnumber(L, ++index);
        lua_pushnumber(L, vol.block.code);
        lua_settable(L, -3);
    }

    return 1;
}

int LuaFilesystem::deleteVolume(lua_State *L)
{
    /*
     * Given a volume block code, mark the volume as deleted.
     */

    unsigned code = luaL_checkinteger(L, 1);
    FlashVolume vol(FlashMapBlock::fromCode(code));
    if (!vol.isValid()) {
        lua_pushfstring(L, "invalid volume");
        lua_error(L);
        return 0;
    }

    vol.deleteTree();

    return 0;
}

int LuaFilesystem::volumeType(lua_State *L)
{
    /*
     * Given a volume block code, return the volume's type.
     */

    unsigned code = luaL_checkinteger(L, 1);
    FlashVolume vol(FlashMapBlock::fromCode(code));
    if (!vol.isValid()) {
        lua_pushfstring(L, "invalid volume");
        lua_error(L);
        return 0;
    }

    lua_pushnumber(L, vol.getType());
    return 1;
}

int LuaFilesystem::volumeMap(lua_State *L)
{
    /*
     * Given a volume block code, return the volume's map
     * as an array of block codes.
     */

    unsigned code = luaL_checkinteger(L, 1);
    FlashVolume vol(FlashMapBlock::fromCode(code));
    if (!vol.isValid()) {
        lua_pushfstring(L, "invalid volume");
        lua_error(L);
        return 0;
    }

    FlashBlockRef ref;
    FlashVolumeHeader *hdr = FlashVolumeHeader::get(ref, vol.block);
    const FlashMap *map = hdr->getMap();

    lua_newtable(L);

    for (unsigned I = 0, E = hdr->numMapEntries(); I != E; ++I) {
        lua_pushnumber(L, I + 1);
        lua_pushnumber(L, map->blocks[I].code);
        lua_settable(L, -3);
    }

    return 1;
}

int LuaFilesystem::volumeEraseCounts(lua_State *L)
{
    /*
     * Given a volume block code, return the volume's array of erase counts.
     */

    unsigned code = luaL_checkinteger(L, 1);
    FlashVolume vol(FlashMapBlock::fromCode(code));
    if (!vol.isValid()) {
        lua_pushfstring(L, "invalid volume");
        lua_error(L);
        return 0;
    }

    FlashBlockRef hdrRef, eraseRef;
    FlashVolumeHeader *hdr = FlashVolumeHeader::get(hdrRef, vol.block);
    unsigned numMapEntries = hdr->numMapEntries();

    lua_newtable(L);

    for (unsigned I = 0; I != numMapEntries; ++I) {
        lua_pushnumber(L, I + 1);
        lua_pushnumber(L, hdr->getEraseCount(eraseRef, vol.block, I, numMapEntries));
        lua_settable(L, -3);
    }

    return 1;
}

int LuaFilesystem::simulatedSectorEraseCounts(lua_State *L)
{
    /*
     * No parameters. Returns a table of simulated erase counts for the
     * master flash memory, one per sector.
     */

    FlashStorage::MasterRecord &storage = SystemMC::getSystem()->flash.data->master;

    lua_newtable(L);

    for (unsigned i = 0; i != arraysize(storage.eraseCounts); ++i) {
        lua_pushnumber(L, i + 1);
        lua_pushnumber(L, storage.eraseCounts[i]);
        lua_settable(L, -3);
    }

    return 1;
}

int LuaFilesystem::rawRead(lua_State *L)
{
    /*
     * Raw flash read. (address, size) -> (data)
     */

    unsigned addr = luaL_checkinteger(L, 1);
    unsigned size = luaL_checkinteger(L, 2);

    if (addr > FlashDevice::CAPACITY ||
        size > FlashDevice::CAPACITY ||
        addr + size > FlashDevice::CAPACITY) {
        lua_pushfstring(L, "Flash memory address and/or size out of range");
        lua_error(L);
        return 0;
    }

    uint8_t *buffer = new uint8_t[size];
    ASSERT(buffer);

    FlashDevice::read(addr, buffer, size);

    lua_pushlstring(L, (const char *) buffer, size);
    delete buffer;
    return 1;
}

int LuaFilesystem::rawWrite(lua_State *L)
{
    /*
     * Raw flash write. (address, data)
     */

    size_t size = 0;
    unsigned addr = luaL_checkinteger(L, 1);
    const char *data = lua_tolstring(L, 2, &size);

    if (addr > FlashDevice::CAPACITY ||
        size > FlashDevice::CAPACITY ||
        addr + size > FlashDevice::CAPACITY) {
        lua_pushfstring(L, "Flash memory address and/or size out of range");
        lua_error(L);
        return 0;
    }

    FlashDevice::write(addr, (const uint8_t*) data, size);
    return 0;
}

int LuaFilesystem::rawErase(lua_State *L)
{
    /*
     * Raw flash sector-erase (address)
     */

    unsigned addr = luaL_checkinteger(L, 1);

    if (addr >= FlashDevice::CAPACITY ||
        (addr % FlashDevice::SECTOR_SIZE)) {
        lua_pushfstring(L, "Not a valid sector-aligned flash address");
        lua_error(L);
        return 0;
    }

    FlashDevice::eraseSector(addr);
    return 0;
}

int LuaFilesystem::invalidateCache(lua_State *L)
{
    /*
     * Invalidate the block cache. No parameters.
     */

    FlashBlock::invalidate();
    return 0;
}

int LuaFilesystem::setCallbacksEnabled(lua_State *L)
{
    callbacksEnabled = lua_toboolean(L, 1);
    return 0;
}

int LuaFilesystem::onRawRead(lua_State *L)
{
    // Default implementation (no-op)
    return 0;
}

int LuaFilesystem::onRawWrite(lua_State *L)
{
    // Default implementation (no-op)
    return 0;
}

int LuaFilesystem::onRawErase(lua_State *L)
{
    // Default implementation (no-op)
    return 0;
}
