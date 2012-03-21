/*
 * Thundercracker Firmware -- Confidential, not for redistribution.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#include "adc.h"

void Adc::init()
{
    // enable peripheral clock
    if (hw == &ADC1) {
        RCC.APB2ENR |= (1 << 9);
    }
    else if (hw == &ADC2) {
        RCC.APB2ENR |= (1 << 10);
    }
    else if (hw == &ADC3) {
        RCC.APB2ENR |= (1 << 15);
    }

    hw->CR2 |= (1 << 2); // begin calibration
//    while (hw->CR2 & (1 << 2)); // wait for it to complete ?
    hw->SQR1 = 0;   // single conversion in all channels
}

/*
  Super simple (and super inefficient) single conversion mode for now.
  on() must be called before invoking sample() if it was previously off().
*/
uint16_t Adc::sample()
{
    hw->CR2 |= (1 << 0);            // set ADON to start the conversion
    while (!(hw->SR & (1 << 1)));   // wait for EOC
    return hw->DR;                  // automatically resets EOC
}
