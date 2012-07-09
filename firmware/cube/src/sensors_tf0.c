/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * Thundercracker cube firmware
 *
 * M. Elizabeth Scott <beth@sifteo.com>
 * Copyright <c> 2011 Sifteo, Inc. All rights reserved.
 */

#include <stdint.h>
#include <protocol.h>
#include <cube_hardware.h>

#include "radio.h"
#include "sensors.h"
#include "sensors_nb.h"

uint8_t __idata nb_instant_state[4];
uint8_t __idata nb_prev_state[4];


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
        push    0
        push    1

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

        ; Start transmitting.
        ;
        ; The first byte is already computed in nb_tx_id.
        ; The second is complemented and shifted left by 3.

        mov     a, _nb_tx_id
        mov     _nb_buffer, a
        cpl     a
        swap    a
        rr      a
        anl     a, #0xF8
        mov     _nb_buffer+1, a

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
        mov     @r0, 1          ;   .. and overwrite it with its new value

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
        ; the ack_bits.

        xrl     a, @r0
        mov     @r0, a
        orl     _ack_bits, #RF_ACK_BIT_NEIGHBOR

#ifdef DEBUG_NBR
        mov		_nbr_temp, a
        mov 	a, r0
        clr 	c
        add		a, #(_nbr_data - RF_ACK_NEIGHBOR)
        subb	a, #_ack_data
        mov 	r1, a
        mov		a, _nbr_temp
        mov		@r1, a
#endif

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

        mov     a, _MISC_PORT
        anl     a, #MISC_TOUCH
        cjne    a, #MISC_TOUCH, 6$
        
        jb      _touch, 8$
#ifdef TOUCH_DEBOUNCE
        mov     _touch_off, #(TOUCH_DEBOUNCE_OFF)
        djnz    _touch_on, 8$
#endif
#ifdef DEBUG_TOUCH
        mov     a, _touch_count
        inc     a
        mov     _touch_count, a
#endif
        setb    _touch
        sjmp    7$
6$:
        jnb     _touch, 8$
#ifdef TOUCH_DEBOUNCE
        mov     _touch_on, #(TOUCH_DEBOUNCE_ON)
        djnz    _touch_off, 8$
#endif
        clr     _touch
7$: 

        xrl     (_ack_data + RF_ACK_NEIGHBOR + 0), #(NB0_FLAG_TOUCH)
        orl     _ack_bits, #RF_ACK_BIT_NEIGHBOR
        
8$:
        ;--------------------------------------------------------------------
        ; Tick tock.. this is not latency-critical at all, it goes last.
        
        mov     a, _sensor_tick_counter
        add     a, #1
        mov     _sensor_tick_counter, a
        mov     a, _sensor_tick_counter_high
        addc    a, #0
        mov     _sensor_tick_counter_high, a
        
        pop     1
        pop     0
        pop     psw
        pop     acc
        reti
    __endasm;
}
