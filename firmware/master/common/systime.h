/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * This file is part of the internal implementation of the Sifteo SDK.
 * Confidential, not for redistribution.
 *
 * Copyright <c> 2011 Sifteo, Inc. All rights reserved.
 */

#ifndef _SYSTIME_H
#define _SYSTIME_H

/*
 * Platform-specific time interfaces.
 */

#include <sifteo/abi.h>

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
     */

    static Ticks sTicks(unsigned seconds) {
        return seconds * 1000000000ULL;
    }

    static Ticks msTicks(unsigned milliseconds) {
        return milliseconds * 1000000ULL;
    }

    static Ticks usTicks(unsigned microseconds) {
        return microseconds * 1000ULL;
    }

    static Ticks nsTicks(unsigned nanoseconds) {
        return nanoseconds;
    }

    static Ticks hzTicks(unsigned hz) {
        return 1000000000ULL / hz;
    }
};

#endif
