/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * Sifteo Thundercracker simulator
 * M. Elizabeth Scott <beth@sifteo.com>
 *
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#ifndef _OSTIME_IMPL
#error This header must only be included by ostime.h
#endif

#include <stdint.h>
#include <mach/mach_time.h>

// Grr.. badly namespaced macros in Mach headers.
#undef PAGE_SIZE

class OSTimeData
{
public:
    double toSeconds;
    uint64_t epoch;

    OSTimeData() {
        mach_timebase_info_data_t info;
        mach_timebase_info(&info);
        toSeconds = (1e-9 * info.numer) / info.denom;
        epoch = mach_absolute_time();
    }
};

inline double OSTime::clock()
{
    return (mach_absolute_time() - data.epoch) * data.toSeconds;
}

inline void OSTime::sleep(double seconds)
{
    mach_wait_until(mach_absolute_time() + seconds / data.toSeconds);
}
