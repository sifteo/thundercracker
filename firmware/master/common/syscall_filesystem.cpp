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

    // TO DO
}

void *_SYS_elf_metadata(_SYSVolumeHandle volHandle, unsigned key, unsigned minSize, unsigned *actualSize)
{
    FlashVolume vol(volHandle);

    if (!vol.isValid()) {
        SvmRuntime::fault(F_BAD_VOLUME_HANDLE);
        return NULL;
    }

    // TO DO
    return NULL;
}


}  // extern "C"
