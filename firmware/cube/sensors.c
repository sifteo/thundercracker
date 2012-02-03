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

#define NBR_IO      // comment out to turn off neighbor IO

/*
 * We export a global tick counter, which can be used by other modules
 * who need a low-frequency timebase.
 *
 * This is currently used by battery voltage polling, and LCD delays.
 */
volatile uint8_t sensor_tick_counter;
volatile uint8_t sensor_tick_counter_high;

/*
    LIS3DH accelerometer.
*/

#define ACCEL_ADDR              0x30    // 00110010 - SDO is tied LOW
#define ACCEL_ADDR_RX           0x31    // 00110011 - SDO is tied LOW

#define ACCEL_CTRL_REG1         0x20
#define ACCEL_REG1_INIT         0x4F    // 50 Hz, low power, x/y/z axes enabled

#define ACCEL_CTRL_REG4         0x23
#define ACCEL_REG4_INIT         0x80    // block update, 2g full-scale, little endian

#define ACCEL_START_READ_X      0xA8    // (AUTO_INC_BIT | OUT_X_L)

uint8_t accel_state;
uint8_t accel_x_low;
uint8_t accel_x_high;
uint8_t accel_y_low;
uint8_t accel_y_high;
uint8_t accel_z_low;
// last byte of z high is just straight into the RF ack buffer

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

// 2us pulses, 48us bit periods
// TODO: bit period is maxed out to account for measured tank ring down of
// about 48us. Better squelching should hopefully allow us to reduce this significantly.
#define NB_BIT_TICKS        70      // In 0.75 us ticks
#define NB_BIT_TICK_FIRST   72      // Tweak for sampling halfway between pulses
 
#define NB_DEADLINE         70      // 48us (previously 15 us - please tune me back down) (Max 48 us)

#define NBR_SQUELCH_ENABLE          // Enable squelching during neighbor RX

uint8_t nb_bits_remaining;          // Bit counter for transmit or receive
uint8_t nb_buffer[2];               // Packet shift register for TX/RX
uint8_t nb_tx_packet[2];            // The packet we're broadcasting
__bit nb_tx_mode;                   // We're in the middle of an active transmission
__bit nb_rx_mask_state0;
__bit nb_rx_mask_state1;
__bit nb_rx_mask_bit0;
__bit nb_rx_mask_bit1;
__bit touch;

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
 *    These reads get kicked off right after the neighbor ISR, so that we'll
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

        ; 1. TX address finished, Send register address next

as_1:
        mov     _W2DAT, #ACCEL_START_READ_X
        mov     _accel_state, #(as_2 - as_1)
        sjmp    as_ret

        ; 2. Register address finished. Send repeated start, and RX address
as_2:
        orl     _W2CON0, #W2CON0_START
        mov     _W2DAT, #ACCEL_ADDR_RX
        mov     _accel_state, #(as_3 - as_1)
        sjmp    as_ret

        ; 3. RX address finished. Subsequent bytes will be reads.
as_3:
        mov     _accel_state, #(as_4 - as_1)
        sjmp    as_ret

        ; 4. Read X axis low byte.
as_4:
        mov     _accel_x_low, _W2DAT
        mov     _accel_state, #(as_5 - as_1)
        sjmp    as_ret

        ; 5. Read X axis high byte.
as_5:
        mov     _accel_x_high, _W2DAT
        mov     _accel_state, #(as_6 - as_1)
        sjmp    as_ret

        ; 6. Read Y axis low byte.
as_6:
        mov     _accel_y_low, _W2DAT
        mov     _accel_state, #(as_7 - as_1)
        sjmp    as_ret

        ; 7. Read Y axis high byte.
as_7:
        mov     _accel_y_high, _W2DAT
        mov     _accel_state, #(as_8 - as_1)
        sjmp    as_ret

        ; 8. Read Z axis low byte.
        ; This is the second-to-last byte, so we queue a Stop condition.
as_8:
        mov     _accel_z_low, _W2DAT
        orl     _W2CON0, #W2CON0_STOP ; stopping after last read below for now
        mov     _accel_state, #(as_9 - as_1)
        sjmp    as_ret

        ; 9. Read Z axis high byte. In rapid succession, store both axes and set the change flag
        ;    if necessary. This minimizes the chances of ever sending one old axis and
        ;    one new axis. In fact, since this interrupt is higher priority than the
        ;    RF interrupt, we are guaranteed to send synchronized updates of both axes.
as_9:

        mov     a, _W2DAT
        ; eh...just stuffing X and & values in for now. need to decide how to
        ; reformat the RF ACK packet now that we have 3 axes & 16 bit values
        mov     a, _accel_x_high
        ; orl     _W2CON0, #W2CON0_STOP

        xrl     a, (_ack_data + RF_ACK_ACCEL + 0)
        jz      1$
        xrl     (_ack_data + RF_ACK_ACCEL + 0), a
        orl     _ack_len, #RF_ACK_LEN_ACCEL
1$:

        mov     a, _accel_y_high
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
        ; Touch sensing
        ;--------------------------------------------------------------------

		mov a, _MISC_PORT
		anl a, #MISC_TOUCH
		cjne a, #MISC_TOUCH, 6$
		
		jb _touch, 8$	
		setb _touch
		sjmp 7$
6$:
		jnb _touch, 8$
		clr _touch
7$:	

		xrl (_ack_data + RF_ACK_NEIGHBOR + 0), #(NB0_FLAG_TOUCH)
		orl _ack_len, #RF_ACK_LEN_NEIGHBOR
		
8$:
        ;--------------------------------------------------------------------
        ; Tick tock.. this is not latency-critical at all, it goes last.
        
        mov     a, _sensor_tick_counter
        add     a, #1
        mov     _sensor_tick_counter, a
        mov     a, _sensor_tick_counter_high
        addc    a, #0
        mov     _sensor_tick_counter_high, a
        
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

        ; In the mean time, reset RX state

        mov     _nb_bits_remaining, #NB_RX_BITS
        clr     _nb_rx_mask_state0
        clr     _nb_rx_mask_state1

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

        ; Briefly squelch the receive LC tanks, and clear the mask.
        ; Do this concurently with capturing and resetting Timer 1.
        
        #if defined(NBR_SQUELCH_ENABLE) && defined(NBR_IO)
        anl     _MISC_DIR, #~MISC_NB_OUT        ; Squelch all sides
        #endif

        mov     a, TL1                          ; Capture count from Timer 1
        mov     TL1, #0                         ; Reset Timer 1
        add     a, #0xFF                        ; Nonzero -> C
        mov     a, (_nb_buffer + 1)             ; Previous shift reg contents -> A

        #ifdef NBR_IO
        orl     _MISC_DIR, #MISC_NB_OUT         ; Clear squelch and/or masking
        #endif

        jb      _nb_rx_mask_state0, 1$          ; Finished first mask bit?
        setb    _nb_rx_mask_state0
        mov     _nb_rx_mask_bit0, c             ;    Store mask bit
        #ifdef NBR_IO
        anl     _MISC_DIR, #~MISC_NB_MASK1      ;    Prepare mask for next bit
        #endif
        sjmp    10$                             ;    End of masking
1$:

        jb      _nb_rx_mask_state1, 2$          ; Finished second mask bit?
        setb    _nb_rx_mask_state1
        mov     _nb_rx_mask_bit1, c             ;    Store mask bit
        sjmp    10$                             ;    End of masking
2$:

        ; Finished receiving the mask bits. For future bits, we want to set the mask only
        ; to the exact side that we need. This serves as a check for the side bits we
        ; received. This check is important, since there is otherwise no way to valdiate
        ; the received side bits.
        ;
        ; At this point, the side bits have been stored independently in nb_rx_mask_bit*.
        ; We decode them rapidly using a jump tree.

        #ifdef NBR_IO
        jb      _nb_rx_mask_bit0, 3$
         jb     _nb_rx_mask_bit1, 4$
          anl   _MISC_DIR, #~(MISC_NB_OUT ^ MISC_NB_0_TOP)      ; 00
          sjmp  10$
4$:
          anl   _MISC_DIR, #~(MISC_NB_OUT ^ MISC_NB_1_LEFT)     ; 01
          sjmp  10$
3$:
         jb     _nb_rx_mask_bit1, 5$
          anl   _MISC_DIR, #~(MISC_NB_OUT ^ MISC_NB_2_BOTTOM)   ; 10
          sjmp  10$
5$:
          anl   _MISC_DIR, #~(MISC_NB_OUT ^ MISC_NB_3_RIGHT)    ; 11
        #endif

10$:
        ; Done with masking.
        ; Shift in the received bit, MSB-first, to our 16-bit packet
        ; _nb_buffer+1 is in already in A

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
        ; Currently we are tuning this for 2 us (32 clocks)

        clr     _TCON_TR1                       ; Prevent echo, disable receiver

        mov     a, _nb_buffer                   ; Just grab the MSB and test it
        rlc     a
        jnc     2$

#ifdef NBR_IO
        orl     MISC_PORT, #MISC_NB_OUT
        anl     _MISC_DIR, #~MISC_NB_OUT        ; 4  HIGH
#endif
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

#ifdef NBR_IO
        anl     MISC_PORT, #~MISC_NB_OUT        ; LOW
#endif

        ;--------------------------------------------------------------------

nb_bit_done:

        djnz    _nb_bits_remaining, nb_irq_ret  ; More bits left?

        clr     _T2CON_T2I0                     ; Nope. Disable the IRQ
        orl     _MISC_DIR, #MISC_NB_OUT         ; Let the LC tanks float

        jb      _nb_tx_mode, nb_tx_handoff      ; TX mode? Nothing to store.

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
        sjmp    nb_packet_done                  ; Done receiving

        ;--------------------------------------------------------------------
        ; TX -> Accelerometer handoff
        ;--------------------------------------------------------------------

nb_tx_handoff:

        ; Somewhat unintuitively, we start accelerometer sampling right after
        ; the neighbor transmit finishes. In a perfect world, it really wouldnt
        ; matter when we start I2C. But in our hardware, the I2C bus has a
        ; frequency very close to our neighbor resonance. So, this keeps the
        ; signals cleaner.

        ; Trigger the next accelerometer read, and reset the I2C bus. We do
        ; this each time, to avoid a lockup condition that can persist
        ; for multiple packets. We include a brief GPIO frob first, to try
        ; and clear the lockup on the accelerometer end.

        ; orl     MISC_PORT, #MISC_I2C      ; Drive the I2C lines high
        ; anl     _MISC_DIR, #~MISC_I2C     ;   Output drivers enabled
        ; xrl     MISC_PORT, #MISC_I2C_SCL  ;   Now pulse SCL low
        mov     _accel_state, #0          ; Reset accelerometer state machine
        mov     _W2CON0, #0               ; Reset I2C master
        mov     _W2CON0, #1               ;   Turn on I2C controller
        mov     _W2CON0, #7               ;   Master mode, 100 kHz.
        mov     _W2DAT, #ACCEL_ADDR       ; Trigger the next I2C transaction
        
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

#define I2C_TX_BYTE(b) {        \
            IR_SPI = 0;         \
            W2DAT = (b);        \
            while (!IR_SPI);    \
        }

#define I2C_TX_W_ACK(b, fail) {             \
    for (;;) {                              \
        uint8_t status;                     \
        I2C_TX_BYTE(b);                     \
        status = W2CON1;                    \
        if (!(status & W2CON1_READY)) {     \
            continue;                       \
        }                                   \
        if (status & W2CON1_NACK)           \
            goto fail;                      \
        else                                \
            break;                          \
    }                                       \
}

#if 0
// used during testing
static void i2c_rx(uint8_t devaddr, uint8_t regaddr, uint8_t *buf, uint8_t len)
{
    I2C_TX_W_ACK(devaddr, cleanup);
    I2C_TX_W_ACK(regaddr, cleanup);

    W2CON0 |= W2CON0_START;

    I2C_TX_W_ACK(devaddr | 0x1, cleanup);

    // wait for bytes
    while (len) {
        while (!IR_SPI);                // Wait until 2-Wire irq flag is set
        IR_SPI = 0;                     // Clear the interrupt flag
        *buf = W2DAT;        // Read received data
        buf++;
        len--;
    }

cleanup:
    W2CON0 |= W2CON0_STOP;
}
#endif

// TODO - asm these functions
static void i2c_tx(uint8_t addr, const __code uint8_t *buf, uint8_t len)
{
    I2C_TX_W_ACK(addr, cleanup);

    while (len--) {
        I2C_TX_W_ACK(*buf, cleanup);
        buf++;
    }

cleanup:
    W2CON0 |= W2CON0_STOP;
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
     * A/D converter (MISC irq) Priority
     */

    IP1 |= 0x10;
    
    /*
     * I2C / 2Wire, for the accelerometer
     */

    MISC_PORT |= MISC_I2C;      // Drive the I2C lines high
    MISC_DIR &= ~MISC_I2C;      // Output drivers enabled

    W2CON0 = 0;                 // Reset I2C master
    W2CON0 = 1;                 // turn it on
    W2CON0 = 7;                 // Turn on I2C controller, Master mode, 100 kHz.
    INTEXP |= 0x04;             // Enable 2Wire IRQ -> iex3
    W2CON1 = 0;                 // Unmask interrupt, leave everything at default
    T2CON |= 0x40;              // iex3 rising edge
    IRCON = 0;                  // Reset interrupt flag (used below)
    
    {
        // put LIS3D in low power mode with all 3 axes enabled &
        // block data update enabled

        const __code uint8_t init1[] = { ACCEL_CTRL_REG1, ACCEL_REG1_INIT };
        const __code uint8_t init2[] = { ACCEL_CTRL_REG4, ACCEL_REG4_INIT };

        i2c_tx(ACCEL_ADDR, init1, sizeof init1);
        i2c_tx(ACCEL_ADDR, init2, sizeof init2);
    }
    /*
        test loop for reading data, blocking style.
        NOTE - this works *without* having to reset the I2C peripheral...
        investigate this further, so we don't have to reset it at the beginning
        of each sample series in the interrupt handler loop
    */
#if 0
    for (;;) {
        uint16_t c;
        uint8_t samples[2] = { ACCEL_START_READ_X };

        i2c_rx(ACCEL_ADDR, ACCEL_START_READ_X, samples, sizeof(samples));

        for (c = 0; c < 0xFFF; ++c);
    }
#endif

    IRCON = 0;                  // Clear any spurious IRQs from the above initialization
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
     * XXX: Build a trivial outgoing neighbor packet based on our trivial cube ID.
     */

    nb_tx_packet[0] = 0xE0 | radio_get_cube_id();
    nb_tx_packet[1] = ~nb_tx_packet[0];
}
