/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * This file is part of the internal implementation of the Sifteo SDK.
 * Confidential, not for redistribution.
 *
 * Copyright <c> 2011 Sifteo, Inc. All rights reserved.
 */

/*
 * Driver for the Nordic nRF24L01 radio
 */

#include "nrf24l01.h"


void NRF24L01::init() {
    /*
     * Common hardware initialization, regardless of radio usage mode.
     */

    spi.init();

    ce.setLow();
    ce.setControl(GPIOPin::OUT_10MHZ);

    irq.setControl(GPIOPin::IN_FLOAT);
    irq.irqInit();
    irq.irqSetFallingEdge();

    static const uint8_t radio_setup[]  = {
        /* Enable nRF24L01 features */
        2, CMD_W_REGISTER | REG_FEATURE,        0x07,
        
        /* Enable receive pipe 0, to support auto-ack */
        2, CMD_W_REGISTER | REG_DYNPD,          0x01,
        2, CMD_W_REGISTER | REG_EN_RXADDR,      0x01,
        2, CMD_W_REGISTER | REG_EN_AA,          0x01,

        /* Max ACK payload size */
        2, CMD_W_REGISTER | REG_RX_PW_P0,       32,
        
        /* Discard any packets queued in hardware */
        1, CMD_FLUSH_RX,
        1, CMD_FLUSH_TX,
                        
        /* Auto retry delay, 500us, 15 retransmits */
        2, CMD_W_REGISTER | REG_SETUP_RETR,     0x1f,

        /* 5-byte address width */
        2, CMD_W_REGISTER | REG_SETUP_AW,       0x03,

        /* 2 Mbit, max transmit power */
        2, CMD_W_REGISTER | REG_RF_SETUP,       0x0e,
        
        /* Clear write-once-to-clear bits */
        2, CMD_W_REGISTER | REG_STATUS,         0x70,

        0
    };
    spi.transferTable(radio_setup);

    // Everything is ready. Unleash our IRQ handler!
    irq.irqEnable();
}

void NRF24L01::ptxMode()
{
    /*
     * Setup for Primary Transmitter (PTX) mode
     */

    static const uint8_t ptx_setup[]  = {
        /* 16-bit CRC, radio enabled, IRQs enabled */
        2, CMD_W_REGISTER | REG_CONFIG,         0x0e,

        /* XXX: Send packet */
        2, CMD_W_TX_PAYLOAD, 0x55,

        0
    };
    spi.transferTable(ptx_setup); 

    ce.setHigh();
    while (1);

}

void NRF24L01::isr()
{
    // Acknowledge the IRQ controller
    irq.irqAcknowledge();

    /*
     * Read the NRF STATUS register, then write to clear.
     * This tells us which IRQ(s) occurred.
     */
    spi.begin();
    uint8_t status = spi.transfer(CMD_W_REGISTER | REG_STATUS);
    spi.transfer(status);
    spi.end();

    /* XXX: Send packet */

    static const uint8_t blah[]  = {
        2, CMD_W_TX_PAYLOAD, 0x55,
        0
    };
    spi.transferTable(blah);
}
