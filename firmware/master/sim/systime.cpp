/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * This file is part of the internal implementation of the Sifteo SDK.
 * Confidential, not for redistribution.
 *
 * Copyright <c> 2011 Sifteo, Inc. All rights reserved.
 */

#include <sys/time.h>
#include "systime.h"


void SysTime::init()
{
    /* Nothing to do yet */
}

SysTime::Ticks SysTime::ticks()
{
    /*
     * XXX: This is supposed to be monotonic. gettimeofday()
     *      is notably very NOT monotonic, but perhaps this is good
     *      enough for the simulator...
     */

    struct timeval tv;
    gettimeofday(&tv, 0);
    return sTicks(tv.tv_sec) + usTicks(tv.tv_usec);
}
