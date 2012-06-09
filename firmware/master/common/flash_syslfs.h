/*
 * Thundercracker Firmware -- Confidential, not for redistribution.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

/*
 * Definitions for the System's own LFS instance.
 *
 * An LFS instance can be attached to any volume on the filesystem.
 * Games and the system Launcher will each have their own personal LFS
 * to write into. This file describes the one special LFS which is made
 * up of volumes without a parent. These LFS volumes are owned by the system,
 * and used for things that are fairly low-level and shared amongst multiple
 * apps, like Cube pairing information and AssetSlot allocation.
 *
 * The main architectural distinction between this LFS and the Launcher's LFS,
 * which can also be used for "system"-seeming information, is that the
 * Launcher LFS normally can only be written to while the launcher is running.
 * The SysLFS, on the other hand, may be written to as a side-effect of running
 * any application.
 *
 * This distinction also lets us keep the system and launcher more decoupled
 * from each other, since they aren't sharing the same key-space. As an
 * additional practical consideration, the SysLFS key space is already fairly
 * densely utilized, so the Launcher wouldn't have much room left over.
 */

#ifndef FLASH_SYSLFS_H_
#define FLASH_SYSLFS_H_

#include <sifteo/abi.h>
#include "flash_lfs.h"
#include "macros.h"


namespace SysLFS {

    /*
     * Defined limits.
     *
     * These are things that userspace doesn't directly care about
     * (so they aren't in abi.h) but they're vital to the binary
     * compatibility of SysLFS records.
     */

    const unsigned ASSET_SLOTS_PER_CUBE = 8;
    const unsigned ASSET_SLOTS_PER_BANK = 4;
    const unsigned TILES_PER_ASSET_SLOT = 4096;
    const unsigned ASSET_GROUPS_PER_SLOT = 24;

    /*
     * Key space
     *
     * Keys in the range [0x00, 0x27] are currently unallocated.
     */

    enum Keys {
        kCubeBase       = 0x28,
        kCubeCount      = _SYS_NUM_CUBE_SLOTS,
        kAssetSlotBase  = kCubeBase + kCubeCount,
        kAssetSlotCount = _SYS_NUM_CUBE_SLOTS * ASSET_SLOTS_PER_CUBE,
        kEnd            = kAssetSlotBase + kAssetSlotCount,
    };

    /*
     * Per-cube stored data
     */

    struct AssetSlotIdentity {
        uint8_t volume;
        uint8_t ordinal;
    };

    struct AssetSlotOverviewRecord {
        uint32_t eraseCount;
        uint16_t numAllocatedTiles;
        AssetSlotIdentity identity;
    };

    struct CubePairingRecord {
        uint64_t hwid;
        // XXX: More here... Radio frequencies, address, etc.
    };

    struct CubeRecord {
        AssetSlotOverviewRecord assetSlots[ASSET_SLOTS_PER_CUBE];
        CubePairingRecord pairing;
    };

    /*
     * Per-AssetSlot stored data
     */

    struct AssetGroupSize {
        // Encoded 8-bit size. Range of [1, 4096], rounding up to the nearest 16 tiles.
        uint8_t code;

        static AssetGroupSize fromTileCount(unsigned tileCount)
        {
            STATIC_ASSERT(TILES_PER_ASSET_SLOT == 4096);
            ASSERT(tileCount >= 1 && tileCount <= 4096);
            AssetGroupSize result = { (tileCount - 1) >> 4 };
            return result;
        }

        unsigned tileCount() const {
            return (code + 1U) << 4;
        };
    };

    struct AssetGroupIdentity {
        uint8_t volume;
        uint8_t ordinal;
    };

    struct LoadedAssetGroupRecord {
        AssetGroupSize size;
        AssetGroupIdentity identity;
    };

    struct AssetSlotRecord {
        LoadedAssetGroupRecord groups[ASSET_GROUPS_PER_SLOT];
    };

    /*
     * Accessors
     */

    inline FlashLFS &get()
    {
        STATIC_ASSERT(kEnd <= _SYS_FS_MAX_OBJECT_KEYS);
        STATIC_ASSERT(sizeof(FlashMapBlock) == 1);
        STATIC_ASSERT(sizeof(LoadedAssetGroupRecord) == 3);
        STATIC_ASSERT(sizeof(AssetSlotRecord) == 3 * ASSET_GROUPS_PER_SLOT);
        STATIC_ASSERT(sizeof(AssetSlotOverviewRecord) == 8);

        return FlashLFSCache::get(FlashMapBlock::invalid());
    }

} // end namespace SysLFS


#endif
