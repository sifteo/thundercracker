/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * Thundercracker cube firmware
 *
 * M. Elizabeth Scott <beth@sifteo.com>
 * Copyright <c> 2011 Sifteo, Inc. All rights reserved.
 */

#include "time.h"

void msleep(uint8_t msec)
{
    /*
     * XXX: Right now we're busy-looping, and we ignore time spent
     *      in interrupts. This should be rewritten to use the timer peripherals.
     */

    register uint8_t c = 0;
    register uint8_t b;
    register uint8_t a = msec;

    do {
        b = 20;
        do {
            do {} while (--c);
        } while (--b);
    } while (--a);
}
