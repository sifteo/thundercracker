/*
 * Thundercracker Firmware -- Confidential, not for redistribution.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

/*
 * Syscalls for filesystem actions.
 *
 * This isn't at all meant to be an exhaustive API for the filesystem, but
 * just a way for userspace to do the minimum necessary set of operations.
 * Specifically, this intentionally leaves out the ability to create and
 * delete volumes. That's something we will only support over USB.
 */

#include <sifteo/abi.h>
#include "macros.h"
#include "flash_volume.h"
#include "flash_lfs.h"
#include "svmmemory.h"
#include "svmruntime.h"
#include "svmloader.h"
#include "elfprogram.h"

extern "C" {


uint32_t _SYS_fs_listVolumes(unsigned volType, _SYSVolumeHandle *results, uint32_t maxResults)
{
    if (!isAligned(results)) {
        SvmRuntime::fault(F_SYSCALL_ADDR_ALIGN);
        return 0;
    }
    if (!SvmMemory::mapRAM(results, mulsat16x16(sizeof *results, maxResults))) {
        SvmRuntime::fault(F_SYSCALL_ADDRESS);
        return 0;
    }

    // Some volume types can't be iterated over from userspace
    if (FlashVolume::typeIsInternal(volType) ||
        volType != (uint16_t)volType) {
        SvmRuntime::fault(F_SYSCALL_PARAM);
        return 0;
    }

    unsigned count = 0;
    FlashVolumeIter vi;
    FlashVolume vol;
    vi.begin();

    while (vi.next(vol) && count < maxResults)
        if (vol.getType() == volType)
            results[count++] = vol.getHandle();

    return count;
}

void _SYS_elf_exec(_SYSVolumeHandle volHandle)
{
    FlashVolume vol(volHandle);
    if (!vol.isValid()) {
        SvmRuntime::fault(F_BAD_VOLUME_HANDLE);
        return;
    }

    SvmLoader::exec(vol);
}

void *_SYS_elf_metadata(_SYSVolumeHandle volHandle, unsigned key, unsigned minSize, unsigned *actualSize)
{
    /*
     * Look up a metadata key in the supplied ELF volume. Parameters behave
     * like Elf::Program::getMeta(). The return value is 0 on error, or
     * on success it's the VA where the metadata value would be found after
     * a call to _SYS_elf_map() on this volume.
     */

    if (!isAligned(actualSize)) {
        SvmRuntime::fault(F_SYSCALL_ADDR_ALIGN);
        return 0;
    }

    FlashVolume vol(volHandle);
    if (!vol.isValid()) {
        SvmRuntime::fault(F_BAD_VOLUME_HANDLE);
        return NULL;
    }

    FlashBlockRef mapRef;
    Elf::Program program;
    if (!program.init(vol.getPayload(mapRef))) {
        SvmRuntime::fault(F_BAD_ELF_HEADER);
        return NULL;
    }

    FlashBlockRef tempRef;
    uint32_t localActualSize = 0;
    uint32_t o = program.getMetaSpanOffset(tempRef, key, minSize, localActualSize);

    if (SvmMemory::mapRAM(actualSize))
        *actualSize = localActualSize;

    return o ? reinterpret_cast<void*>(o + SvmMemory::SEGMENT_1_VA) : 0;
}

uint32_t _SYS_elf_map(_SYSVolumeHandle volHandle)
{
    /*
     * Maps the entirety of the specified volume into the alternate
     * address space. Returns an offset that can be added to a normal
     * flash VA to convert it into the mapped address, equal to the
     * mapped location of the read-only data segment minus its VA.
     *
     * If volHandle==0, unmaps the previous mapping and leaves
     * nothing mapped in its place.
     */

    if (volHandle == 0) {
        SvmLoader::secondaryUnmap();
        return 0;
    }

    FlashVolume vol(volHandle);
    if (!vol.isValid()) {
        SvmRuntime::fault(F_BAD_VOLUME_HANDLE);
        return 0;
    }

    FlashBlockRef mapRef;
    Elf::Program program;
    if (!program.init(SvmLoader::secondaryMap(vol))) {
        SvmRuntime::fault(F_BAD_ELF_HEADER);
        return 0;
    }

    FlashBlockRef hdrRef;
    const Elf::ProgramHeader *ro = program.getRODataSegment(hdrRef);
    ASSERT(ro);
    
    return SvmMemory::SEGMENT_1_VA + ro->p_offset - ro->p_vaddr;
}

int32_t _SYS_fs_objectRead(unsigned key, uint8_t *buffer,
    unsigned bufferSize, _SYSVolumeHandle parent)
{
    // Default to reading the running volume, but allow overriding this.
    FlashVolume parentVol;
    if (parent) {
        parentVol = parent;
        if (!parentVol.isValid()) {
            SvmRuntime::fault(F_BAD_VOLUME_HANDLE);
            return _SYS_EINVAL;
        }
    } else {
        parentVol = SvmLoader::getRunningVolume();
        ASSERT(parentVol.isValid());
    }

    if (!FlashLFSIndexRecord::isKeyAllowed(key)) {
        SvmRuntime::fault(F_SYSCALL_PARAM);
        return _SYS_EINVAL;
    }
    if (!SvmMemory::mapRAM(buffer, bufferSize)) {
        SvmRuntime::fault(F_SYSCALL_ADDRESS);
        return _SYS_EFAULT;
    }

    /*
     * Search for the requested object in the index.
     *
     * Traverse backwards from the newest to the oldest, returning
     * the first instance of this key which has a valid CRC.
     *
     * Note that we use the userspace buffer to CRC the object,
     * obviating the need for any separate buffer space. This means
     * it's required that the entire object, excepting any trailing
     * 0xFF padding, must fit in the buffer. If not, we'll notice
     * a CRC failure.
     */

    FlashLFS &lfs = FlashLFSCache::get(parentVol);
    FlashLFSObjectIter iter(lfs);

    while (iter.previous(FlashLFSKeyQuery(key))) {
        unsigned size = iter.record()->getSizeInBytes();
        size = MIN(size, bufferSize);
        if (iter.readAndCheck(buffer, size))
            return size;
    }

    return 0;
}

int32_t _SYS_fs_objectWrite(unsigned key, const uint8_t *data, unsigned dataSize)
{
    // Programs may only write objects in their own local volume
    FlashVolume parentVol = SvmLoader::getRunningVolume();
    ASSERT(parentVol.isValid());

    /*
     * However! If a volume has been deleted but is still running, we want to
     * avoid writing any additional objects to its LFS child.
     */
    if (FlashVolume::typeIsRecyclable(parentVol.getType())) {
        return _SYS_ENOENT;
    }

    if (!FlashLFSIndexRecord::isKeyAllowed(key) ||
        !FlashLFSIndexRecord::isSizeAllowed(dataSize)) {
        SvmRuntime::fault(F_SYSCALL_PARAM);
        return _SYS_EINVAL;
    }

    /*
     * Do the CRC early; we need to catch faults on 'data' before we create
     * the new FS object. We'll ask for the CRC to be padded out to the
     * nearest object allocation unit boundary.
     */

    SvmMemory::VirtAddr va = reinterpret_cast<SvmMemory::VirtAddr>(data);
    FlashBlockRef ref;
    uint32_t crc;

    if (!SvmMemory::crcROData(ref, va, dataSize, crc, FlashLFSIndexRecord::SIZE_UNIT)) {
        SvmRuntime::fault(F_SYSCALL_ADDRESS);
        return _SYS_EFAULT;
    }

    /*
     * Allocate the LFS object. It will only become valid once we've also
     * written data to the filesystem which matches our above CRC.
     */

    FlashLFS &lfs = FlashLFSCache::get(parentVol);
    FlashLFSObjectAllocator allocator(lfs, key, dataSize, crc);

    if (!allocator.allocateAndCollectGarbage())
        return _SYS_ENOSPC;

    /*
     * Write the actual object data. We effectively do a non-cache-polluting
     * write here, by writing directly to the device and invalidating a portion
     * of the cache. (There's no benefit to using the cache here, since it would
     * involve an extra copy from userspace memory to cache memory).
     */

    uint32_t currentAddr = allocator.address();
    uint32_t remainingBytes = dataSize;

    while (remainingBytes) {
        SvmMemory::PhysAddr pa;
        uint32_t chunk = remainingBytes;

        if (!SvmMemory::mapROData(ref, va, chunk, pa)) {
            // Shouldn't fail here, we already touched this memory above.
            ASSERT(0);
            SvmRuntime::fault(F_SYSCALL_ADDRESS);
            return _SYS_EFAULT;
        }

        ASSERT(chunk > 0);
        ASSERT(currentAddr >= allocator.address());
        ASSERT(currentAddr + chunk <= allocator.address() + dataSize);

        FlashDevice::write(currentAddr, pa, chunk);

        // Verify, on siftulator only
        DEBUG_ONLY({
            uint8_t buffer[FlashLFSIndexRecord::MAX_SIZE];
            ASSERT(chunk <= sizeof buffer);
            FlashDevice::read(currentAddr, buffer, chunk);
            ASSERT(0 == memcmp(buffer, pa, chunk));
        });

        va += chunk;
        remainingBytes -= chunk;
        currentAddr += chunk;
    }

    // If any refs are held to the page(s) we touched, they will be reloaded
    // from flash. I
    FlashBlock::invalidate(allocator.address(), allocator.address() + dataSize);

    return dataSize;
}

uint32_t _SYS_fs_runningVolume()
{
    // Return a _SYSVolumeHandle for the currently executing volume

    FlashVolume vol = SvmLoader::getRunningVolume();
    ASSERT(vol.isValid());
    return vol.getHandle();
}

uint32_t _SYS_fs_previousVolume()
{
    // Return a _SYSVolumeHandle for the volume running before the last exec()

    FlashVolume vol = SvmLoader::getPreviousVolume();
    if (vol.block.isValid()) {
        ASSERT(vol.isValid());
        return vol.getHandle();
    }
    return 0;
}

uint32_t _SYS_fs_info(_SYSFilesystemInfo *buffer, uint32_t bufferSize)
{
    if (!SvmMemory::mapRAM(buffer, bufferSize)) {
        SvmRuntime::fault(F_SYSCALL_ADDRESS);
        return 0;
    }

    /*
     * Tabulate the space used by everything in our filesystem
     */

    FlashVolumeIter vi;
    FlashVolume vol;
    _SYSFilesystemInfo info;

    memset(&info, 0, sizeof info);

    info.unitSize = FlashMapBlock::BLOCK_SIZE;
    info.totalUnits = FlashMapBlock::numBlocks();
    info.freeUnits = FlashMapBlock::numBlocks();

    vi.begin();
    while (vi.next(vol)) {
        FlashBlockRef ref;
        FlashVolumeHeader *hdr = FlashVolumeHeader::get(ref, vol.block);
        ASSERT(hdr->isHeaderValid());

        // Ignore deleted/incomplete volumes. (Treat them as free space)
        if (FlashVolume::typeIsRecyclable(hdr->type))
            continue;

        // All other volumes count against our free space
        unsigned volUnits = hdr->numMapEntries();
        info.freeUnits -= volUnits;

        // Is this ours?
        if (SvmLoader::getRunningVolume().block.code == vol.block.code)
            info.selfElfUnits += volUnits;

        // Count other volumes according to their type
        switch (hdr->type) {

            case FlashVolume::T_GAME:
                info.gameElfUnits += volUnits;
                break;

            case FlashVolume::T_LAUNCHER:
                info.launcherElfUnits += volUnits;
                break;

            case FlashVolume::T_LFS:
                // For filesystems, look at the type of their parent

                if (hdr->parentBlock == 0) {
                    // No parent: SysLFS
                    info.systemUnits += volUnits;
                } else {
                    FlashVolume parent = FlashMapBlock::fromCode(hdr->parentBlock);

                    // Is this ours?
                    if (SvmLoader::getRunningVolume().block.code == parent.block.code)
                        info.selfObjUnits += volUnits;

                    // Sort by parent's type
                    switch (parent.getType()) {

                        case FlashVolume::T_GAME:
                            info.gameObjUnits += volUnits;
                            break;

                        case FlashVolume::T_LAUNCHER:
                            info.launcherObjUnits += volUnits;
                            break;
                    }
                }
                break;
        }
    }

    /* 
     * Copy out results.
     *
     * The caller can request a subset of the available data, for forward compatibility.
     * We return the amount of data actually written to the buffer, which could be limited
     * by this firmware's _SYSFilesystemInfo size or by the app's buffer size.
     *
     * If the app gives us a bigger buffer than we know how to fill, pad the rest with zeroes.
     */

    unsigned actualSize = MIN(sizeof info, bufferSize);
    memset(buffer, 0, bufferSize);
    memcpy(buffer, &info, actualSize);
    return actualSize;
}


}  // extern "C"
