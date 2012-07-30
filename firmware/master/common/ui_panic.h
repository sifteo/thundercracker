/*
 * Thundercracker Firmware -- Confidential, not for redistribution.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#ifndef _UI_PANIC_H
#define _UI_PANIC_H

#include <sifteo/abi.h>
#include "svmmemory.h"

/**
 * This is a low-level way to splat text onto a cube, with
 * few expectations that the system is set up in any particular way.
 *
 * This can be used to display messages when nothing else is available:
 * for example, in the case of an unhandled fault in the loader, or
 * during debugging.
 *
 * In order to make this runnable at any time, even deep in interrupts,
 * we opt to steal memory from userspace instead of the stack.
 * This means it's impossible to continue executing userspace code
 * after using UIPanic.
 */

class UIPanic {
public:
    _SYSAttachedVideoBuffer *avb;
    uint16_t addr;

    void init(SvmMemory::VirtAddr vbufVA);
    void erase();
    void paint(_SYSCubeID cube);
    void paintAndWait();

    /// This is a standard way to halt execution indefinitely, in case of fatal error.
    static void haltForever();

    /// Like haltForever(), but allow resuming on home button press/release
    static void haltUntilButton();

    UIPanic &at(int x, int y) {
        addr = x + _SYS_VRAM_BG0_WIDTH * y;
        return *this;
    }

    UIPanic &operator<< (char c);
    UIPanic &operator<< (const char *str);
    UIPanic &operator<< (uint8_t byte);
    UIPanic &operator<< (uint32_t word);

private:
    void dumpScreenToUART();
};


#endif
