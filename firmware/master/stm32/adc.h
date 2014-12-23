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


    ALWAYS_INLINE void enableEocInterrupt() {
        hw->CR1 |= (1 << 5);
    }

    ALWAYS_INLINE void disableEocInterrupt() {
        hw->CR1 &= ~(1 << 5);
    }

    void beginSample(uint8_t ch);
    uint16_t sampleSync(uint8_t channel);

    static Adc Adc1;    // shared instance

private:
    volatile ADC_t *hw;

    ALWAYS_INLINE bool isBusy() const {
        /*
         * We clear STRT in the EOC handler, so we can
         * effectively use it as a busy flag
         */
        return hw->SR & (1 << 4);
    }
    void serveIsr();
    AdcIsr_t handlers[16];

    friend void ISR_ADC1_2();
};

#endif // ADC_H
