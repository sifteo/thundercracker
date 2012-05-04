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
 *   - Volume prefix: Magic number, CRCs, type code, type-specific data
 *   - A FlashMap, identifying all MapBlocks in the volume
 *   - Erase counts for all MapBlocks in the volume
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
 */

#ifndef FLASH_VOLUME_H_
#define FLASH_VOLUME_H_

#include "flash_map.h"


/**
 * Represents a single Volume in flash: a physically discontiguous
 * coarsely-allocated object. We don't store any of the volume contents
 * in RAM, it's all referenced via the block cache.
 */
class FlashVolume
{
public:


private:
    FlashMapBlock   headerBlock;

    static const uint64_t MAGIC = 0x4c4f564674666953;

    struct Prefix {
        uint64_t magic;
        uint32_t type;
    };

    void getPrefixBlock();
    
    
    

/*
 * Scanning
 * Referencing
 * Allocating
 * Erasing

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
