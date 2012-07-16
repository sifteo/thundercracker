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

        0,
    };

    // RTC2 external capture on (timestamp all incoming radio packets)
    // Interrupt on, but compare disabled until we begin a nap.
    RTC2CON = 0x09;
    IEN_TICK = 1;

    radio_rx_disable();                 // Receiver starts out disabled
    RF_CKEN = 1;                        // Radio clock running
    radio_transfer_table(table);        // Send initialization commands
    radio_irq_enable();
}

void radio_ack_query() __naked
{
    // Assembly callable only. Packet length in r0.
    // Clobbers r0 and r1. Call on main thread ONLY.

    __asm
        mov     r1, #_radio_query

        clr     _IEN_RF                             ; Begin RF critical section
        clr     _RF_CSN                             ; Begin SPI transaction
        mov     _SPIRDAT, #RF_CMD_W_ACK_PAYLD       ; Start sending ACK packet

4$:     mov     _SPIRDAT, @r1
        inc     r1
        SPI_WAIT
        mov     a, _SPIRDAT                         ; RX dummy byte
        djnz    r0, 4$

        SPI_WAIT                                    ; RX last dummy byte
        mov     a, _SPIRDAT
        setb    _RF_CSN                             ; End SPI transaction
        setb    _IEN_RF                             ; End RF critical section

        ret
    __endasm ;
}
