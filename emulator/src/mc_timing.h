/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * Sifteo Thundercracker simulator
 * M. Elizabeth Scott <beth@sifteo.com>
 *
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
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
    static const unsigned TICKS_PER_PAGE_WRITE = 16512;
    static const unsigned TICKS_PER_BLOCK_ERASE = 8000000;

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
