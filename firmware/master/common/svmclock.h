/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * Thundercracker firmware
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
