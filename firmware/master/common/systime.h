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
     * Get the current system timer.  Guaranteed to be
     * monotonic. Units are undefined, they may be platform-specific.
     */

    typedef int64_t Ticks;
    static Ticks ticks();

    /*
     * Convert common things to Ticks.
     *
     * XXX: Needs porting love. Currently assumes Ticks == nanoseconds,
     *      which we may not want to do on hardware.
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
