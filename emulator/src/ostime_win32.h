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

#include <windows.h>
#include <mmsystem.h>
#include <stdint.h>
#include "macros.h"

class OSTimeData
{
public:
    bool hasQPC;
    double toSeconds;
    
    OSTimeData() {
        LARGE_INTEGER freq;
        if (QueryPerformanceFrequency(&freq)) {
            hasQPC = true;
            toSeconds = 1.0 / (double)freq.QuadPart;
        } else {
            hasQPC = false;
            LOG(("WARNING: No high-resolution timer available!\n"));
        }
        
        // Ask for 1ms resolution in Sleep(), rather than the default 10ms!
        timeBeginPeriod(1);
    }
};

inline double OSTime::clock()
{
    if (data.hasQPC) {
        LARGE_INTEGER t;
        QueryPerformanceCounter(&t);
        return t.QuadPart * data.toSeconds;
    }

    return GetTickCount() * 1e-3;
}

inline void OSTime::sleep(double seconds)
{
    int ms = seconds * 1000 - 1;
    if (ms >= 1)
        Sleep(ms);
}
