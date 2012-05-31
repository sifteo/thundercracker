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
 * Common methods used by multiple LFS components.
 */
namespace LFS {
    uint8_t computeCheckByte(uint8_t a, uint8_t b);
};


/**
 * FlashLFSKeyFilter is a probabilistic data structure that can be used to
 * test whether a key is maybe-in the set or definitely-not-in the set.
 *
 * This is equivalent to a very tiny Bloom filter with a single trivial hash
 * function (k=1). Whereas the generalized Bloom filter is good for cases
 * where the space of keys is very large compared to the size of the filter,
 * this object is intended for cases where there's only an order of magnitude
 * or so of difference: We have 256 possible keys, and 16 bits of hash.
 *
 * These sizes were chosen so that this KeyFilter can be used to build a fast
 * "meta-index" that tells us which index pages we need to examine. This
 * must fit within the type-specific-data region in each Volume.
 *
 * In order to optimize for cases where games use small counting numbers for
 * keys, we opt to use a trivial hash function: the low 4 bits of every key.
 * This way, if a game chooses to use keys from 1 through 16, for example,
 * they will all occupy distinct filter bits, and our scan of the meta-index
 * will tell us for sure which index page we need. If there are collisions,
 * we still operate correctly, we just may need to load additional index
 * pages that we don't actually need.
 *
 * Because this filter must operate in flash memory, and adding an item must
 * not require an erase, we invert the typical Bloom filter bit values. "1"
 * represents an empty hash bucket, "0" means that one or more items occupy
 * that bucket.
 */
class FlashLFSKeyFilter
{
    uint16_t bits;

    unsigned bitHash(unsigned key) const {
        // We have no need for CLZ here, so use a right-to-left order.
        return 1 << (key & 0xF);
    }

public:
    bool empty() {
        return bits == 0xFFFF;
    }

    void add(unsigned key) {
        bits &= ~bitHash(key);
    }

    /**
     * Returns 'true' if the item is possibly in the filter, or 'false'
     * if it is definitely not.
     */
    bool test(unsigned key) const {
        return !(bits & bitHash(key));
    }
};


/**
 * FlashLFSIndexRecord is the data type which comprises our object index.
 * This stores all of the metadata we need to identify and load an object
 * from the LFS itself.
 *
 * Index records should be small: Larger index records require more flash
 * blocks to store, which means more meta-index records. The meta-index
 * must be small enough to keep in the Volume header block. This is one
 * of the reasons we keep the size of the key space limited. (The other
 * reason is to put a reasonable upper bound on the size of the whole LFS.)
 *
 * This record must hold enough information to (1) locate an object,
 * (2) very reliably determine whether any space has been allocated,
 * even if we experience a power failure during write, and (3) validate
 * the object, so we can fall back on a previous version if it's corrupted.
 *
 * This implementation requires 5 bytes of storage:
 *    - 8-bit key, 8-bit size
 *    - 16-bit CRC for the object contents
 *    - Check byte for key and size
 *
 * This lets us detect the difference between the two main power-fail modes:
 *
 *    1. We started to write the index, but didn't finish. This means
 *       we've wasted an index slot, but no space was actually allocated for
 *       the object, so that space is still free.
 *
 *    2. We finished writing the index, but the object data didn't finish
 *       writing. This will render the content CRC invalid, so we'll
 *       ignore this and use an earlier version of the same object.
 */
class FlashLFSIndexRecord
{
    uint8_t key;
    uint8_t size;
    uint8_t crc[2];     // Unaligned on purpose...
    uint8_t check;      //   to keep the check byte at the end.

public:
    // Keys are 8-bit, there can be at most this many
    static const unsigned MAX_KEYS = 0x100;

    // Object sizes are 8-bit, nonzero, measured in multiples of SIZE_UNIT
    static const unsigned SIZE_SHIFT = 4;
    static const unsigned SIZE_UNIT = 1 << SIZE_SHIFT;
    static const unsigned SIZE_MASK = SIZE_UNIT - 1;
    static const unsigned MAX_SIZE = 0x100 << SIZE_SHIFT;
    static const unsigned MIN_SIZE = 0x001 << SIZE_SHIFT;

    void init(unsigned key, unsigned sizeInBytes, unsigned crc)
    {
        STATIC_ASSERT(sizeof *this == 5);

        ASSERT(key < MAX_KEYS);
        ASSERT(sizeInBytes >= MIN_SIZE);
        ASSERT(sizeInBytes <= MAX_SIZE);
        ASSERT((sizeInBytes & SIZE_MASK) == 0);

        unsigned size = (sizeInBytes >> SIZE_SHIFT) - 1;

        this->key = key;
        this->size = size;
        this->crc[0] = crc;
        this->crc[1] = crc >> 8;
        this->check = LFS::computeCheckByte(key, size);
    }

    bool isValid() const {
        return check == LFS::computeCheckByte(key, size);
    }

    unsigned getKey() const {
        return key;
    }

    unsigned getSizeInBytes() const {
        return (size + 1U) << SIZE_SHIFT;
    }

    bool checkCRC(unsigned reference) const {
        return !(((crc[0] | (crc[1] << 8)) ^ reference) & 0xFFFF);
    }
};


/**
 * FlashLFSIndexAnchor is a flavor of header record that appears at
 * the beginning of a FlashLFSIndexBlock. Typically there is exactly
 * one anchor per block, but we must allow for the possibility of multiple
 * anchors, in case there's a power failure while allocating a new index
 * block. Anchors must be validated, and invalid anchors are ignored.
 * There must be exactly one valid anchor per block. Records begin
 * immediately after the first valid anchor.
 *
 * Anchors are used to locate the object data which corresponds with the
 * first record in any given FlashLFSIndexBlock. The anchor value could also
 * be determined by summing the sizes of all preceeding index records, but
 * including this value explicitly in the anchor lets us locate the objects
 * associated with an index block without reading any additional index blocks.
 *
 * An offset of zero is valid. Erased (0xFF) memory is guaranteed to never
 * be interpreted as a valid FlashLFSIndexAnchor.
 */
class FlashLFSIndexAnchor
{
    uint8_t offset[2];
    uint8_t check;

public:
    static const unsigned OFFSET_SHIFT = FlashLFSIndexRecord::SIZE_SHIFT;
    static const unsigned OFFSET_UNIT = FlashLFSIndexRecord::SIZE_UNIT;
    static const unsigned OFFSET_MASK = FlashLFSIndexRecord::SIZE_MASK;
    static const unsigned MAX_OFFSET = FlashMapBlock::BLOCK_SIZE;

    void init(unsigned offsetInBytes)
    {
        STATIC_ASSERT(sizeof offset == 2);
        STATIC_ASSERT(sizeof *this == 3);
        STATIC_ASSERT((MAX_OFFSET >> OFFSET_SHIFT) < 0xFFFF);

        ASSERT((offsetInBytes & OFFSET_MASK) == 0);
        ASSERT(offsetInBytes < MAX_OFFSET);

        unsigned offsetWord = offsetInBytes >> OFFSET_SHIFT;
        uint8_t offsetLow = offsetWord;
        uint8_t offsetHigh = offsetWord >> 8;

        this->offset[0] = offsetLow;
        this->offset[1] = offsetHigh;
        this->check = LFS::computeCheckByte(offsetLow, offsetHigh);
    }

    bool isValid() const {
        return check == LFS::computeCheckByte(offset[0], offset[1]);
    }

    unsigned getOffsetInBytes() const {
        return (this->offset[0] | (this->offset[1] << 8)) << OFFSET_SHIFT;
    }
};


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

    /*
     * All of our volumes occupy exactly one FlashMapBlock.
     * The first block is used for a header (Volume header, plus our meta-index).
     * Other blocks are available for our own payload (Index plus objects).
     */
    static const unsigned VOLUME_SIZE = FlashMapBlock::BLOCK_SIZE;
    static const unsigned VOLUME_PAYLOAD_BYTES = VOLUME_SIZE - FlashBlock::BLOCK_SIZE;

    static const unsigned MAX_OBJECTS_PER_VOLUME = FlashMapBlock::BLOCK_SIZE / FlashLFSIndexRecord::MIN_SIZE;

    /*
     * To spec sizes for:
     *   - Objects
     *   - Object index (header at the beginning of volume payload)
     *   - Meta-index (in type-specific data portion of volume)
     */

    FlashVolume volumes[];
};

#endif
