/*
 * Thundercracker Firmware -- Confidential, not for redistribution.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#ifndef _PULSE_RX_H
#define _PULSE_RX_H

#include <stdint.h>
#include "gpio.h"


class PulseRX {
public:
    PulseRX(GPIOPin);

    void init();

    void start();
    void stop();

    uint16_t count();

    void pulseISR();

private:
    static const uint16_t maxCount = 65535;
    GPIOPin pin;
    uint16_t pulseCount;
};

#endif
