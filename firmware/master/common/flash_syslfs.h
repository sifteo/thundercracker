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
     */

    const unsigned ASSET_SLOTS_PER_CUBE = _SYS_ASSET_SLOTS_PER_BANK * 2;
    const unsigned ASSET_SLOTS_PER_BANK = _SYS_ASSET_SLOTS_PER_BANK;
    const unsigned TILES_PER_ASSET_SLOT = _SYS_TILES_PER_ASSETSLOT;
    const unsigned ASSET_GROUPS_PER_SLOT = _SYS_ASSET_GROUPS_PER_SLOT;

    // This cannot be changed without modifying the Key layout below
    const unsigned NUM_PAIRINGS = _SYS_NUM_CUBE_SLOTS;

    /*
     * Key space
     *
     * Keys below the first item in the table are currently unallocated.
     */

    enum Key {
        kPairingMRU     = 0x26,
        kPairingID      = 0x27,
        kCubeBase       = 0x28,
        kCubeCount      = NUM_PAIRINGS,
        kAssetSlotBase  = kCubeBase + kCubeCount,
        kAssetSlotCount = NUM_PAIRINGS * ASSET_SLOTS_PER_CUBE,
        kEnd            = kAssetSlotBase + kAssetSlotCount,
    };

    /*
     * Pairing data.
     *
     * We quickly access data for all paired-but-disconnected cubes
     * in aggregate, by storing them in two records. The ID record
     * changes infrequently (only when a new cube is paired), while
     * the MRU record may change at every connection event.
     */

    struct PairingIDRecord {
        uint64_t hwid[NUM_PAIRINGS];

        static const uint64_t INVALID_HWID = uint64_t(-1);

        void init();
        void load();
    };

    struct PairingMRURecord {
        // Indices into hwid[], in order of most recently used first.
        uint8_t rank[NUM_PAIRINGS];

        void init();
        void load();
        bool access(unsigned index);
    };

    /*
     * Per-cube data for connected cubes.
     *
     * This data is associated with a CubeSlot at the moment it's
     * connected to a cube. The IDs starting with kCubeBase parallel
     * the slots in PairingIDRecord, however they do *not* map
     * one-to-one with our CubeSlots. CubeSlot is responsible
     * for storing this mapping for each connected cube.
     */

    struct AssetSlotIdentity {
        uint8_t volume;
        uint8_t ordinal;

        bool inActiveSet(FlashVolume vol, unsigned numSlots) const {
            return volume == vol.block.code && ordinal < numSlots;
        }
    };

    struct AssetSlotOverviewRecord {
        uint8_t eraseCount;
        uint8_t accessRank;
        AssetSlotIdentity identity;

        static const unsigned MAX_COST = 0xFE;
        unsigned costToEvict() const;
    };
    
    struct CubeAssetsRecord {
        AssetSlotOverviewRecord slots[ASSET_SLOTS_PER_CUBE];

        typedef BitVector<ASSET_SLOTS_PER_CUBE> SlotVector_t;

        bool checkBinding(FlashVolume vol, unsigned numSlots) const;
        void allocBinding(FlashVolume vol, unsigned numSlots);

        void markErased(unsigned slot);
        bool markAccessed(FlashVolume vol, unsigned numSlots, bool force);

    private:
        void recycleSlots(unsigned bank, unsigned numSlots,
            SlotVector_t &vecOut, unsigned &costOut) const;
    };

    struct CubeRecord {
        CubeAssetsRecord assets;

        void init();
        bool load(const FlashLFSObjectIter &iter);

        static Key makeKey(_SYSCubeID cube);
        static bool decodeKey(Key cubeKey, _SYSCubeID &cube);
    };

    /*
     * Per-AssetSlot stored data
     */

    struct AssetGroupSize {
        // Encoded 8-bit size. Range of [1, 4096], rounding up to the nearest 16 tiles.
        uint8_t code;

        static AssetGroupSize fromTileCount(unsigned tileCount)
        {
            STATIC_ASSERT(_SYS_ASSET_GROUP_SIZE_UNIT == 16);
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

        bool operator == (AssetGroupIdentity other) const {
            return volume == other.volume && ordinal == other.ordinal;
        }
    };

    struct LoadedAssetGroupRecord {
        AssetGroupSize size;
        AssetGroupIdentity identity;

        bool isEmpty() const {
            return identity.ordinal == 0xFF;
        }
    };

    struct AssetSlotRecord {
        enum Flags {
            F_LOAD_IN_PROGRESS = (1 << 0),
        };

        uint8_t flags;

        // Variable size. Empty groups can be truncated when writing.
        LoadedAssetGroupRecord groups[ASSET_GROUPS_PER_SLOT];

        static Key makeKey(Key cubeKey, unsigned slot);
        static bool decodeKey(Key slotKey, Key &cubeKey, unsigned &slot);

        bool findGroup(AssetGroupIdentity identity, unsigned &offset) const;
        bool allocGroup(AssetGroupIdentity identity, unsigned numTiles, unsigned &offset);
        unsigned totalTiles() const;
        unsigned totalGroups() const;
        bool isEmpty() const;

        void init();
        bool load(const FlashLFSObjectIter &iter);
        unsigned writeableSize() const;

        unsigned ALWAYS_INLINE tilesFree() const {
            return TILES_PER_ASSET_SLOT - totalTiles();
        }
    };

    /*
     * Accessors
     */

    inline FlashLFS &get()
    {
        STATIC_ASSERT(kEnd <= _SYS_FS_MAX_OBJECT_KEYS);
        STATIC_ASSERT(sizeof(FlashMapBlock) == 1);
        STATIC_ASSERT(sizeof(LoadedAssetGroupRecord) == 3);
        STATIC_ASSERT(sizeof(AssetSlotRecord) == 3 * ASSET_GROUPS_PER_SLOT + 1);
        STATIC_ASSERT(sizeof(AssetSlotOverviewRecord) == 4);
        STATIC_ASSERT(sizeof(CubeRecord) == ASSET_SLOTS_PER_CUBE * sizeof(AssetSlotOverviewRecord));

        return FlashLFSCache::get(FlashMapBlock::invalid());
    }

    // System equivalents to _SYS_fs_objectRead/Write
    int read(Key k, uint8_t *buffer, unsigned bufferSize);
    int write(Key k, const uint8_t *data, unsigned dataSize, bool gc=true);

    template <typename T>
    inline bool read(Key k, T &obj) {
        return read(k, (uint8_t*) &obj, sizeof obj) == sizeof obj;
    }

    template <typename T>
    inline bool write(Key k, const T &obj, bool gc=true) {
        return write(k, (const uint8_t*) &obj, sizeof obj, gc) == sizeof obj;
    }

    void deleteAll();
    void deleteCube(unsigned index);

} // end namespace SysLFS


#endif
