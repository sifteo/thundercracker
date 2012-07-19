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
#include "params.h"


static void radio_tx_sync() __naked
{
    /*
     * Transmit a single byte over SPI, from 'A', and wait for the result.
     * This is fully synchronous, with no pipelining! It should only be used
     * by initialization code, where space is more important than speed.
     */

    __asm
        mov     _SPIRDAT, a
        SPI_WAIT
        mov     a, _SPIRDAT
        ret
    __endasm ;
}

static void radio_transfer_table(const __code uint8_t *ptr)
{
    /*
     * Execute a table of SPI transfers (Zero-terminated,
     * length-prefixed). Intended for initialization only, i.e. this
     * isn't intended to be efficient.
     */

    ptr = ptr;
    __asm

3$:     clr     a               ; Read length prefix
        movc    a, @a+dptr
        jz      1$              ; Zero terminator
        inc     dptr
        mov     r0, a

        clr     _RF_CSN         ; Begin SPI transfer

2$:     clr     a               ; Transfer one byte from buffer
        movc    a, @a+dptr
        inc     dptr
        acall   _radio_tx_sync

        djnz    r0, 2$          ; Loop until done
        setb    _RF_CSN
        sjmp    3$
1$:
    __endasm ;
}

void radio_init(void)
{
    /*
     * Initialize the radio, but don't yet turn it on.
     *
     * Before using the radio, it still needs a channel and address
     * assigned, then you must call radio_rx_enable(). This is normally
     * done for the first time when disconnected_init() invokes
     * radio_set_idle_addr(), or when we do the same when setting
     * up wake-on-RF.
     *
     * This is the very first initialization step we run, before any
     * peripherals are powered on. This needs to stay short, since
     * it runs even while we're asleep and polling for wake-on-RF.
     *
     * Runs before clearing RAM!
     */

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

    radio_rx_disable();                 // Receiver starts out disabled
    RF_CSN = 1;                         // Set proper idle SPI chip-select state
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

void radio_set_idle_addr(void)
{
    /*
     * Program the radio with a channel and address derived from our HWID,
     * for use when we're idle. If the radio_idle_hop bit is set, we use
     * the alternate channel rather than the default one.
     *
     * This function disables the radio receiver if it's enabled, and enables
     * it again before leaving.
     *
     * Algorithm:
     *
     *   1. Start by CRC'ing the first three bytes of the HWID
     *   2. Next, for each of the remaining five bytes:
     *      a. CRC this byte of the HWID
     *      b. If the result is one of the disallowed values (00, FF, AA, 55)
     *         repeatedly CRC a zero byte until the value is no longer disallowed.
     *         With a proper generator polynomial this loop is guaranteed to
     *         terminate.
     *      c. Store the current 8-bit CRC state as an address byte
     *   3. CRC a single zero byte
     *   4. If the low 7 bits of the result are greater than 125, repeatedly
     *      CRC additional zero bytes until it is <= 125.
     *   5. Take the low 7 bits of the current CRC state as our channel number.
     *
     * If idle_hop is set:
     *
     *   1. Add 62 to the address
     *   2. If it's greater than 125, subtract 126
     */

    radio_rx_disable();

    __asm

        mov     _CCPDATIA, #0xFF    ; Initial condition for CRC
        mov     _CCPDATIB, #0x84    ; Generator polynomial (See tools/gfm.py)
        mov     dptr, #PARAMS_HWID  ; Point to stored 64-bit HWID

        ; Step (1), feed three bytes through CRC

        mov     r0, #3
1$:     movx    a, @dptr            ; CRC next byte of HWID
        inc     dptr
        xrl     a, _CCPDATO
        mov     _CCPDATIA, a
        djnz    r0, 1$

        ; Begin write to RX address

        clr     _RF_CSN
        mov     a, #(RF_CMD_W_REGISTER | RF_REG_RX_ADDR_P0)
        acall   _radio_tx_sync

        ; Step (2), use remaining 5 bytes to generate RX address

        mov     r0, #5
2$:     movx    a, @dptr            ; CRC next byte of HWID
        inc     dptr
        sjmp    4$                  ; (skip zero)
3$:     mov     a, #0xFF            ; Retry, CRC a 0xFF byte
4$:     xrl     a, _CCPDATO
        mov     _CCPDATIA, a

        jz      3$                  ; Disallowed value 0x00
        cpl     a
        jz      3$                  ; Disallowed value 0xFF
        xrl     a, #0x55
        jz      3$                  ; Disallowed value 0xAA
        cpl     a
        jz      3$                  ; Disallowed value 0x55
        xrl     a, #0x55            ; Its okay! Get the original byte back.

        acall   _radio_tx_sync
        djnz    r0, 2$              ; Next byte
        setb    _RF_CSN             ; End SPI transfer

        ; Begin write to channel

        clr     _RF_CSN
        mov     a, #(RF_CMD_W_REGISTER | RF_REG_RF_CH)
        acall   _radio_tx_sync

        ; Step (3), prepare channel byte

5$:     mov     a, _CCPDATO
        xrl     a, #0xFF
        mov     _CCPDATIA, a
        anl     a, #0x7F            ; Only examine low 7 bits
        mov     r0, a               ; Save channel
        add     a, #(256 - 126)     ; Is it 126 or greater?
        jc      5$                  ;    (4) Try again

        ; Handle idle_hop, rotate our channel by 62.

        jnb     _radio_idle_hop, 6$

        mov     a, r0               ; Add 62 to the channel
        add     a, #62
        mov     r0, a               ; Assume this channel is good
        add     a, #(256 - 126)     ;   Did it overflow?
        jnc     6$                  ; No, it was good. Keep using that.
        mov     r0, a               ; Yes, it overflowed. Use the version we subtracted 126 from

6$:     mov     a, r0
        acall   _radio_tx_sync      ; Write channel
        setb    _RF_CSN             ; End SPI transfer

    __endasm ;

    radio_rx_enable();
}

void radio_set_pairing_addr()
{
    /*
     * Program the radio with a channel and address appropriate for pairing.
     * The neighbor ID (24-31) should already be loaded into CCPDATIA.
     *
     * This function disables the radio receiver if it's enabled, and enables
     * it again before leaving.
     */

    // Address
    static const __code uint8_t table[] = {
        6, RF_CMD_W_REGISTER | RF_REG_RX_ADDR_P0, 0xec, 0x4f, 0xa9, 0x52, 0x18,
        0,
    };

    radio_rx_disable();
    radio_transfer_table(table);

    // Set channel
    __asm
        clr     _RF_CSN
        mov     a, #(RF_CMD_W_REGISTER | RF_REG_RF_CH)
        mov     _CCPDATIB, #0x1C
        acall   _radio_tx_sync
        mov     a, _CCPDATO
        acall   _radio_tx_sync
        setb    _RF_CSN
    __endasm ;

    radio_rx_enable();
}

void radio_fifo_status() __naked
{
    /*
     * This is an assembly-callable routine used to check whether any pending RX packets
     * are buffered in the nRF's FIFO. Intended only for use when the RF ISR is
     * disabled.
     *
     * Notes: We can't use RPD for this purpose, because its power threshold is
     * much higher than the normal RX sensitivity. And we can't use the RX_P_NO
     * field in STATUS for this, because that field is only updated *after* a packet
     * is dequeued.
     *
     * Instead, the most reliable way to determine this state is to read the
     * FIFO_STATUS register, and directly examine the RX_EMPTY bit.
     *
     * Returns the FIFO_STATUS register. Bit 0 is the RX_EMPTY flag.
     */
     
    __asm
        clr     _RF_CSN

        ; Put both TX bytes in the FIFO
        mov     _SPIRDAT, #(RF_CMD_R_REGISTER | RF_REG_FIFO_STATUS)
        mov     _SPIRDAT, #0

        SPI_WAIT                    ; Dequeue and ignore STATUS
        mov     a, _SPIRDAT

        SPI_WAIT                    ; Dequeue and keep FIFO_STATUS
        mov     a, _SPIRDAT
        
        setb    _RF_CSN
        ret
    __endasm ;
}