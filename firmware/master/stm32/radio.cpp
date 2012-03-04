/*
 * Thundercracker Firmware -- Confidential, not for redistribution.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

/*
 * High-level radio interface. This is the glue between our
 * cross-platform code and the low-level nRF radio driver.
 */

#include "radio.h"
#include "nrf24l01.h"
#include "debug.h"
#include "board.h"

static NRF24L01 NordicRadio(RF_CE_GPIO,
                            RF_IRQ_GPIO,
                            SPIMaster(&RF_SPI,              // SPI:
                                      RF_SPI_CSN_GPIO,      //   CSN
                                      RF_SPI_SCK_GPIO,      //   SCK
                                      RF_SPI_MISO_GPIO,     //   MISO
                                      RF_SPI_MOSI_GPIO));   //   MOSI

#if BOARD == BOARD_TC_MASTER_REV1
IRQ_HANDLER ISR_EXTI9_5()
{
    NordicRadio.isr();
}
#else
IRQ_HANDLER ISR_EXTI15_10()
{
    NordicRadio.isr();
}
#endif

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
     * Disabled during debug builds. WFI loops make JTAG debugging
     * very annoying, since the JTAG clock is also turned off while
     * we're waiting.
     */

#ifndef DEBUG
    __asm__ __volatile__ ("wfi");
#endif
}
