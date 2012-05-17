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
#include "flash_volume.h"

extern "C" {


uint32_t _SYS_fs_listVolumes(unsigned volType, _SYSVolumeHandle *results, uint32_t maxResults)
{
    return 0;
}

void _SYS_elf_exec(_SYSVolumeHandle vol)
{
}

void *_SYS_elf_metadata(_SYSVolumeHandle vol, unsigned key, unsigned minSize, unsigned *actualSize)
{
    return 0;
}


}  // extern "C"
