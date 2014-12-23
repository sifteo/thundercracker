/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * Sifteo Thundercracker simulator
 * Micah Elizabeth Scott <micah@misc.name>
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
