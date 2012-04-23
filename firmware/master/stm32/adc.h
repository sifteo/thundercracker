/*
 * Thundercracker Firmware -- Confidential, not for redistribution.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#ifndef ADC_H
#define ADC_H

#include "hardware.h"

class Adc
{
public:
    Adc(volatile ADC_t *adc) :
        hw(adc)
    {}

    void init();
    void on() {
        hw->CR2 |= (1 << 0); // set ADON
        // TODO - wait 2 ADC clock cycles to ensure it has stabilized
    }
    void off() {
        hw->CR2 &= ~(1 << 0); // clear ADON
    }

    uint16_t sample();

private:
    volatile ADC_t *hw;
};

#endif // ADC_H
