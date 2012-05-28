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

        ; Trigger Timer 2

        mov     _TL2, #(0x100 - NB_BIT_TICK_FIRST)
        setb    _T2CON_T2I0

        ; In the mean time, reset RX state

        mov     _nb_bits_remaining, #NB_RX_BITS
        clr     _nb_rx_mask_state0
        clr     _nb_rx_mask_state1
        clr     _nb_rx_mask_state2

        reti
    __endasm;
}
