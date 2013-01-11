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
    typedef void (*AdcIsr_t)(uint16_t sample);

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

    static void init();
    static void setCallback(uint8_t ch, AdcIsr_t funct);
    static void setSampleRate(uint8_t ch, SampleRate rate);
    static bool sample(uint8_t ch);

private:
    static bool isBusy();

    static void serveIsr(AdcIsr_t &handler);

    static AdcIsr_t ADCHandlers[16];

    friend void ISR_ADC1_2();
};

#endif // ADC_H
