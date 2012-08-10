/*
 * Thundercracker Firmware -- Confidential, not for redistribution.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#ifndef _ASSETUTIL_H
#define _ASSETUTIL_H

#include <sifteo/abi.h>
#include <protocol.h>
#include "macros.h"
#include "svmmemory.h"
#include "flash_syslfs.h"


/**
 * AssetUtil is home to some random asset-related utilities that are not
 * specifically related to asset loading.
 */

class AssetUtil {
public:

    static _SYSAssetGroupCube *mapGroupCube(SvmMemory::VirtAddr group, _SYSCubeID cid);
    static _SYSAssetLoaderCube *mapLoaderCube(SvmMemory::VirtAddr loader, _SYSCubeID cid);

    static ALWAYS_INLINE _SYSAssetGroupCube *mapGroupCube(_SYSAssetGroup *group, _SYSCubeID cid) {
        return mapGroupCube(reinterpret_cast<SvmMemory::VirtAddr>(group), cid);
    }

    static ALWAYS_INLINE _SYSAssetLoaderCube *mapLoaderCube(_SYSAssetLoader *loader, _SYSCubeID cid) {
        return mapLoaderCube(reinterpret_cast<SvmMemory::VirtAddr>(loader), cid);
    }

    static unsigned loadedBaseAddr(SvmMemory::VirtAddr group, _SYSCubeID cid);

    static bool isValidConfig(const _SYSAssetConfiguration *cfg, unsigned cfgSize);

private:
    AssetUtil();  // Do not implement
};


/**
 * This object holds critical AssetGroup data which requires some
 * context-specific help to gather: A local copy of its header,
 * the full AssetGroupIdentity, and the virtual address of the
 * usermode SYSAssetGroup structure.
 *
 * This is a parameter to VirtAssetSlots::locateGroup()
 */

struct AssetGroupInfo {
    SvmMemory::VirtAddr va;
    SvmMemory::VirtAddr headerVA;
    uint32_t dataSize;
    uint16_t numTiles;
    uint8_t ordinal;
    FlashVolume volume;
    bool remapToVolume;

    bool fromUserPointer(const _SYSAssetGroup *group);
    bool fromAssetConfiguration(const _SYSAssetConfiguration *config);

    void copyCRC(uint8_t *buffer) const;

    SysLFS::AssetGroupIdentity identity() const
    {
        SysLFS::AssetGroupIdentity result;
        result.ordinal = ordinal;
        result.volume = volume.block.code;
        return result;
    }
};


/**
 * This is an access utility for _SYSAssetLoaderCube FIFOs. It's a short-lived
 * object which holds a consistent local copy of the FIFO's state, and provides
 * safe accessors for reading and writing it.
 */

class AssetFIFO {
public:
    static const unsigned ADDRESS_SIZE = 3;
    static const unsigned CRC_QUERY_SIZE = 3;

    ALWAYS_INLINE AssetFIFO(_SYSAssetLoaderCube &sys)
        : sys(sys), head(sys.head), tail(sys.tail)
    {
        // Must clamp the head and tail pointer, since we can't trust userspace.
        ASSERT(head < _SYS_ASSETLOAD_BUF_SIZE);
        ASSERT(tail < _SYS_ASSETLOAD_BUF_SIZE);
        head = MIN(head, _SYS_ASSETLOAD_BUF_SIZE - 1);
        tail = MIN(tail, _SYS_ASSETLOAD_BUF_SIZE - 1);
        count = umod(tail - head, _SYS_ASSETLOAD_BUF_SIZE);
        ASSERT(count < _SYS_ASSETLOAD_BUF_SIZE);
    }

    static unsigned fetchFromGroup(_SYSAssetLoaderCube &sys, AssetGroupInfo &group, unsigned offset);

    ALWAYS_INLINE unsigned readAvailable() const {
        return count;
    }

    ALWAYS_INLINE unsigned writeAvailable() const {
        return (_SYS_ASSETLOAD_BUF_SIZE - 1) - count;
    }

    ALWAYS_INLINE uint8_t read()
    {
        ASSERT(head < _SYS_ASSETLOAD_BUF_SIZE);
        ASSERT(count > 0);
        uint8_t byte = sys.buf[head];
        if (++head == _SYS_ASSETLOAD_BUF_SIZE)
            head = 0;
        count--;
        return byte;
    }

    ALWAYS_INLINE void write(uint8_t byte)
    {
        ASSERT(tail < _SYS_ASSETLOAD_BUF_SIZE);
        ASSERT(count < (_SYS_ASSETLOAD_BUF_SIZE - 1));
        sys.buf[tail] = byte;
        if (++tail == _SYS_ASSETLOAD_BUF_SIZE)
            tail = 0;
        count++;
    }

    ALWAYS_INLINE void writeAddress(unsigned addr)
    { 
        // Opcode, lat1, lat2:a21
        ASSERT(addr < 0x8000);
        write(FLS_OP_ADDRESS);
        write(addr << 1);
        write(((addr >> 6) & 0xfe) | ((addr >> 14) & 1));
    }

    ALWAYS_INLINE void writeCRCQuery(unsigned id, unsigned numTiles)
    {
        ASSERT(id & 0x80);
        ASSERT(numTiles > 0);
        write(FLS_OP_QUERY_CRC);
        write(id);
        write(ceildiv<unsigned>(numTiles, _SYS_ASSET_GROUP_SIZE_UNIT));
    }

    ALWAYS_INLINE void commitReads() const {
        sys.head = head;
    }

    ALWAYS_INLINE void commitWrites() const {
        sys.tail = tail;
    }

    ALWAYS_INLINE uint8_t& spare(unsigned n)
    {
        /*
         * Access 'spare' byte "N". These bytes begin with N=0
         * indicating the byte at 'tail', and subsequent bytes
         * coming after that.
         * 
         * This can be used to store and retrieve additional data
         * in the unused portion of the FIFO, so the FIFO can be
         * reclaimed as general purpose memory during periods of
         * low activity.
         *
         * Does not affect the head/tail pointers.
         */

        n += tail;
        if (n >= _SYS_ASSETLOAD_BUF_SIZE)
            n -= _SYS_ASSETLOAD_BUF_SIZE;
        ASSERT(n < _SYS_ASSETLOAD_BUF_SIZE);

        return sys.buf[n];
    }

private:
    _SYSAssetLoaderCube &sys;
    unsigned head;
    unsigned tail;
    unsigned count;
};


#endif
