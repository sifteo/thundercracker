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
#include "svmmemory.h"
#include "svmruntime.h"
#include "svmloader.h"
#include "elfprogram.h"

extern "C" {


uint32_t _SYS_fs_listVolumes(unsigned volType, _SYSVolumeHandle *results, uint32_t maxResults)
{
    if (!SvmMemory::mapRAM(results, mulsat16x16(sizeof *results, maxResults))) {
        SvmRuntime::fault(F_SYSCALL_ADDRESS);
        return 0;
    }

    // Some volume types can't be iterated over from userspace
    if (volType == FlashVolume::T_INCOMPLETE ||
        volType == FlashVolume::T_DELETED ||
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


}  // extern "C"
