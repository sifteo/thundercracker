/*
 * Thundercracker Firmware -- Confidential, not for redistribution.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#ifndef _NEIGHBOR_RX_H
#define _NEIGHBOR_RX_H

#include <stdint.h>

class NeighborRX {
public:

    // One-time hardware init
    static void init();

    // Start or stop receiving, with a specified subset of sides
    static void start(unsigned sideMask);
    static void stop();

    // Callback for received data.
    static void callback(unsigned side, unsigned data);

    // Hardware glue
    static void pulseISR(unsigned side);

};

#endif
