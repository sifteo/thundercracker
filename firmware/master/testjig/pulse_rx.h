/*
 * Thundercracker Firmware -- Confidential, not for redistribution.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#ifndef _PULSE_RX_H
#define _PULSE_RX_H

#include <stdint.h>

class PulseRX {
public:
    static void init();

    static void start();
    static void stop();

    static uint16_t count();

    static void pulseISR();
};

#endif
