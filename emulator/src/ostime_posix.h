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
#include <time.h>
#include <sys/time.h>

class OSTimeData
{
public:
    struct timeval epoch;

    OSTimeData() {
        gettimeofday(&epoch, 0);
    }
};

inline double OSTime::clock()
{
    struct timeval now;
    gettimeofday(&now, 0);
    return (now.tv_sec - data.epoch.tv_usec) +
        1e-6 * (now.tv_usec - data.epoch.tv_usec);
}

inline void OSTime::sleep(double seconds)
{
    struct timespec tv;
    tv.tv_sec = seconds;
    tv.tv_nsec = (seconds - tv.tv_sec) * 1e9;
    nanosleep(&tv, 0);
}
