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

/*
 * These MEMSIC accelerometers are flaky little buggers! We're seeing
 * different chips ship with different addresses, so we have to do a
 * quick search if we don't get a response. Additionally, we're seeing
 * them deadlock the I2C bus occasionally. So, we try to work around
 * that by unjamming the bus manually before every poll.
 */

#define ADDR_SEARCH_START       0x20    // Start here...
#define ADDR_SEARCH_INC         0x02    // Add this
#define ADDR_SEARCH_MASK        0x2F    // And mask to this (Keep the 0x20 bit always)

/*
 * Parameters that affect the neighboring protocol. Some of these are
 * baked in to some extent, and not easily changeable with
 * #defines. But they're documented here anyway.
 *
 *  Timer period:    162.76 Hz   Overflow rate of 13-bit Timer 0 at clk/12)
 *  Packet length:   16 bits     Long enough for one data byte and one check byte
 *  # of timeslots:  32          Maximum number of supported cubes in master
 *  Timeslot size:   192 us      Period is split evenly into 32 slots
 *
 * These parameters on their own would give us a maximum bit period of
 * 12 us.  But that assumes that we can perfectly synchronize all
 * cubes. The more margin we can give ourselves for timer drift and
 * communication latency, the better. Our electronics seem to be able
 * to handle pulses down to ~4 us, but it's also better not to push these
 * limits.
 *
 * The other constraint on bit rate is that we need it to divide evenly
 * into our clk/12 timebase (0.75 us).
 */

#define NB_BIT_COUNT    16
#define NB_BIT_TICKS    12    // 9.0 us, in 0.75 us ticks


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
        jnb     acc.0, as_ret           ; Not a real I2C interrupt. Ignore it.
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
        ;    RF interrupt, we are guaranteed to send synchronized updates of both axes.

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
 *    This is a relatively low-frequency ISR that we use to kick off
 *    reading most of our sensors. Currently it handles neighbor TX,
 *    and it initiates I2C reads.
 *
 *    This timer runs at 162.76 Hz. It's a convenient frequency for us
 *    to get from the 16 MHz oscillator, but it also works out nicely
 *    for other reasons:
 * 
 *      1. It's similar to our accelerometer's analog bandwidth
 *      2. It works out well as a neighbor transmit period (32x 192us slots)
 *      3. It's good for human-timescale interactivity
 *
 *    Because neighbor transmits must be interleaved, we allow the
 *    master to adjust the phase of Timer 0. This does not affect the
 *    period. To keep the latencies predictable, we perform our
 *    neighbor transmit first-thing in this ISR.
 */

void tf0_isr(void) __interrupt(VECTOR_TF0) __naked
{
    __asm

        ;--------------------------------------------------------------------
        ; Neighbor TX
        ;--------------------------------------------------------------------

        ; XXX fake

        orl     MISC_PORT, #MISC_NB_OUT
        anl     _MISC_DIR, #~MISC_NB_OUT
        nop
        nop
        nop
        nop
        orl     _MISC_DIR, #MISC_NB_OUT

        ;--------------------------------------------------------------------
        ; Accelerometer Sampling
        ;--------------------------------------------------------------------

        ; Trigger the next accelerometer read, and reset the I2C bus. We do
        ; this each time, to avoid a lockup condition that can persist
        ; for multiple packets. We include a brief GPIO frob first, to try
        ; and clear the lockup on the accelerometer end.

        orl     MISC_PORT, #MISC_I2C      ; Drive the I2C lines high
        anl     _MISC_DIR, #~MISC_I2C     ;   Output drivers enabled
        xrl     MISC_PORT, #MISC_I2C_SCL  ;   Now pulse SCL low
        mov     _accel_state, #0          ; Reset accelerometer state machine
        mov     _W2CON0, #0               ; Reset I2C master
        mov     _W2CON0, #1               ;   Turn on I2C controller
        mov     _W2CON0, #7               ;   Master mode, 100 kHz.
        mov     _W2CON1, #0               ;   Unmask interrupt
        mov     _W2DAT, _accel_addr       ; Trigger the next I2C transaction

        reti
    __endasm;
}


/*
 * Timer 1 ISR --
 *
 *    We use Timer 1 in counter mode, to count incoming neighbor pulses.
 *    We use it both to detect the start pulse, and to detect subsequent
 *    pulses in our serial stream.
 *
 *    We only use the Timer 1 ISR to detect the start pulse. We keep the
 *    counter loaded with 0xFFFF when the receiver is idle. At other times,
 *    the counter value is small and won't overflow.
 *
 *    This interrupt is at the highest priority level! It's very important
 *    that we detect the start pulse with predictable latency, so we can
 *    time the subsequent pulses accurately.
 */

void tf1_isr(void) __interrupt(VECTOR_TF1) __naked
{
    __asm
        reti
    __endasm;
}


/*
 * Timer 2 ISR --
 *
 *    We use Timer 2 to measure out individual bit-periods, during
 *    active transmit/receive of a neighbor packet.
 */

void tf2_isr(void) __interrupt(VECTOR_TF2) __naked
{
    __asm
 
        clr     _IR_TF2
        reti
    __endasm;
}


void sensors_init()
{
    /*
     * This is a maze of IRQs, all with very specific priority
     * assignments.  Here's a quick summary:
     *
     *  - Prio 0 (lowest)
     *
     *    Radio
     *
     *       This is a very long-running ISR. It really needs to be
     *       just above the main thread in priority, but no higher.
     *
     *  - Prio 1 
     *
     *    Accelerometer (I2C)
     *
     *       I2C is somewhat time-sensitive, since we do need to
     *       service it within a byte-period. But this is a fairly
     *       long-timescale operation compared to our other IRQs, and
     *       there's no requirement for predictable latency.
     *
     *  - Prio 2
     *
     *    Master clock (Timer 0)
     *
     *       We need to try our best to service Timer 0 with
     *       predictable latency, within several microseconds, so that
     *       we can reliably begin our neighbor transmit on-time. This
     *       is what keeps us inside the timeslot that we were
     *       assigned.
     *
     *       XXX: This isn't possible with Timer 0, since it shares a
     *       priority level with the radio!
     *
     *  - Prio 3 (highest)
     *
     *    Neighbor pulse counter (Timer 1)
     *
     *       Absolutely must have low and predictable latency. We use
     *       this timer to line up our bit clock with the received
     *       bit, an operation that needs to be accurate down to just
     *       a couple dozen clock cycles.
     *
     *    Neighbor bit TX/RX (Timer 2)
     *
     *       This is the timer we use to actually sample/generate bits.
     *       It's our baud clock. Timing is critical for the same reasons
     *       as above. (Luckily we should never have these two high-prio
     *       interrupts competing. They're mutually exclusive)
     */

    /*
     * I2C / 2Wire, for the accelerometer
     */

    INTEXP |= 0x04;             // Enable 2Wire IRQ -> iex3
    T2CON |= 0x40;              // iex3 rising edge
    IRCON = 0;                  // Clear any spurious IRQs from initialization
    IP0 |= 0x04;                // Interrupt priority level 1.
    IEN_SPI_I2C = 1;            // Global enable for SPI interrupts

    /*
     * Timer 0: master sensor period
     *
     * This is set up as a 13-bit counter, fed by Cclk/12.
     * It overflows at 16000000 / 12 / (1<<13) = 162.76 Hz
     */

    TCON |= 0x10;               // Timer 0 running
    IEN_TF0 = 1;                // Enable Timer 0 interrupt

    /*
     * Timer 1: neighbor pulse counter
     *
     * We use Timer 1 as a pulse counter in the neighbor receiver.
     * When we're idle, waiting for a new neighbor packet, we set the
     * counter to 0xFFFF and get an interrupt when it overflows.
     * Subsequently, we check it on every bit-period to see if we
     * received any pulses during that bit's time window.
     */     

    TMOD |= 0x50;               // Timer 1 is a 16-bit counter
    TL1 = 0xff;                 // Overflow on the next pulse
    TH1 = 0xff;
    IP0 |= 0x08;                // Highest priority for TF1 interrupt
    IP1 |= 0x08;
    TCON |= 0x40;               // Timer 1 running
    IEN_TF1 = 1;                // Enable TF1 interrupt

    /*
     * Timer 2: neighbor bit period timer
     *
     * This is a very fast timer that we use during active reception
     * or transmission of a neighbor packet. It accurately times out
     * bit periods that we use for generating pulses or for sampling
     * Timer 1.
     *
     * We need this to be on Timer 2, since at such a short period we
     * really get a lot of benefit from having automatic reload.
     *
     * This timer begins stopped. We start it on-demand when we need
     * to do a transmit or receive.
     */     

    CRCH = 0xff;                // Constant reload value
    CRCL = 0x100 - NB_BIT_TICKS;
    T2CON |= 0x10;              // Reload enabled
    TH2 = 0xff;                 // TL2 is set as needed, but TH2 is constant
    IP0 |= 0x20;                // Highest priority for TF2 interrupt
    IP1 |= 0x20;
    IEN_TF2_EXF2 = 1;           // Enable TF2 interrupt
}
