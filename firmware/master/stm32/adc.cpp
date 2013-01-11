/*
 * Thundercracker Firmware -- Confidential, not for redistribution.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#include "adc.h"

Adc::AdcIsr_t Adc::ADCHandlers[16];

void Adc::init()
{

    // enable peripheral clock
    RCC.APB2ENR |= (1 << 9); //ADC1

    /*
     * Enable SWSTART as our external event selection, default channel selection
     * to none, and enable the periph.
     */
    ADC1.CR1 = 1 << 5;       // enable EOCIE
    ADC1.CR2 = 7 << 17;
    ADC1.SQR1 = 0;
    ADC1.SQR2 = 0;
    ADC1.SQR3 = 0;
    ADC1.SMPR1 = 0;
    ADC1.SMPR2 = 0;
    ADC1.CR2 |= 0x1;

    // reset calibration & wait for it to complete
    const uint32_t resetCalibration = (1 << 3);
    ADC1.CR2 |= resetCalibration;
    while (ADC1.CR2 & resetCalibration)
        ;

    // perform calibration & wait for it to complete
    const uint32_t calibrate = (1 << 2);
    ADC1.CR2 |= calibrate;
    while (ADC1.CR2 & calibrate)
        ;
}

void Adc::setSampleRate(uint8_t channel, SampleRate rate)
{
    if (channel < 10) {
        ADC1.SMPR2 |= rate << (channel * 3);
    } else {
        ADC1.SMPR1 |= rate << ((channel - 10) * 3);
    }
}

void Adc::setCallback(uint8_t channel, AdcIsr_t funct)
{
    ADCHandlers[channel] = funct;
}

/*
 * Super simple (and inefficient) synchronous single conversion mode for now.
*/
bool Adc::sample(uint8_t channel)
{
    if(!isBusy()){
        ADC1.SQR3 = channel;
        ADC1.CR2 |= ((1 << 22) | (1 << 20));     // SWSTART the conversion
        return true;
    }else{
        return false;                           //returns false if the ADC is busy
    }
}

/*
 * Returns if ADC peripheral is busy
 */
bool Adc::isBusy()
{
    return ADC1.SR & (1 << 1);
}

/*
 * IRQ Handler
 */
IRQ_HANDLER ISR_ADC1_2()
{
    //Check which handler this IRQ belongs to.
    //Fire the required callback depending on which one it is.
    unsigned channel = ADC1.SQR3;

    Adc::serveIsr(Adc::ADCHandlers[channel]);
}

/*
 * ADC Isr Handler
 */
void Adc::serveIsr(AdcIsr_t & handler)
{
    uint16_t sample = ADC1.DR;

    if(handler) {
        handler(sample);
    }
}