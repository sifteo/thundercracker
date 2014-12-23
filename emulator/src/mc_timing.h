/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * Sifteo Thundercracker simulator
 * M. Elizabeth Scott <beth@sifteo.com>
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

/*
 * Timing constants, for emulating the master cube.
 */

#ifndef _MC_TIMING_H
#define _MC_TIMING_H

#include "macros.h"

struct MCTiming {

    // Frequency of our master simulation clock
    static const unsigned TICK_HZ = 16000000;

    // 450us, minimum packet period (Measured on real hardware)
    static const unsigned TICKS_PER_PACKET = 7200;

    // XXX: Arbitrary unverified time for SVC execution, in ticks
    // (This is especially arbitrary... I guessed ~200 CPU cycles)
    static const unsigned TICKS_PER_SVC = 50;

    // XXX: Arbitrary time for Tasks::work(), necessary so simulator
    // can make forward progress in cases where Tasks are polling for
    // time to elapse. (For example, audio output with --headless)
    static const unsigned TICKS_PER_TASKS_WORK = 10;

    // XXX: Arbitrary unverified time for flash cache miss, in ticks
    // (Based on theoretical 18 MHz / 1.8 MBps bus speed and 256-byte pages,
    // including some generous padding)
    static const unsigned TICKS_PER_PAGE_MISS = 3000;

    // Typical write timing for our flash chip
    static const unsigned TICKS_PER_PAGE_WRITE = 51200;     // min 1.4ms, max 5ms. go with ~3.2ms in the middle
    static const unsigned TICKS_PER_BLOCK_ERASE = 21600000; // min 0.7s, max 2s.   go with ~1.35s in the middle

    // Delay long enough for cubes to establish a HWID, for auto-pairing.
    static const unsigned STARTUP_DELAY = 200000;

    // Fractional number of CPU cycles per tick (72/16 MHz)
    static const unsigned CPU_RATE_NUMERATOR = 9;
    static const unsigned CPU_RATE_DENOMINATOR = 2;

    // CPU cycle counts, pre-multiplied by CPU_RATE_DENOMINATOR.
    static const unsigned CPU_FETCH = CPU_RATE_DENOMINATOR * 1;
    static const unsigned CPU_PIPELINE_RELOAD = CPU_RATE_DENOMINATOR * 2;
    static const unsigned CPU_LOAD_STORE = CPU_RATE_DENOMINATOR * 1;
    static const unsigned CPU_DIVIDE = CPU_RATE_DENOMINATOR * 11;  // Worst case

    // Minimum number of cycles that we bother forwarding to SystemMC in one
    // batch. Pre-multiplied by CPU_RATE_DENOMINATOR.
    static const unsigned CPU_THRESHOLD = CPU_RATE_DENOMINATOR * 100;
};

#endif
