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
    enum SampleRate {
        SampleRate_1_5,     // 1.5 ADC cycles
        SampleRate_7_5,     // 7.5 ADC cycles
        SampleRate_13_5,    // 13.5 ADC cycles
        SampleRate_28_5,    // 28.5 ADC cycles
        SampleRate_41_5,    // 41.5 ADC cycles
        SampleRate_55_5,    // 55.5 ADC cycles
        SampleRate_71_5,    // 71.5 ADC cycles
        SampleRate_239_5    // 239.5 ADC cycles
    };

    Adc(volatile ADC_t *adc) :
        hw(adc)
    {}

    void init();

    void setSampleRate(uint8_t channel, SampleRate rate);
    uint16_t sample(uint8_t channel);

private:
    volatile ADC_t *hw;
};

#endif // ADC_H
