/*
 * Thundercracker Firmware -- Confidential, not for redistribution.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#ifndef _PANIC_H
#define _PANIC_H

#include <sifteo/abi.h>
#include "svmmemory.h"

/**
 * This is a low-level way to splat text onto a cube, with
 * few expectations that the system is set up in any particular way.
 *
 * This can be used to display messages when nothing else is available:
 * for example, in the case of an unhandled fault in the loader, or
 * during debugging.
 */

struct PanicMessenger {
    _SYSAttachedVideoBuffer *avb;
    uint16_t addr;

    void init(SvmMemory::VirtAddr vbufVA);
    void erase();
    void paint(_SYSCubeID cube);

    PanicMessenger &at(int x, int y) {
        addr = x + _SYS_VRAM_BG0_WIDTH * y;
        return *this;
    }

    PanicMessenger &operator<< (char c);
    PanicMessenger &operator<< (const char *str);
    PanicMessenger &operator<< (uint8_t byte);
    PanicMessenger &operator<< (uint32_t word);
};


#endif
