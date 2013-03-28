/*
 * Thundercracker Firmware -- Confidential, not for redistribution.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#ifndef ADC_H
#define ADC_H

#include "hardware.h"
#include "macros.h"

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

    Adc(volatile ADC_t *adc) :
        hw(adc)
    {}

    void init();
    void setCallback(uint8_t ch, AdcIsr_t funct);
    void setSampleRate(uint8_t ch, SampleRate rate);
    bool sample(uint8_t ch);

    static Adc Adc1;    // shared instance

private:
    volatile ADC_t *hw;

    ALWAYS_INLINE bool isBusy() const {
        return hw->SR & (1 << 1);
    }
    void serveIsr();
    AdcIsr_t handlers[16];

    friend void ISR_ADC1_2();
};

#endif // ADC_H
