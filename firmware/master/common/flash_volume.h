/*
 * Thundercracker Firmware -- Confidential, not for redistribution.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

/*
 * Just above the Mapping layer in the flash stack, the Volume layer
 * is responsible for locating and allocating large discontiguous areas
 * ("Volumes") in flash. A Volume can contain, for example, an ELF object
 * file or a log-structured filesystem.
 */

#ifndef FLASH_VOLUME_H_
#define FLASH_VOLUME_H_

#include "flash_map.h"


/**
 * Header for a single flash volume.
 *
 *   1. It contains its own valid FlashMapHeader.
 *   2. It is referenced by a preceeding valid FlashMapHeader.
 *   3. It is unallocated.
 *
 * Large objects that occupy more than one FlashMapBlock only need a single
 * FlashMapHeader, on the object's first block. This allows a single
 * FlashMapSpan to reference the entirety of the object's data.
 *
 * In order to provide robustness against flash failures as well as robustness
 * against power outages that occur between erasing an AllocBlock and fully
 * writing its header, we keep a CRC for the header. If the CRC is not valid,
 * the AllocBlock is treated as unallocated.
 *
 * For wear leveling, an AllocHeader must also store erase counts for every
 * block it's referencing. These erase counts are 32-bit monotonically
 * increasing integers, incremented every time the specified AllocBlock
 * is erased.
 *
 * XXX: Work in progress.
 * XXX: This comment is outdated...
 */

struct FlashVolumeHeader
{
    static const unsigned NUM_BLOCKS = 4;
    static const unsigned NUM_BYTES = FlashBlock::BLOCK_SIZE * NUM_BLOCKS;
    static const unsigned NUM_WORDS = NUM_BYTES / sizeof(uint32_t);
    static const uint32_t MAGIC = 0x74666953;   // 'Sift'

    enum Type {
        T_OBJECT = 0x4a424f46,  // 'FOBJ' = large object
    };

    // Universal header
    struct {
        uint32_t magic;
        uint32_t type;
        uint32_t crc;
    } h;

    // Type-specific data occupies the rest of the block
    union {
        uint32_t words[NUM_WORDS - 3];
        struct {
            FlashMap map;
            uint32_t eraseCounts[FlashMap::NUM_ALLOC_BLOCKS];
        } object;
    } u;
};


#endif
