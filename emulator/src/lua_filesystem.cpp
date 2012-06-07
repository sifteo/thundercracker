/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * Sifteo Thundercracker simulator
 * M. Elizabeth Scott <beth@sifteo.com>
 *
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */
 
#include "lua_script.h"
#include "lua_filesystem.h"
#include "flash_volume.h"
#include "flash_volumeheader.h"

const char LuaFilesystem::className[] = "Filesystem";

Lunar<LuaFilesystem>::RegType LuaFilesystem::methods[] = {
    LUNAR_DECLARE_METHOD(LuaFilesystem, newVolume),
    LUNAR_DECLARE_METHOD(LuaFilesystem, listVolumes),
    LUNAR_DECLARE_METHOD(LuaFilesystem, deleteVolume),
    LUNAR_DECLARE_METHOD(LuaFilesystem, volumeType),
    LUNAR_DECLARE_METHOD(LuaFilesystem, volumeMap),
    LUNAR_DECLARE_METHOD(LuaFilesystem, volumeEraseCounts),
    LUNAR_DECLARE_METHOD(LuaFilesystem, simulatedSectorEraseCounts),
    {0,0}
};


LuaFilesystem::LuaFilesystem(lua_State *L)
{
    /* Nothing to do; the filesystem is a singleton */
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