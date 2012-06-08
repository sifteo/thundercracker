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
 */

#ifndef FLASH_SYSLFS_H_
#define FLASH_SYSLFS_H_

#include "flash_lfs.h"


namespace SysLFS {

    /*
     * Key space
     */

    const unsigned kCubeBase = 0x28;
    const unsigned kCubeCount = _SYS_NUM_CUBE_SLOTS;
    const unsigned kAssetSlotBase = kCubeBase + kCubeCount;
    const unsigned kAssetSlotCount = _SYS_NUM_CUBE_SLOTS * _SYS_NUM_ASSET_SLOTS;
    const unsigned kEnd = kAssetSlotBase + kAssetSlotCount;

    /*
     * Accessors
     */

    inline FlashLFS &get() {
        STATIC_ASSERT(kEnd <= _SYS_FS_MAX_OBJECT_KEYS);
        return FlashLFSCache::get(FlashMapBlock::invalid());
    }

} // end namespace SysLFS


#endif
