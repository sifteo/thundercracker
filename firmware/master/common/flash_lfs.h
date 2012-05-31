/*
 * Thundercracker Firmware -- Confidential, not for redistribution.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

/*
 * LFS layer: our Log-structured File System
 *
 * This is an optional layer which can sit on top of the Volume layer.
 * While it's totally feasible to use Volumes directly, for storing large
 * immutable objects, the LFS is designed to provide an efficient way to
 * store small objects that may change frequently.
 *
 * This LFS is a fairly classic implementation of the general LFS concept.
 * For more background information, check out Wikipedia:
 *
 *   http://en.wikipedia.org/wiki/Log-structured_file_system
 *
 * Each Volume in the filesystem may act as the parent for an LFS, which
 * occupies zero or more child volumes of that parent. These volumes are
 * used as a ring buffer for stored objects.
 *
 * Objects are identified in three ways:
 *
 *   1. Their parentage. Each object is "owned" by a particular volume,
 *      typically a Game or the Launcher. This forms the top-level namespace
 *      under which objects are stored.
 *
 *   2. An 8-bit key identifies distinct objects within a particular parent
 *      volume. These keys are opaque identifiers for us. Games may use
 *      them to differentiate between different kinds of saved data, such
 *      as different types of metrics, or different save-game slots.
 *
 *   3. A version. Because several copies of an object may exist in flash,
 *      we need to know which one is the most recent. This ordering is
 *      implied by the physical order of objects within a single LFS volume,
 *      and the ordering of LFS volumes within a particular LFS is stated
 *      explicitly via a sequence number in the volume header.
 */

#ifndef FLASH_LFS_H_
#define FLASH_LFS_H_

#include "macros.h"
#include "flash_volume.h"


/**
 * Represents the in-memory state associated with a single LFS.
 *
 * This is a vector of FlashVolumes for each volume within the LFS.
 * Each of those volumes have been sorted in order of ascending sequence
 * number.
 */
class FlashLFS
{
private:
    // Object sizes are 8-bit, measured in multiples of our OBJ_SIZE_UNIT.
    static const unsigned OBJ_SIZE_UNIT = 16;
    static const unsigned MAX_OBJ_SIZE = OBJ_SIZE_UNIT * 0x100;

    // Keys are 8-bit, there can be at most this many
    static const unsigned MAX_KEYS = 0x100;

    // All of our volumes occupy exactly one FlashMapBlock.
    static const unsigned VOLUME_SIZE = FlashMapBlock::BLOCK_SIZE;

    /*
     * To spec sizes for:
     *   - Objects
     *   - Object index (header at the beginning of volume payload)
     *   - Meta-index (in type-specific data portion of volume)
     */

    static const unsigned MAX_OBJECTS_PER_VOLUME = FlashMapBlock::BLOCK_SIZE / OBJECT_SIZE_UNIT;
    FlashVolume volumes[];
};

#endif
