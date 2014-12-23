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

#ifndef REALTIMECLOCK_H
#define REALTIMECLOCK_H

#include "hardware.h"
#include "macros.h"

class RealTimeClock
{
public:
    static void beginInit();
    static void finishInit();

    static ALWAYS_INLINE uint32_t count() {
        return (RTC.CNTH << 16) | RTC.CNTL;
    }
    static void setCount(uint32_t v);

private:

    static ALWAYS_INLINE void beginConfig() {
        // ensure any previous writes have completed
        while ((RTC.CRL & (1 << 5)) == 0) {
            ;
        }
        RTC.CRL |= (1 << 4);    // CNF: enter configuration mode
    }

    static ALWAYS_INLINE void endConfig() {
        RTC.CRL &= ~(1 << 4);   // Exit configuration mode
    }
};

#endif // REALTIMECLOCK_H
