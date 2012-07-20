/*
 * Thundercracker Firmware -- Confidential, not for redistribution.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#ifndef _NEIGHBOR_PROTOCOL_H
#define _NEIGHBOR_PROTOCOL_H

#include <stdint.h>


namespace Neighbor {

    /*
     * Timers are set to the neighbor transmission's bit width of 16us.
     * Units are in APB1 ticks, which has a rate of 36MHz.
     *
     * Pulse duration is 2us.
     *
     * 2us  / (1 / 36000000) == 72 ticks
     * 12us / (1 / 36000000) == 432 ticks (this is the old bit period width, if you ever need it)
     * 16us / (1 / 36000000) == 576 ticks
     */
    static const unsigned PULSE_LEN_TICKS = 72;
    static const unsigned BIT_PERIOD_TICKS = 576;

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

}

#endif
