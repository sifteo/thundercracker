/*
 * Thundercracker Firmware -- Confidential, not for redistribution.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#include "i2c.h"

void I2CSlave::init(GPIOPin scl, GPIOPin sda, uint8_t address)
{
    // enable i2c clock & ISRs
    if (hw == &I2C1) {
        RCC.APB1ENR |= (1 << 21);
    }
    else if (hw == &I2C2) {
        RCC.APB1ENR |= (1 << 22);
    }

    scl.setHigh();
    sda.setHigh();
    scl.setControl(GPIOPin::OUT_ALT_OPEN_50MHZ);
    sda.setControl(GPIOPin::OUT_ALT_OPEN_50MHZ);

    // NOTE - this must match the PPRE1 config in setup.cpp
    const uint32_t PCLK1 = 18000000;
    const uint32_t APB_CLOCK_MHZ = PCLK1 / 1000000;

    // reset the peripheral
    hw->CR1 = (1 << 15);
    hw->CR1 = 0;

    // CCR & TRISE are master mode only

    // set frequency & select ISRs we're interested in
    hw->CR2 = (1 << 9)  |       // ITEVTEN: event ISR
              (1 << 8)  |       // ITERREN: error ISR
              (1 << 10) |       // ITBUFEN: buffer ISR
              APB_CLOCK_MHZ;    // frequency in MHz - must be at least 2

    // set 7-bit address - bit 14 must remain set to 1
    hw->OAR1 = (1 << 14) | ((address & 0x7f) << 1);
    hw->CR1 = (1 << 10) |   // enable hardware ACK support
              (1 << 0);     // go!
}

/*
 * Error interrupt handler.
 * Not handling errors at the moment other than for debug.
 */
void I2CSlave::isrER()
{
    uint32_t status = hw->SR1;

    if (status & Nack) {            // AF: acknowledge failure
        hw->SR1 &= ~Nack;
    }

    if (status & (1 << 9)) {        // ARLO: arbitrarion lost
        hw->SR1 &= ~(1 << 9);
    }

    if (status & (1 << 8)) {        // BERR: bus error
        hw->SR1 &= ~(1 << 8);
    }

    if (status & OverUnderRun) {    // OVR: Overrun/underrun
        hw->SR1 &= ~OverUnderRun;
    }
}
