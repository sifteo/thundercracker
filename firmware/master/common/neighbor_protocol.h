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

#ifndef _NEIGHBOR_PROTOCOL_H
#define _NEIGHBOR_PROTOCOL_H

#include <stdint.h>


namespace Neighbor {

    /*
     * APB1 runs at a rate of 36MHz (ie, SYSCLK / 2), but if the divisor for
     * SYSCLK is anything other than 1, TIM2-7 run at APB1 * 2.
     *
     * So, our effective rate for these timers is 72Mhz.
     */
    static const unsigned TICK_HZ = 72000000;

    /*
     * Timers are set to the neighbor transmission's bit width of 16us.
     * Pulse duration is 2us.
     *
     * 2us  / (1 / 72000000) == 144 ticks
     * 15.75us / (1 / 72000000) == 1134 ticks
     */
    static const unsigned PULSE_LEN_TICKS = 144;
    static const unsigned BIT_PERIOD_TICKS = 1134;

    /*
     * We want to start sampling pulses somewhere in the middle of the bit
     * period, ideally as far towards the end of it as possible in order
     * to allow for as much ringdown time as possible.
     *
     * This is basically (BIT_PERIOD_TICKS / 2) plus as much as we can
     * get away with.
     */
    static const unsigned BIT_SAMPLE_PHASE = 600;

    // Number of bits to wait for during an RX sequence
    static const unsigned NUM_RX_BITS = 16;


    /*
     * Number of bit periods to wait between transmissions.
     *
     * We need at least one successful transmit per 162 Hz sensor polling interval,
     * and we need to speak with unpaired cubes which won't be synchronized with our
     * clock yet. So, currently we target a transmit rate of about twice that. If we
     * ever miss two polling intervals in a row, the cube will perceive that we've
     * de-neighbored.
     */
    static const unsigned NUM_TX_WAIT_PERIODS = 192;

    /*
     * Neighbor ID constants
     */

    static const unsigned FIRST_CUBE_ID = 0xE0;
    static const unsigned NUM_CUBE_ID = 24;
    static const unsigned FIRST_MASTER_ID = FIRST_CUBE_ID + NUM_CUBE_ID;
    static const unsigned NUM_MASTER_ID = 8;
}

#endif
