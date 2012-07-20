/*
 * Thundercracker Firmware -- Confidential, not for redistribution.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#ifndef _NEIGHBOR_TX_H
#define _NEIGHBOR_TX_H

#include <stdint.h>

class NeighborTX {
public:

    // One-time hardware init
    static void init();

    // Transmit a particular 16-bit packet repeatedly
    static void start(unsigned data, unsigned sideMask);
    static void stop();

    // Change TX pin state
    static void floatSide(unsigned side);
    static void squelchSide(unsigned side);

};

#endif
