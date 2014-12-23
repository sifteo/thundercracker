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
