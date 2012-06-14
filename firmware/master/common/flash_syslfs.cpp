/*
 * Thundercracker Firmware -- Confidential, not for redistribution.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#include "flash_syslfs.h"
#include "crc.h"


int SysLFS::read(unsigned key, uint8_t *buffer, unsigned bufferSize)
{
    // System internal version of _SYS_fs_objectRead()
    
    ASSERT(FlashLFSIndexRecord::isKeyAllowed(key));

    FlashLFS &lfs = SysLFS::get();
    FlashLFSObjectIter iter(lfs);

    while (iter.previous(key)) {
        unsigned size = iter.record()->getSizeInBytes();
        size = MIN(size, bufferSize);
        if (iter.readAndCheck(buffer, size))
            return size;
    }

    return _SYS_ENOENT;
}

int SysLFS::write(unsigned key, const uint8_t *data, unsigned dataSize)
{
    // System internal version of _SYS_fs_objectWrite()

    ASSERT(FlashLFSIndexRecord::isKeyAllowed(key));
    ASSERT(FlashLFSIndexRecord::isSizeAllowed(dataSize));

    CrcStream cs;
    cs.reset();
    cs.addBytes(data, dataSize);
    uint32_t crc = cs.get(FlashLFSIndexRecord::SIZE_UNIT);

    FlashLFS &lfs = SysLFS::get();
    FlashLFSObjectAllocator allocator(lfs, key, dataSize, crc);

    if (!allocator.allocateAndCollectGarbage())
        return _SYS_ENOSPC;

    FlashBlock::invalidate(allocator.address(), allocator.address() + dataSize);
    FlashDevice::write(allocator.address(), data, dataSize);
    return dataSize;
}

void SysLFS::CubeRecord::read(_SYSCubeID cube)
{
    if (SysLFS::read(cubeKey(cube), *this))
        return;

    // Initialize with default contents
    memset(this, 0, sizeof *this);
}

void SysLFS::CubeRecord::write(_SYSCubeID cube)
{
    if (SysLFS::write(cubeKey(cube), *this))
        return;

    // Updates will be lost if there's no space to store them!
    LOG(("SYSLFS: Failed to write updated cube record\n"));
}
