/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * Sifteo Thundercracker simulator
 * M. Elizabeth Scott <beth@sifteo.com>
 *
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

/*
 * Utilities for working with the host operating system's notion of time.
 * Retrieve the system's monotonic high-res timer, and perform high-resolution
 * single-thread sleeps.
 */

#ifndef _OSTIME_H
#define _OSTIME_H

class OSTimeData;

class OSTime {
public:
    static double clock();
    static void sleep(double seconds);

private:
    static OSTimeData data;
};

#define _OSTIME_IMPL

#ifdef _WIN32
#   include "ostime_win32.h"
#else
#   ifdef __APPLE__
#      include "ostime_mach.h"
#   else
#      include "ostime_posix.h"
#   endif
#endif

#undef _OSTIME_IMPL

#endif
