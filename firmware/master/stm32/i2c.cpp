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

    scl.setControl(GPIOPin::OUT_ALT_OPEN_50MHZ);
    sda.setControl(GPIOPin::OUT_ALT_OPEN_50MHZ);

    const uint32_t PCLK1 = 36000000;
    const uint32_t APB_CLOCK_MHZ = PCLK1 / 1000000;

    // reset the peripheral
    hw->CR1 = (1 << 15);
    hw->CR1 = 0;

    // set frequency & select ISRs we're interested in
    hw->CR2 = (1 << 9)  |       // ITEVTEN: event ISR
              (1 << 8)  |       // ITERREN: error ISR
              (1 << 10) |       // ITBUFEN: buffer ISR
              APB_CLOCK_MHZ;    // frequency in MHz - must be at least 2

    // CCR & TRISE are master mode only

    // set 7-bit address - bit 14 must remain set to 1
    hw->OAR1 = (1 << 14) | ((address & 0x7f) << 1);
    hw->CR1 |= (1 << 0);    // enable peripheral
}

/*
 * Error interrupt handler.
 * Not handling errors at the moment other than for debug.
 */
void I2CSlave::isrER()
{
    uint32_t status = hw->SR1;
    hw->SR1 = 0;

    if (status & (1 << 10)) {   // AF: acknowledge failure

    }

    if (status & (1 << 9)) {    // ARLO: arbitrarion lost

    }

    if (status & (1 << 8)) {    // BERR: bus error

    }

    if (status & (1 << 11)) {   // OVR: Overrun/underrun

    }
}

/*
 * IRQ for i2c events.
 *
 * We expect whoever is calling us here has gotten our status(), and set
 * the contents of 'byte' accordingly.
 *
 * If data has become available, write it to 'byte'.
 * If TX is empty, if 'byte' is null, we're done, otherwise write it to DR.
 */
void I2CSlave::isrEV(uint8_t *byte)
{
    uint16_t status = hw->SR1;
    hw->SR1 = 0;

    // ADDR: Address match for incoming transaction.
    if (status & AddressMatch) {

    }

    // TXE: data register empty, time to transmit next byte
    if (status & TxEmpty) {

        if (byte)
            hw->DR = *byte;
    }

    // RXNE: data register not empty, byte has arrived
    if (status & RxNotEmpty) {
        *byte = hw->DR;
    }

    // STOPF: Slave has detected a STOP condition on the bus
    if (status & StopBit) {

    }
}
