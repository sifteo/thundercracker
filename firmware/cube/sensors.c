/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * Thundercracker cube firmware
 *
 * M. Elizabeth Scott <beth@sifteo.com>
 * Copyright <c> 2011 Sifteo, Inc. All rights reserved.
 */

#include <stdint.h>
#include "sensors.h"
#include "hardware.h"
#include "radio.h"
#include "time.h"

#define ADDR_SEARCH_START       0x20    // Start here...
#define ADDR_SEARCH_INC         0x02    // Add this
#define ADDR_SEARCH_MASK        0x2F    // And mask to this (Keep the 0x20 bit always)

uint8_t accel_addr = ADDR_SEARCH_START;
uint8_t accel_state;
uint8_t accel_x;


/*
 * I2C ISR --
 *
 *    This is where we handle accelerometer reads. Since I2C is a
 *    pretty slow bus, and the hardware has no real buffering to speak
 *    of, we use this IRQ to implement a state machine that executes
 *    our I2C read byte-by-byte.
 *
 *    These reads get kicked off during the Radio ISR, so that we'll
 *    start taking a new accel reading any time the previous reading,
 *    if any, had a chance to be retrieved.
 *
 *    The kick-off (writing the first byte) is handled by some inlined
 *    code in sensors.h, which we include in the radio ISR.
 */

void spi_i2c_isr(void) __interrupt(VECTOR_SPI_I2C) __naked
{
    __asm
        push    acc
        push    psw
        push    dpl
        push    dph
        push    _DPS
        mov     _DPS, #0

        ;--------------------------------------------------------------------
        ; Accelerometer State Machine
        ;--------------------------------------------------------------------

        ; Check status of I2C engine.

        mov     a, _W2CON1
        jnb     acc.0, as_ret           ; Wasnt a real I2C interrupt. Ignore it.
        jb      acc.1, as_nack          ; Was not acknowledged!

        mov     dptr, #as_1
        mov     a, _accel_state
        jmp     @a+dptr

        ; 1. Address/Write finished, Send register address next

as_1:
        mov     _W2DAT, #0              
        mov     _accel_state, #(as_2 - as_1)
        sjmp    as_ret

        ; 2. Register address finished. Send repeated start, and address/read

as_2:
        orl     _W2CON0, #W2CON0_START  
        mov     a, _accel_addr
        orl     a, #1
        mov     _W2DAT, a
        mov     _accel_state, #(as_3 - as_1)
        sjmp    as_ret

        ; 3. Address/read finished. Subsequent bytes will be reads.

as_3:
        mov     _accel_state, #(as_4 - as_1)
        sjmp    as_ret

        ; 4. Read X axis. This is the second-to-last byte, so we queue a Stop condition.

as_4:
        mov     _accel_x, _W2DAT
        orl     _W2CON0, #W2CON0_STOP  
        mov     _accel_state, #(as_5 - as_1)
        sjmp    as_ret

        ; 5. Read Y axis. In rapid succession, store both axes and set the change flag
        ;    if necessary. This minimizes the chances of ever sending one old axis and
        ;    one new axis. In fact, since this interrupt is higher priority than the
        ;    RF interrupt, we're guaranteed to send synchronized updates of both axes.

as_5:

        mov     a, _W2DAT
        xrl     a, (_ack_data + RF_ACK_ACCEL + 0)
        jz      1$
        xrl     (_ack_data + RF_ACK_ACCEL + 0), a
        orl     _ack_len, #RF_ACK_LEN_ACCEL
1$:

        mov     a, _accel_x
        xrl     a, (_ack_data + RF_ACK_ACCEL + 1)
        jz      2$
        xrl     (_ack_data + RF_ACK_ACCEL + 1), a
        orl     _ack_len, #RF_ACK_LEN_ACCEL
2$:

        mov     _accel_state, #0
        sjmp    as_ret

        ; NACK handler. Stop, go back to the default state, try again.
        ;
        ; Also try a different I2C address... some of these accelerometer
        ; chips are shipping with different addresses!

as_nack:
        mov     a, _accel_addr
        add     a, #ADDR_SEARCH_INC
        anl     a, #ADDR_SEARCH_MASK
        mov     _accel_addr, a

        orl     _W2CON0, #W2CON0_STOP  
        mov     _accel_state, #0
        sjmp    as_ret

        ;--------------------------------------------------------------------

as_ret:
        pop     _DPS
        pop     dph
        pop     dpl
        pop     psw
        pop     acc
        reti
    __endasm ;
}


/*
 * Timer 0 ISR --
 *
 *    XXX: Right now, this is a relatively low frequency ISR that we
 *         to kick off new accelerometer reads. In the future, this
 *         should be merged with sensor code for reading neighbor and
 *         touch sensors, so we can minimize interrupt overhead and
 *         timer resource usage.
 */

void tf0_isr(void) __interrupt(VECTOR_TF0) __naked
{
    __asm

        mov     _accel_state, #0
        mov     _W2DAT, _accel_addr

        reti
    __endasm;
}


void sensors_init()
{
    /*
     * I2C, for the accelerometer
     *
     * This MUST be a high priority interrupt. Specifically, it
     * must be higher priority than the RFIRQ. The RFIRQ can take
     * a while to run, and it'll cause an underrun on I2C. Since
     * there's only a one-byte buffer for I2C reads, this is fairly
     * time critical.
     *
     * Currently we're using priority level 2.
     */

    W2CON0 |= 1;                // Enable I2C / 2Wire controller
    W2CON0 = 0x07;              // 100 kHz, Master mode
    W2CON1 = 0x00;              // Unmask interrupt
    INTEXP |= 0x04;             // Enable 2Wire IRQ -> iex3
    T2CON |= 0x40;              // iex3 rising edge
    IRCON = 0;                  // Clear any spurious IRQs from initialization
    IP1 |= 0x04;                // Interrupt priority level 2.
    IEN_SPI_I2C = 1;            // Global enable for SPI interrupts

    /*
     * Timer0, for triggering sensor reads.
     *
     * This is set up as a 13-bit counter, fed by Cclk/12.
     * It overflows at 16000000 / 12 / (1<<13) = 162.76 Hz
     */

    TMOD = 0x00;                // 13-bit counter mode
    TCON |= 0x10;               // Timer 0 running
    IEN_TF0 = 1;                // Enable Timer 0 interrupt
}
