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
