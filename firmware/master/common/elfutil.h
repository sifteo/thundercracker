/*
 * Thundercracker Firmware -- Confidential, not for redistribution.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#ifndef ELF_UTIL_H
#define ELF_UTIL_H

#include <stdint.h>
#include <inttypes.h>

#include "svmmemory.h"

namespace Elf {

    struct Segment {
        uint32_t vaddr;
        uint32_t size;
        FlashRange data;
    };

    struct ProgramInfo {
        uint32_t entry;
        Segment rodata;
        Segment rwdata;
        Segment bss;

        bool init(const FlashRange &elf);
    };

};

#endif // ELF_UTIL_H
