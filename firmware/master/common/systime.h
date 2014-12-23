/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * Thundercracker firmware
 *
 * Copyright <c> 2012 Sifteo, Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#ifndef _SYSTIME_H
#define _SYSTIME_H

/*
 * Platform-specific time interfaces.
 */

#include <sifteo/abi.h>
#include "macros.h"

struct SysTime {

    /*
     * Hardware timer initialization
     */

    static void init();
    
    /*
     * Get the current system timer, a monotonically increasing
     * nanosecond clock.
     *
     * (The "Ticks" type is intentionally vague on units, to limit the
     * amount of code that makes assumptions about our time units, in
     * the event we need to change it in the future).
     */

    typedef int64_t Ticks;
    static Ticks ticks();

    /*
     * Convert common things to and from Ticks.
     *
     * These *must* be inlined - specifically in the case of hzTicks,
     * GCC generates calls to library code for division otherwise,
     * even with constant values.
     */

    static ALWAYS_INLINE Ticks sTicks(unsigned seconds) {
        return seconds * 1000000000ULL;
    }

    static ALWAYS_INLINE Ticks msTicks(unsigned milliseconds) {
        return milliseconds * 1000000ULL;
    }

    static ALWAYS_INLINE Ticks usTicks(unsigned microseconds) {
        return microseconds * 1000ULL;
    }

    static ALWAYS_INLINE Ticks nsTicks(unsigned nanoseconds) {
        return nanoseconds;
    }

    static ALWAYS_INLINE Ticks hzTicks(unsigned hz) {
        return 1000000000ULL / hz;
    }
};

#endif
