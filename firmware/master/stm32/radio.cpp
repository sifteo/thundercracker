/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * This file is part of the internal implementation of the Sifteo SDK.
 * Confidential, not for redistribution.
 *
 * Copyright <c> 2011 Sifteo, Inc. All rights reserved.
 */

/*
 * High-level radio interface. This is the glue between our
 * cross-platform code and the low-level nRF radio driver.
 */

#include "radio.h"
#include "nrf24l01.h"

static NRF24L01 NordicRadio(GPIOPin(&GPIOB, 10),                // CE
                            GPIOPin(&GPIOB, 11),                // IRQ
                            SPIMaster(&SPI2,                    // SPI:
                                      GPIOPin(&GPIOB, 12),      //   CSN
                                      GPIOPin(&GPIOB, 13),      //   SCK
                                      GPIOPin(&GPIOB, 14),      //   MISO
                                      GPIOPin(&GPIOB, 15)));    //   MOSI

void ISR_NordicRadio()
{
    NordicRadio.isr();
}

void Radio::open()
{
    NordicRadio.init();
    NordicRadio.ptxMode();
}

void Radio::halt()
{
    /*
     * Wait for any interrupt
     *
     * XXX: Disabled for now, this makes JTAG debugging
     *      very annoying, since the JTAG clock is also
     *      turned off during wfi.
     */
#if 0
    __asm__ __volatile__ ("wfi");
#endif
}
