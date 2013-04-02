/*
 * Thundercracker Firmware -- Confidential, not for redistribution.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#include "adc.h"

Adc Adc::Adc1(&ADC1);

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

    /*
     * Enable SWSTART as our external event selection, default channel selection
     * to none, and enable the periph.
     */
    enableInterrupt();
    hw->CR2 = 7 << 17;
    hw->SQR1 = 0;
    hw->SQR2 = 0;
    hw->SQR3 = 0;
    hw->SMPR1 = 0;
    hw->SMPR2 = 0;
    hw->CR2 |= 0x1;

    // reset calibration & wait for it to complete
    const uint32_t resetCalibration = (1 << 3);
    hw->CR2 |= resetCalibration;
    while (hw->CR2 & resetCalibration)
        ;

    // perform calibration & wait for it to complete
    const uint32_t calibrate = (1 << 2);
    hw->CR2 |= calibrate;
    while (hw->CR2 & calibrate)
        ;
}

void Adc::enableInterrupt() {
    hw->CR1 |= 1 << 5;       // enable EOCIE
}

void Adc::disableInterrupt() {
    hw->CR1 &= ~(1 << 5);    // disable EOCIE
}

void Adc::setSampleRate(uint8_t channel, SampleRate rate)
{
    if (channel < 10) {
        hw->SMPR2 |= rate << (channel * 3);
    } else {
        hw->SMPR1 |= rate << ((channel - 10) * 3);
    }
}

void Adc::setCallback(uint8_t channel, AdcIsr_t funct)
{
    handlers[channel] = funct;
}

void Adc::beginSample(uint8_t channel)
{
    /*
     * Kick off a new sample if there's not already one in progress.
     */

    if (!isBusy()) {
        hw->SQR3 = channel;
        hw->CR2 |= ((1 << 22) | (1 << 20));     // SWSTART the conversion
    }
}

uint16_t Adc::sampleSync(uint8_t channel)
{
    /*
     * Inefficient but simple synchronous sample.
     */

    hw->SQR3 = channel;

    hw->CR2 |= ((1 << 22) | (1 << 20));     // SWSTART the conversion
    while (!(hw->SR & (1 << 1)))            // wait for EOC
        ;
    return hw->DR;
}

IRQ_HANDLER ISR_ADC1_2()
{
    Adc::Adc1.serveIsr();
}

void Adc::serveIsr()
{
    if (hw->SR & (1 << 1)) {    // check for EOC

        unsigned channel = hw->SQR3;
        AdcIsr_t &callback = handlers[channel];

        if (callback) {
            callback(hw->DR);
        }
    }
}
