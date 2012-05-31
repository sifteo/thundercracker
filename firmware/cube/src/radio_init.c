/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * Thundercracker cube firmware
 *
 * M. Elizabeth Scott <beth@sifteo.com>
 * Copyright <c> 2011 Sifteo, Inc. All rights reserved.
 */

#include <protocol.h>
#include <cube_hardware.h>
#include "radio.h"


static void radio_transfer_table(const __code uint8_t *ptr)
{
    /*
     * Execute a table of SPI transfers (Zero-terminated,
     * length-prefixed). Intended for initialization only, i.e. this
     * isn't intended to be efficient.
     */

    uint8_t len;

    while ((len = *ptr)) {
        ptr++;

        RF_CSN = 0;

        do {
            SPIRDAT = *ptr;
            ptr++;
            while (!(SPIRSTAT & SPI_RX_READY));
            SPIRDAT;
        } while (--len);

        RF_CSN = 1;
    }
}

void radio_init(void)
{
    static const __code uint8_t table[] = {

        /* Enable nRF24L01 features */
        2, RF_CMD_W_REGISTER | RF_REG_FEATURE,        0x07,
        
        /* Enable receive pipe 0 */
        2, RF_CMD_W_REGISTER | RF_REG_DYNPD,          0x01,
        2, RF_CMD_W_REGISTER | RF_REG_EN_RXADDR,      0x01,
        2, RF_CMD_W_REGISTER | RF_REG_EN_AA,          0x01,

        /* Max payload size */
        2, RF_CMD_W_REGISTER | RF_REG_RX_PW_P0,       32,
        
        /* Discard any packets queued in hardware */
        1, RF_CMD_FLUSH_RX,
        1, RF_CMD_FLUSH_TX,
                        
        /* 5-byte address width */
        2, RF_CMD_W_REGISTER | RF_REG_SETUP_AW,       0x03,

        /* 2 Mbit, max transmit power */
        2, RF_CMD_W_REGISTER | RF_REG_RF_SETUP,       0x0e,
        
        /* Clear write-once-to-clear bits */
        2, RF_CMD_W_REGISTER | RF_REG_STATUS,         0x70,

        /* 16-bit CRC, radio enabled, PRX mode, RX_DR IRQ enabled */
        2, RF_CMD_W_REGISTER | RF_REG_CONFIG,         0x3f,

        /*
         * XXX: Hardcoded cube addresses, for testing only
         */

#ifdef CUBE_CHAN
        2, RF_CMD_W_REGISTER | RF_REG_RF_CH,          CUBE_CHAN,
#else
        2, RF_CMD_W_REGISTER | RF_REG_RF_CH,          0x02,
#endif

#ifdef CUBE_ADDR
        6, RF_CMD_W_REGISTER | RF_REG_TX_ADDR,        CUBE_ADDR, 0xe7, 0xe7, 0xe7, 0xe7,
        6, RF_CMD_W_REGISTER | RF_REG_RX_ADDR_P0,     CUBE_ADDR, 0xe7, 0xe7, 0xe7, 0xe7,
#endif

        0,
    };

    radio_rx_disable();                 // Receiver starts out disabled
    RF_CKEN = 1;                        // Radio clock running
    radio_transfer_table(table);        // Send initialization commands
    radio_irq_enable();
}


uint8_t radio_get_cube_id(void)
{
    /*
     * XXX: This is temporary, until we have a real pairing mechanism.
     *      Our cube will be identified by radio address and channel, but
     *      also by an ID between 0 and 31. This will eventually come from
     *      the master cube, but for now we take it from the LSB of the
     *      radio address. That can be set at compile time with CUBE_ADDR,
     *      or it could be provided as a hardware default by the simulator.
     */

    uint8_t id;

    radio_irq_disable();
    RF_CSN = 0;

    SPIRDAT = RF_CMD_R_REGISTER | RF_REG_TX_ADDR;
    SPIRDAT = 0;
    while (!(SPIRSTAT & SPI_RX_READY));
    SPIRDAT;
    while (!(SPIRSTAT & SPI_RX_READY));
    id = SPIRDAT;

    RF_CSN = 1;
    radio_irq_enable();

    return id;
}
