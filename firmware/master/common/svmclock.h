/*
 * Thundercracker Firmware -- Confidential, not for redistribution.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#ifndef SVM_CLOCK_H
#define SVM_CLOCK_H

#include "macros.h"
#include "systime.h"


class SvmClock {
public:
    typedef SysTime::Ticks Ticks;

    static ALWAYS_INLINE bool isPaused() {
        return pauseTimestamp != NOT_PAUSED;
    }

    /**
     * Return the current time, as seen by userspace.
     * Assumes we aren't paused.
     */
    static ALWAYS_INLINE Ticks ticks()
    {
        ASSERT(!isPaused());
        Ticks t = SysTime::ticks() + clockOffset;
        ASSERT(t > 0);
        return t;
    }

    static void init();
    static void pause();
    static void resume();

private:
    SvmClock();  // Do not implement

    static const Ticks NOT_PAUSED = -1;

    static Ticks pauseTimestamp;
    static Ticks clockOffset;
};


#endif // SVM_CLOCK_H
