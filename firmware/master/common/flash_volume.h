/*
 * Thundercracker Firmware -- Confidential, not for redistribution.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

/*
 * Just above the Mapping layer in the flash stack, the Volume layer
 * is responsible for locating and allocating large discontiguous areas
 * ("Volumes") in flash. A Volume can contain, for example, an ELF object
 * file or a log-structured filesystem.
 *
 * Each volume begins with a volume header in its first MapBlock, containing:
 *
 *   - Prefix: Magic number, CRCs, type code, flags
 *   - Optional FlashMap, identifying all MapBlocks in the volume
 *   - Optional erase counts for all MapBlocks in the volume
 *
 * A volume header is responsible for storing this information on behalf of
 * the entire volume. For volumes consisting of multiple blocks, subsequent
 * blocks have no header of their own. This keeps the MapSpans for the volume
 * contiguous.
 *
 * Because the volume header is responsible for storing erase counts, and
 * we must take care to preserve these erase counts even in unused areas of
 * the flash, we need a way to mark the volume as "deleted" without
 * invalidating the other data stored in this header.
 *
 * Volumes support only a few operations:
 *
 *   - Enumeration. With no prior knowledge, we can list all of the volumes
 *     on our flash device by scanning for valid headers.
 *
 *   - Referencing. Without copying it, we can pin a FlashMap in our cache
 *     and use it to copy or map the volume's contents.
 *
 *   - Deleting. A volume can be marked as deleted, causing its space to
 *     be reclaimable for new volume allocations. Individual MapBlocks can
 *     be reclaimed in arbitrary order, as long as the header block is
 *     reclaimed last. Blocks should generally be reclaimed in order of
 *     increasing erase count. This means that the header block should
 *     typically be placed in the MapBlock with the highest erase count.
 *
 *   - Allocating. A volume of a given size can be allocated by erasing
 *     and reclaiming deleted MapBlocks.
 *
 * Note that erase counts are temporarily deleted during an Allocate operation.
 * If we lose power during an Allocate, the space behind that volume will be
 * unreachable from any other volume's map, but it is not actually marked as
 * deleted. This is also the state we're in when faced with an uninitialized
 * flash device. We'll do the best we can, and create new erase counts by
 * averaging all of the known erase counts from other blocks.
 *
 * Layout of a FlashVolume's first MapBlock:
 *
 *   First cache block:
 *      - Fixed size "Prefix" struct, containing type/flag-specific fields
 *      - Optional FlashMap
 *
 *   Optional, ERASE_COUNT_BLOCKS blocks of erase count data:
 *      - A uint32_t erase count for each of NUM_MAP_BLOCKS
 *
 *   Payload data begins.
 */

#ifndef FLASH_VOLUME_H_
#define FLASH_VOLUME_H_

#include "macros.h"
#include "flash_map.h"


/**
 * Represents a single Volume in flash: a physically discontiguous
 * coarsely-allocated object. We don't store any of the volume contents
 * in RAM, it's all referenced via the block cache.
 */
class FlashVolume
{
public:
    enum Flags {
        F_HAS_MAP   = 1 << 0,
    };

    enum Type {
        T_DELETED   = 0,        // Must be zero
        T_ELF       = 0x4C45,
    };

    FlashVolume(FlashMapBlock block) : block(block) {}

    bool isValid() const;
    const FlashMap *getMap(FlashBlockRef &ref) const;
    uint32_t getEraseCount(unsigned index) const;
    void markAsDeleted() const;

    /// Synonymous with isValid()
    operator bool() const { return isValid(); }

private:
    FlashMapBlock block;

    static const uint64_t MAGIC = 0x4c4f564674666953ULL;
    static const unsigned ERASE_COUNT_BYTES = FlashMap::NUM_MAP_BLOCKS * sizeof(uint32_t);
    static const unsigned ERASE_COUNT_BLOCKS = ERASE_COUNT_BYTES / FlashBlock::BLOCK_SIZE;
    static const unsigned ERASE_COUNTS_PER_BLOCK = FlashBlock::BLOCK_SIZE / sizeof(uint32_t);
        
    struct Prefix {
        uint64_t magic;                 // Must equal MAGIC
        uint16_t flags;                 // Bitmap of F_*
        uint16_t type;                  // One of T_*

        union {
            struct {                    // For F_HAS_MAP:
                uint32_t crcMap;        //   CRC for FlashMap only
                uint32_t crcErase[ERASE_COUNT_BLOCKS];
            } hasMap;
            struct {                    // For !F_HAS_MAP:
                uint32_t eraseCount;    //   Erase count for the first and only MapBlock
                uint32_t eraseCountCopy;
            } noMap;
        };

        uint16_t flagsCopy;             // Redundant copy of 'flags'
        uint16_t typeCopy;              // Redundant copy of 'type'

        bool isValid() const;
    };

    Prefix *getPrefix(FlashBlockRef &ref) const;
    uint32_t *getEraseCountBlock(FlashBlockRef &ref, unsigned index) const;
};


/**
 * A lightweight iterator, capable of finding all valid FlashVolumes on
 * the device. When iteration is over, next() returns an invalid volume.
 */
class FlashVolumeIter
{
public:
    FlashVolumeIter() {
        remaining.mark();
    }

    FlashVolume next();

private:
    FlashMapBlock::Set remaining;
};


#endif
