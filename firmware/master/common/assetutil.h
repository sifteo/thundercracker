/*
 * Thundercracker Firmware -- Confidential, not for redistribution.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#ifndef _ASSETUTIL_H
#define _ASSETUTIL_H

#include <sifteo/abi.h>
#include "macros.h"
#include "svmmemory.h"


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
 * This is an access utility for _SYSAssetLoaderCube FIFOs. It's a short-lived
 * object which holds a consistent local copy of the FIFO's state, and provides
 * safe accessors for reading and writing it.
 */

class AssetFIFO {
public:
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
		head++;
        if (head == _SYS_ASSETLOAD_BUF_SIZE)
            head = 0;
        count--;
        return byte;
    }

    ALWAYS_INLINE void write(uint8_t byte)
    {
		ASSERT(tail < _SYS_ASSETLOAD_BUF_SIZE);
		ASSERT(count < (_SYS_ASSETLOAD_BUF_SIZE - 1));
		sys.buf[tail] = byte;
		tail++;
        if (tail == _SYS_ASSETLOAD_BUF_SIZE)
            tail = 0;
        count++;
    }

    ALWAYS_INLINE void commitReads() const {
    	sys.head = head;
    }

    ALWAYS_INLINE void commitWrites() const {
    	sys.tail = tail;
    }

private:
	_SYSAssetLoaderCube &sys;
	unsigned head;
	unsigned tail;
	unsigned count;
};


#endif
