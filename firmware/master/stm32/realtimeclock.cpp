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

#include "realtimeclock.h"
#include "tasks.h"
#include "macros.h"

void RealTimeClock::beginInit()
{
    /*
     * Because it can take up to a second or more if we're
     * waking up after a battery insertion, split the init
     * process in 2.
     */

    // Enable power and backup interface clocks
    // and access to RTC and Backup registers
    RCC.APB1ENR |= (1 << 27) | (1 << 28);
    PWR.CR |= (1 << 8); // DBP: Disable backup domain write protection

    // assume LSE as the clock source
    // support others eventually if we need them...

    // are we waking up from a battery removal?
    if ((RCC.BDCR & (1 << 15)) == 0) {

        RCC.BDCR |= (1 << 16);  // BDRST - backup domain reset
        RCC.BDCR &= ~(1 << 16);

        // turn on LSE on and wait while it stabilizes
        RCC.BDCR |= (1 << 0);   // LSEON
    }
}

void RealTimeClock::finishInit()
{
    /*
     * Ensure our init process has completed.
     *
     * XXX: this can take a long time - consider a better way
     *      to avoid waiting for this on start up after
     *      battery insert.
     */

    if ((RCC.BDCR & (1 << 15)) == 0) {

        // are we waking up from a battery removal?
        // we set LSEON above...hopefully it made some progress in the meantime
        while ((RCC.BDCR & (1 << 1)) == 0) {    // LSERDY
            Tasks::work();
        }

        // select LSE as the source
        RCC.BDCR &= ~((1 << 8) | (1 << 9));
        RCC.BDCR |= (1 << 8);

        // Enable the RTC
        RCC.BDCR |= (1 << 15);
    }

    // wait for Registers Synchronized Flag
    RTC.CRL &= ~(1 << 3);
    while (!(RTC.CRL & (1 << 3))) {
        ;
    }
}

void RealTimeClock::setCount(uint32_t v)
{
    beginConfig();

    RTC.CNTH = (v & 0xffff0000) >> 16; // CNT[31:16]
    RTC.CNTL = v & 0x0000ffff;         // CNT[15:0]

    endConfig();
}
