/*
 * Thundercracker Firmware -- Confidential, not for redistribution.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#include "svmclock.h"

SvmClock::Ticks SvmClock::pauseTimestamp;
SvmClock::Ticks SvmClock::clockOffset;


void SvmClock::init()
{
    // Not paused, sync'ed with system time
    pauseTimestamp = NOT_PAUSED;
    clockOffset = 0;
}

void SvmClock::pause()
{
    ASSERT(!isPaused());
    pauseTimestamp = SysTime::ticks();
    ASSERT(isPaused());
}

void SvmClock::resume()
{
    ASSERT(isPaused());
    clockOffset -= SysTime::ticks() - pauseTimestamp;
    pauseTimestamp = NOT_PAUSED;
}
