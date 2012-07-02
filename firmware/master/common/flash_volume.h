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
    /**
     * Convention for volume types:
     *
     *    - System volume types are two capital letters
     *
     *    - If userspace volumes are ever allowed, they will
     *      follow a still-TBD namespacing scheme.
     *
     * Note that some volume types imply special behaviour in the filesystem.
     * Both DELETED and INCOMPLETE volumes are automatically garbage-collected.
     * LFS volumes are guaranteed to always be exactly one map-block in length.
     */
    enum Type {
        T_LAUNCHER      = _SYS_FS_VOL_LAUNCHER,     // "LN"
        T_GAME          = _SYS_FS_VOL_GAME,         // "GM"
        T_LFS           = 0x5346,                   // "FS"
        T_DELETED       = 0x0000,                   // Must be zero
        T_INCOMPLETE    = 0xFFFF,                   // Must be FFFF
    };

    FlashMapBlock block;

    FlashVolume() {}
    FlashVolume(FlashMapBlock block) : block(block) {}
    FlashVolume(_SYSVolumeHandle vh);

    bool isValid() const;
    _SYSVolumeHandle getHandle() const;
    unsigned getType() const;
    FlashVolume getParent() const;
    FlashMapSpan getPayload(FlashBlockRef &ref) const;
    uint8_t *mapTypeSpecificData(FlashBlockRef &ref, unsigned &size) const;

    /**
     * Delete just this volume. Only for use as part of a larger delete
     * operation, or for volumes which are guaranteed not to have any
     * child volumes. Saves the cost of a volume scan to look for children.
     */
    void deleteSingle() const;

    /**
     * A version of deleteSingle() which doesn't invalidate the LFS cache.
     * This must be used by the implementation of the LFS garbage collector.
     */
    void deleteSingleWithoutInvalidate() const;

    /**
     * Delete this volume and all children.
     *
     * Always use this function if there's any possibility that a volume
     * has child volumes under it. Deleting a parent volume without deleting
     * the corresponding child volumes will mean that the child volumes
     * will return a no-longer-meaningful FlashVolume from getParent(). If
     * a new volume is created at the same address later, the child volumes
     * will be incorrectly parented.
     *
     * Note that it's allowed to delete the currently-running volume. The
     * volume will be marked as deleted, but its blocks will not be recycled
     * as long as that volume is still running.
     */
    void deleteTree() const;

    /**
     * Delete all volumes on the system! Can be performed as part of a
     * factory reset / system wipe.
     */
    static void deleteEverything();

private:
    static uint32_t signHandle(uint32_t h);
};


/**
 * A lightweight iterator, capable of finding all valid FlashVolumes on
 * the device.
 */
class FlashVolumeIter
{
public:
    /// Reset the iterator back to the beginning of the sequence
    void begin() {
        DEBUG_ONLY(initialized = true);
        remaining.mark();
    }

    /// Returns 'true' iff another FlashVolume can be found.
    bool next(FlashVolume &vol);

private:
    FlashMapBlock::Set remaining;
    DEBUG_ONLY(bool initialized;)
};


/**
 * Manages the process of finding unused FlashMapBlocks to recycle.
 * Typically we look for blocks in deleted volumes, starting with
 * the lowest-erase-count blocks that aren't part of a volume header.
 *
 * We use header blocks only when all other blocks in that volume have
 * already been recycled, so that we don't lose any erase count data.
 *
 * If we spot any orphaned blocks (not part of any volume, even a deleted
 * one) we unfortunately have no way of knowing the true erase count of
 * those blocks. We prefer to use these orphaned blocks before recycling
 * deleted blocks, since we make up an erase count by taking the average
 * of all known erase counts. If we didn't use the orphaned blocks first,
 * the calculated pseudo-erase-count would increase over time, causing
 * these orphan blocks to systematically appear more heavily worn than
 * they actually are, causing us to overwear the non-orphaned blocks.
 *
 * This property of the recycler is especially important when dealing with
 * a blank or heavily damaged filesystem, in which large numbers of blocks
 * (possibly all of them) are orphaned.
 *
 * Much of the complexity here comes from trying to keep the memory usage
 * and algorithmic complexity low. For example, we don't want to spend the
 * memory on a full table of blocks sorted by erase count, nor do we want
 * to re-scan the device once per recycled block. It turns out that it isn't
 * actually important that we pick the absolute lowest-erase-count block
 * every time. As long as we tend to pick lower-erase-count blocks first,
 * and we accurately propagate erase counts, we'll still provide wear
 * leveling.
 *
 * Since we prefer to operate within a single deleted volume at a time,
 * our approach involves keeping a set of candidate volumes in which at least
 * one of their blocks has an erase count <= the average. This candidate list
 * is always preferred when searching for blocks to recycle.
 */

class FlashBlockRecycler {
public:
    typedef uint32_t EraseCount;

    FlashBlockRecycler();

    /**
     * Find the next recyclable block, as well as its erase count.
     *
     * In the case of orphaned blocks, this returns quickly without modifying
     * any memory. However, if we're reclaiming a block from a deleted volume,
     * this may need to write to the deleted volume's map in order to invalidate
     * individual blocks.
     *
     * Note: One alternative design, which wouldn't require writing to the
     * volume map, would be to 'version' each volume header, as you may do
     * in a log-structured filesystem. This seems less preferable, however,
     * since it would turn the recycling operation into an O(N^2) problem
     * with the number of flash blocks in the device!
     */
    bool next(FlashMapBlock &block, EraseCount &eraseCount);

private:
    FlashMapBlock::Set orphanBlocks;
    FlashMapBlock::Set deletedVolumes;
    FlashMapBlock::Set candidateVolumes;
    uint32_t averageEraseCount;

    FlashBlockWriter dirtyVolume;

    void findOrphansAndDeletedVolumes();
    void findCandidateVolumes();
};


/**
 * A FlashVolumeWriter keeps track of the multi-step process of writing
 * a volume for the first time.
 *
 * Some volumes may be appended to or written after the initial allocation,
 * but if you want to allocate a volume atomically you can use a
 * FlashVolumeWriter to ensure that the volume is never marked as valid until
 * you've fully written its contents.
 */

class FlashVolumeWriter
{
public:
    FlashVolume volume;

    /**
     * Start writing a new volume. In begin() we do all of the prep work that's
     * possible to do without actually filling in the volume's payload or header
     * data. By the end of begin(), we've created a valid volume of type
     * T_INCOMPLETE. The erase counts and header are all valid, so if we get
     * interrupted (or if the download is intentionally aborted) the volume
     * will be automatically recycled.
     *
     * This can take some time, as it involves erasing flash blocks as well
     * as scanning for recyclable blocks.
     */
    bool begin(unsigned type, unsigned payloadBytes,
        unsigned hdrDataBytes = 0,
        FlashVolume parent = FlashMapBlock::invalid());

    /**
     * Start writing a game, after deleting any existing game volumes with
     * the same package name.
     */
    bool beginGame(unsigned payloadBytes, const char *package);

    /**
     * Start writing the launcher, after deleting any existing launcher.
     */
    bool beginLauncher(unsigned payloadBytes);

    /**
     * Map a writeable copy of the portion of the type-specific data region
     * which fits in the header block.
     *
     * May not be interleaved with appendPayload()!
     */
    uint8_t *mapTypeSpecificData(unsigned &size);

    /**
     * Write any amount of payload data, starting at the beginning of the volume
     * or at the end of the last appendPayload() chunk. This data is buffered
     * in the flash cache until we have a complete page, then it's written to
     * the device.
     *
     * We've already erased all payload blocks in begin(), so this call never
     * has to erase.
     */ 
    void appendPayload(const uint8_t *bytes, uint32_t count);

    /**
     * Finish writing the volume. This completes any writes that are pending
     * from earlier appendPayload() operations, plus it writes the correct
     * type code into the volume's header, making it persistent.
     */
    void commit();

private:
    FlashBlockWriter payloadWriter;
    unsigned payloadOffset;
    uint16_t type;
};


#endif
