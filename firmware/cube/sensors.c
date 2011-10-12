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

uint8_t accel_addr = ADDR_SEARCH_START;
uint8_t accel_state;
uint8_t accel_x;

/*
 * Parameters that affect the neighboring protocol. Some of these are
 * baked in to some extent, and not easily changeable with
 * #defines. But they're documented here anyway.
 *
 *  Timer period:    162.76 Hz   Overflow rate of 13-bit Timer 0 at clk/12
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
 *
 * To ensure that we stay within our timeslot even if we can't begin
 * transmitting immediately, we have a deadline, measured in Timer 0
 * ticks (clk/12). The maximum value of this deadline is our timeslot
 * size minus the packet length. But we'll make it a little bit
 * shorter still, so that we preserve our margin for radio latency.
 *
 * Our current packet format is based on a 16-bit frame, so that we can
 * quickly store it using a 16-bit shift register. Currently, for packet
 * validation, we simply define the second 8 bits to be the complement
 * of the first 8 bits. So, we really have only an 8-bit frame. From
 * MSB to LSB, it is defined as:
 *
 *    [7  ] -- Always one. In the first byte, this serves as a start bit,
 *             and the receiver does not store it explicitly. In the second
 *             byte, it just serves as an additional check.
 *
 *    [6:5] -- Mask bits. Transmitted as ones, but by applying different
 *             side masks for each bit, we can discern which side the packet
 *             was received from.
 *
 *    [4:0] -- ID for the transmitting cube.
 */

#define NB_TX_BITS          18      // 1 header, 2 mask, 13 payload, 2 damping
#define NB_RX_BITS          15      // 2 mask, 13 payload

#define NB_BIT_TICKS        12      // 9.0 us, in 0.75 us ticks
#define NB_BIT_TICK_FIRST   13      // Tweak for sampling halfway between pulses
#define NB_DEADLINE         20      // 15 us (Max 48 us)

uint8_t nb_bits_remaining;          // Bit counter for transmit or receive
uint8_t nb_buffer[2];               // Packet shift register for TX/RX
uint8_t nb_tx_packet[2];            // The packet we're broadcasting
__bit nb_tx_mode;                   // We're in the middle of an active transmission
__bit nb_rx_mask_pending;           // We still need to do another RX mask bit

/*
 * We do a little bit of signal conditioning on neighbors before
 * passing them on to the ACK packet. There are really two things we
 * need to do:
 *
 *   1. Detect when a neighbor has disappeared
 *   2. Smooth over momentary glitches
 *
 * For (1), we use the master sensor clock as a sort of watchdog for
 * the neighbors. If our communication were always perfect, we would
 * get one packet per active neighbor per master sensor period. So,
 * we'll start with that as a baseline. That leads into part (2)... if
 * any state change occurs (neighboring or de-neighboring) we need it
 * to stay stable for at least a couple consecutive frames before we
 * report that over the radio.
 */

// Neighbor state for the current Timer 0 period
uint8_t __idata nb_instant_state[4];

// State that we'd like to promote to the ACK packet, if we can verify it.
uint8_t __idata nb_prev_state[4];


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
        push    acc
        push    psw
        mov     psw, #0         ; Register bank 0
        push    ar0
        push    ar1

        ;--------------------------------------------------------------------
        ; Neighbor TX
        ;--------------------------------------------------------------------

        ; Start transmitting the next neighbor packet, assuming we
        ; have not already missed our chance due to interrupt latency.
        ;
        ; XXX: This is mostly only necessary because we currently run at
        ;      the same prio as RFIRQ. Suck. See the priority discussion
        ;      below, there may be a way to work around this by using a
        ;      different time source. It actually isnt that bad at the moment,
        ;      and we end up here infrequently.

        mov    a, TH0
        jnz    1$
        mov    a, TL0
        addc   a, #(0x100 - NB_DEADLINE)
        jc     1$

        ; Okay, there is still time! Start the TX IRQ.
        
        setb    _nb_tx_mode

        mov     _nb_buffer, _nb_tx_packet
        mov     (_nb_buffer + 1), (_nb_tx_packet + 1)
        mov     _nb_bits_remaining, #NB_TX_BITS
        mov     _TL2, #(0x100 - NB_BIT_TICKS)
        setb    _T2CON_T2I0

1$:

        ;--------------------------------------------------------------------
        ; Neighbor Filtering
        ;--------------------------------------------------------------------

        ; Any valid neighbor packets we receive in this period are latched into
        ; nb_instant_state by the RX code. Ideally we would get one packet from
        ; each active neighbor per period. If we see any change from the current
        ; ACK packet contents, make sure the new data stays stable before
        ; propagating it into the ACK packet.
        ;
        ; This serves several purposes- debouncing the inputs, acting as a more
        ; forgiving timeout for detecting de-neighboring, and smoothing over
        ; any instantaneous radio glitches.

        mov     r0, #_nb_instant_state
2$:

        mov     a, @r0          ; Read instant state...
        mov     @r0, #0         ;   .. and clear quickly, in case we get preempted.
        mov     r1, a

        mov     a, r0           ; Now the previous state
        add     a, #(_nb_prev_state - _nb_instant_state)
        mov     r0, a

        mov     a, @r0          ; Read previous state...
        mov     @r0, ar1        ;   .. and overwrite it with its new value

        xrl     a, r1           ; Does it match?
        jz      3$

        ; State does not match. Go back to instant_state and loop over the next side.

        mov     a, r0
        add     a, #(_nb_instant_state + 1 - _nb_prev_state)
        mov     r0, a
        cjne    a, #(_nb_instant_state + 4), 2$
        sjmp    4$

3$:     
        ; State matches. Is it different than the ACK packet?

        mov     a, r0
        clr     c
        add     a, #(_ack_data + RF_ACK_NEIGHBOR)
        subb    a, #_nb_prev_state
        mov     r0, a

        mov     a, @r0          ; Only compare the actual neighbor-related bits
        xrl     a, r1
        anl     a, #(NB_ID_MASK | NB_FLAG_SIDE_ACTIVE)
        jz      5$

        ; Okay! We are actually making a difference here. Update the neighbor
        ; bits in the ACK packet (leaving the other bits alone) and mark
        ; the ack_len flags.

        xrl     a, @r0
        mov     @r0, a
        orl     _ack_len, #RF_ACK_LEN_NEIGHBOR

5$:     ; Loop to the next side

        mov     a, r0
        clr     c
        add     a, #(_nb_instant_state + 1 - RF_ACK_NEIGHBOR)
        subb    a, #_ack_data
        mov     r0, a
        cjne    a, #(_nb_instant_state + 4), 2$

4$:

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

        ;--------------------------------------------------------------------

        pop     ar1
        pop     ar0
        pop     psw
        pop     acc
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

        ; Begin RX mode. Carefully line up our Timer 2 phase!
        ; Phase is determined by:
        ;
        ;   - The NB_BIT_TICK_FIRST constant
        ;
        ;   - Interrupt and timer latency
        ;
        ;   - The length of this ISR
        ;
        ;   - The lenght of the instructions in tf2_isr prior to
        ;     capturing TL1.
        ;

        ; We also begin our masking sequence, so that we can detect
        ; which side we are receiving on. Set up mask bit 0.  This must
        ; happen well before the next bit arrives!

        anl     _MISC_DIR, #~MISC_NB_MASK0

        ; Trigger Timer 2

        mov     _TL2, #(0x100 - NB_BIT_TICK_FIRST)
        setb    _T2CON_T2I0

        ; In the mean time, set up RX state

        mov     _nb_bits_remaining, #NB_RX_BITS
        setb    _nb_rx_mask_pending

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
        push    acc
        push    psw
        
        jb      _nb_tx_mode, nb_tx
 
        ;--------------------------------------------------------------------
        ; Neighbor Bit Receive
        ;--------------------------------------------------------------------

        mov     a, TL1          ; Capture count from Timer 1
        mov     TL1, #0
        add     a, #0xFF        ; Nonzero -> C

        ; Set up the next mask bit, or clear the mask if we are done masking.

        orl     _MISC_DIR, #MISC_NB_OUT
        jnb     _nb_rx_mask_pending, 1$
        anl     _MISC_DIR, #~MISC_NB_MASK1
        clr     _nb_rx_mask_pending
1$:

        ; Shift in this bit, MSB-first, to our 16-bit packet

        mov     a, (_nb_buffer + 1)
        rlc     a
        mov     (_nb_buffer + 1), a
        mov     a, _nb_buffer
        rlc     a
        mov     _nb_buffer, a

        sjmp    nb_bit_done

        ;--------------------------------------------------------------------
        ; Neighbor Bit Transmit
        ;--------------------------------------------------------------------

nb_tx:

        ; We are shifting one bit out of a 16-bit register, then transmitting
        ; a timed pulse if it's a 1. Since we'll be busy-waiting on the pulse
        ; anyway, we organize this code so that we can start the pulse ASAP,
        ; and perform the rest of our shift while we wait.
        ;
        ; The time between driving the tanks high vs. low should be calibrated
        ; according to the resonant frequency of our LC tank. Cycle counts are
        ; included below, for reference. The "LOW" line can go anywhere after
        ; "HIGH" here.
        ;
        ; Currently we're tuning this for 2 us (32 clocks)

        clr     _TCON_TR1                       ; Prevent echo, disable receiver

        mov     a, _nb_buffer                   ; Just grab the MSB and test it
        rlc     a
        jnc     2$

        orl     MISC_PORT, #MISC_NB_OUT
        anl     _MISC_DIR, #~MISC_NB_OUT        ; 4  HIGH
2$:
        mov     a, (_nb_buffer + 1)             ; 2  Now do a proper 16-bit shift.
        clr     c                               ; 1  Make sure to shift in a zero,
        rlc     a                               ; 1    so our suffix bit is a zero too.
        mov     (_nb_buffer + 1), a             ; 3
        mov     a, _nb_buffer                   ; 2
        rlc     a                               ; 1
        mov     _nb_buffer, a                   ; 3

        mov     a, #0x2                         ; 2
        djnz    ACC, .                          ; 12 (3 iters, 4 cycles each)
        nop                                     ; 1

        anl     MISC_PORT, #~MISC_NB_OUT        ; LOW

        ;--------------------------------------------------------------------

nb_bit_done:

        djnz    _nb_bits_remaining, nb_irq_ret  ; More bits left?

        clr     _T2CON_T2I0                     ; Nope. Disable the IRQ
        orl     _MISC_DIR, #MISC_NB_OUT         ; Let the LC tanks float

        jb      _nb_tx_mode, nb_packet_done     ; TX mode? Nothing to store.

        ;--------------------------------------------------------------------
        ; RX Packet Completion
        ;--------------------------------------------------------------------

        mov     a, _nb_buffer
        orl     a, #0xE0                        ; Put the implied bits back in
        cpl     a                               ; Complement
        xrl     a, (_nb_buffer+1)               ; Check byte
        jnz     nb_packet_done                  ;   Invalid, ignore the packet.

        ; We store good packets in nb_instant_state here. The Timer 0 ISR
        ; will do the rest of our filtering before passing on neighbor data
        ; to the radio ACK packet.

        mov     a, _nb_buffer                   ; Get side bits
        swap    a
        rr      a
        anl     a, #3
        add     a, #_nb_instant_state           ; Array pointer

        mov     psw, #0                         ; Default register bank
        push    ar0                             ; Need to use r0 for indirect addressing
        mov     r0, a

        mov     a, _nb_buffer                   ; Store in ACK-compatible format
        anl     a, #NB_ID_MASK
        orl     a, #NB_FLAG_SIDE_ACTIVE
        mov     @r0, a

        pop     ar0

        ;--------------------------------------------------------------------

nb_packet_done:

        mov     TL1, #0xFF                      ; RX mode: Interrupt on the next incoming edge
        mov     TH1, #0xFF
        clr     _nb_tx_mode
        setb    _TCON_TR1

nb_irq_ret:
        clr     _IR_TF2                         ; Must ack IRQ (TF2 is not auto-clear)

        pop     psw
        pop     acc
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
     *       priority level with the radio! We could pick a different
     *       timebase for the master clock, but it's probably easier to
     *       thunk the RF IRQ onto a different IRQ that has a lower
     *       priority (in a different prio group). But let's wait to
     *       see if this is even a problem. So far in practice it doesn't
     *       seem to be.
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

    TCON_TR0 = 1;               // Timer 0 running
    IEN_TF0 = 1;                // Enable Timer 0 interrupt

    /*
     * Timer 1: neighbor pulse counter
     *
     * We use Timer 1 as a pulse counter in the neighbor receiver.
     * When we're idle, waiting for a new neighbor packet, we set the
     * counter to 0xFFFF and get an interrupt when it overflows.
     * Subsequently, we check it on every bit-period to see if we
     * received any pulses during that bit's time window.
     *
     * By default, the timer will be stopped. We start it after
     * exiting transmit mode for the first time. (We only wait
     * for RX packets when we aren't transmitting)
     */     

    TMOD |= 0x50;               // Timer 1 is a 16-bit counter
    IP0 |= 0x08;                // Highest priority for TF1 interrupt
    IP1 |= 0x08;
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

    /*
     * XXX: Build a trivial neighbor packet based on our trivial cube ID.
     */

    nb_tx_packet[0] = 0xE0 | radio_get_cube_id();
    nb_tx_packet[1] = ~nb_tx_packet[0];
}
