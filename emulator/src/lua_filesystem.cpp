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
#include "flash_lfs.h"
#include "elfprogram.h"

const char LuaFilesystem::className[] = "Filesystem";
const char LuaFilesystem::callbackHostField[] = "__filesystem_callbackHost";
bool LuaFilesystem::callbacksEnabled;

Lunar<LuaFilesystem>::RegType LuaFilesystem::methods[] = {
    LUNAR_DECLARE_METHOD(LuaFilesystem, newVolume),
    LUNAR_DECLARE_METHOD(LuaFilesystem, listVolumes),
    LUNAR_DECLARE_METHOD(LuaFilesystem, deleteVolume),
    LUNAR_DECLARE_METHOD(LuaFilesystem, volumeType),
    LUNAR_DECLARE_METHOD(LuaFilesystem, volumeParent),
    LUNAR_DECLARE_METHOD(LuaFilesystem, volumeMap),
    LUNAR_DECLARE_METHOD(LuaFilesystem, volumeEraseCounts),
    LUNAR_DECLARE_METHOD(LuaFilesystem, volumePayload),
    LUNAR_DECLARE_METHOD(LuaFilesystem, simulatedSectorEraseCounts),
    LUNAR_DECLARE_METHOD(LuaFilesystem, rawRead),
    LUNAR_DECLARE_METHOD(LuaFilesystem, rawWrite),
    LUNAR_DECLARE_METHOD(LuaFilesystem, rawErase),
    LUNAR_DECLARE_METHOD(LuaFilesystem, invalidateCache),
    LUNAR_DECLARE_METHOD(LuaFilesystem, setCallbacksEnabled),
    LUNAR_DECLARE_METHOD(LuaFilesystem, onRawRead),
    LUNAR_DECLARE_METHOD(LuaFilesystem, onRawWrite),
    LUNAR_DECLARE_METHOD(LuaFilesystem, onRawErase),
    LUNAR_DECLARE_METHOD(LuaFilesystem, readMetadata),
    LUNAR_DECLARE_METHOD(LuaFilesystem, readObject),
    LUNAR_DECLARE_METHOD(LuaFilesystem, writeObject),
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

    FlashScopedStealthIO sio;
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

    FlashScopedStealthIO sio;
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

    FlashDevice::setStealthIO(1);

    unsigned code = luaL_checkinteger(L, 1);
    FlashVolume vol(FlashMapBlock::fromCode(code));
    if (!vol.isValid()) {
        FlashDevice::setStealthIO(-1);
        lua_pushfstring(L, "invalid volume");
        lua_error(L);
        return 0;
    }

    vol.deleteTree();

    FlashDevice::setStealthIO(-1);
    return 0;
}

int LuaFilesystem::volumeType(lua_State *L)
{
    /*
     * Given a volume block code, return the volume's type.
     */

    FlashDevice::setStealthIO(1);

    unsigned code = luaL_checkinteger(L, 1);
    FlashVolume vol(FlashMapBlock::fromCode(code));
    if (!vol.isValid()) {
        FlashDevice::setStealthIO(-1);
        lua_pushfstring(L, "invalid volume");
        lua_error(L);
        return 0;
    }

    lua_pushnumber(L, vol.getType());
    FlashDevice::setStealthIO(-1);
    return 1;
}

int LuaFilesystem::volumeParent(lua_State *L)
{
    /*
     * Given a volume block code, return the volume's parent.
     */

    FlashDevice::setStealthIO(1);

    unsigned code = luaL_checkinteger(L, 1);
    FlashVolume vol(FlashMapBlock::fromCode(code));
    if (!vol.isValid()) {
        FlashDevice::setStealthIO(-1);
        lua_pushfstring(L, "invalid volume");
        lua_error(L);
        return 0;
    }

    lua_pushnumber(L, vol.getParent().block.code);
    FlashDevice::setStealthIO(-1);
    return 1;
}

int LuaFilesystem::volumeMap(lua_State *L)
{
    /*
     * Given a volume block code, return the volume's map
     * as an array of block codes.
     */

    FlashDevice::setStealthIO(1);

    unsigned code = luaL_checkinteger(L, 1);
    FlashVolume vol(FlashMapBlock::fromCode(code));
    if (!vol.isValid()) {
        FlashDevice::setStealthIO(-1);
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

    FlashDevice::setStealthIO(-1);
    return 1;
}

int LuaFilesystem::volumeEraseCounts(lua_State *L)
{
    /*
     * Given a volume block code, return the volume's array of erase counts.
     */

    FlashDevice::setStealthIO(1);

    unsigned code = luaL_checkinteger(L, 1);
    FlashVolume vol(FlashMapBlock::fromCode(code));
    if (!vol.isValid()) {
        FlashDevice::setStealthIO(-1);
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

    FlashDevice::setStealthIO(-1);
    return 1;
}

int LuaFilesystem::volumePayload(lua_State *L)
{
    /*
     * Given a volume block code, return the volume's payload data as a string.
     */

    FlashDevice::setStealthIO(1);

    unsigned code = luaL_checkinteger(L, 1);
    FlashVolume vol(FlashMapBlock::fromCode(code));
    if (!vol.isValid()) {
        FlashDevice::setStealthIO(-1);
        lua_pushfstring(L, "invalid volume");
        lua_error(L);
        return 0;
    }

    FlashBlockRef ref;
    FlashMapSpan span = vol.getPayload(ref);

    uint8_t *buffer = new uint8_t[span.sizeInBytes()];
    if (!span.copyBytes(0, buffer, span.sizeInBytes())) {
        delete buffer;
        FlashDevice::setStealthIO(-1);
        lua_pushfstring(L, "I/O error");
        lua_error(L);
        return 0;
    }

    lua_pushlstring(L, (const char *)buffer, span.sizeInBytes());
    FlashDevice::setStealthIO(-1);
    delete buffer;
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

    FlashScopedStealthIO sio;
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

    FlashScopedStealthIO sio;
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

    FlashScopedStealthIO sio;
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

int LuaFilesystem::readMetadata(lua_State *L)
{
    /*
     * Read ELF metadata. (volume, key) -> (data)
     */

    FlashDevice::setStealthIO(1);

    unsigned code = luaL_checkinteger(L, 1);
    unsigned key = luaL_checkinteger(L, 2);

    FlashVolume vol(FlashMapBlock::fromCode(code));
    if (!vol.isValid()) {
        FlashDevice::setStealthIO(-1);
        lua_pushfstring(L, "invalid volume");
        lua_error(L);
        return 0;
    }

    FlashBlockRef mapRef;
    Elf::Program program;
    if (!program.init(vol.getPayload(mapRef))) {
        FlashDevice::setStealthIO(-1);
        lua_pushfstring(L, "volume is not an ELF binary");
        lua_error(L);
        return 0;
    }

    FlashBlockRef dataRef;
    uint32_t actualSize = 0;
    const void *data = program.getMeta(dataRef, key, 1, actualSize);

    FlashDevice::setStealthIO(-1);
    if (data) {
        lua_pushlstring(L, (const char*)data, actualSize);
        return 1;
    }
    return 0;
}

int LuaFilesystem::readObject(lua_State *L)
{
    /*
     * Read an LFS object. (volume, key) -> (data)
     */

    FlashDevice::setStealthIO(1);

    unsigned code = luaL_checkinteger(L, 1);
    unsigned key = luaL_checkinteger(L, 2);

    FlashVolume vol(FlashMapBlock::fromCode(code));
    if (code && !vol.isValid()) {
        FlashDevice::setStealthIO(-1);
        lua_pushfstring(L, "invalid volume");
        lua_error(L);
        return 0;
    }

    if (!FlashLFSIndexRecord::isKeyAllowed(key)) {
        FlashDevice::setStealthIO(-1);
        lua_pushfstring(L, "invalid key");
        lua_error(L);
        return 0;
    }

    FlashLFS &lfs = FlashLFSCache::get(vol);
    FlashLFSObjectIter iter(lfs);
    uint8_t buffer[FlashLFSIndexRecord::MAX_SIZE];

    while (iter.previous(key)) {
        unsigned size = iter.record()->getSizeInBytes();
        ASSERT(size <= sizeof buffer);
        if (iter.readAndCheck(buffer, size)) {
            FlashDevice::setStealthIO(-1);
            lua_pushlstring(L, (const char*)buffer, size);
            return 1;
        }
    }

    FlashDevice::setStealthIO(-1);
    return 0;
}

int LuaFilesystem::writeObject(lua_State *L)
{
    /*
     * Write an LFS object. (volume, key, data) -> ()
     */

    size_t dataStrLen = 0;
    unsigned code = luaL_checkinteger(L, 1);
    unsigned key = luaL_checkinteger(L, 2);
    const uint8_t *dataStr = (const uint8_t*) lua_tolstring(L, 3, &dataStrLen);

    FlashVolume vol(FlashMapBlock::fromCode(code));
    if (code && !vol.isValid()) {
        lua_pushfstring(L, "invalid volume");
        lua_error(L);
        return 0;
    }

    if (!FlashLFSIndexRecord::isKeyAllowed(key)) {
        lua_pushfstring(L, "invalid key");
        lua_error(L);
        return 0;
    }
    
    if (!FlashLFSIndexRecord::isSizeAllowed(dataStrLen)) {
        lua_pushfstring(L, "invalid data size");
        lua_error(L);
        return 0;
    }

    CrcStream cs;
    cs.reset();
    cs.addBytes(dataStr, dataStrLen);
    uint32_t crc = cs.get(FlashLFSIndexRecord::SIZE_UNIT);

    FlashDevice::setStealthIO(1);

    FlashLFS &lfs = FlashLFSCache::get(vol);
    FlashLFSObjectAllocator allocator(lfs, key, dataStrLen, crc);

    if (!allocator.allocateAndCollectGarbage()) {
        FlashDevice::setStealthIO(-1);
        lua_pushfstring(L, "out of space");
        lua_error(L);
        return 0;
    }

    FlashBlock::invalidate(allocator.address(), allocator.address() + dataStrLen);
    FlashDevice::write(allocator.address(), dataStr, dataStrLen);

    FlashDevice::setStealthIO(-1);
    return 0;
}
