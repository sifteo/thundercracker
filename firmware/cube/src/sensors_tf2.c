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
#include "sensors_i2c.h"

/*
 * Timer 2 ISR --
 *
 *    We use Timer 2 to measure out individual bit-periods, during
 *    active transmit/receive of a neighbor packet.
 */

void tf2_isr(void) __interrupt(VECTOR_TF2) __naked
{
    __asm
        ; Squelch immediately. Doesnt matter if it is Tx

        #ifdef NBR_SQUELCH_ENABLE
        ; no squelch for data bits (since side-mask already set)
        jb		_nb_rx_mask_state2, no_squelch
squelch:
        anl     _MISC_DIR, #~MISC_NB_OUT        ; Squelch all sides
no_squelch:
        #endif

        push    acc
        push    psw
        
        jb      _nb_tx_mode, nb_tx
 
        ;--------------------------------------------------------------------
        ; Neighbor Bit Receive
        ;--------------------------------------------------------------------

        ; Capture and reset Timer1 here.

        mov     a, TL1                          ; Capture count from Timer 1
        add     a, #0xFF                        ; Nonzero -> C
        mov     a, (_nb_buffer + 1)             ; Previous shift reg contents -> A
        mov     TL1, #0                         ; Reset Timer 1.

        ; Any transition from this point on, will be accounted towards the next bit

h_bit:

        jb		_nb_rx_mask_state0, s0_bit		; First bit?
		#ifdef NBR_RX
        mov     _MISC_DIR, #(MISC_DIR_VALUE & ~MISC_NB_MASK0)
		#endif
        setb    _nb_rx_mask_state0
        sjmp    read_buf                    	; End of masking

s0_bit:

        jb		_nb_rx_mask_state1, s1_bit		; s0 bit?
		#ifdef NBR_RX
        mov		_MISC_DIR, #(MISC_DIR_VALUE & ~MISC_NB_MASK1)
		#endif
        mov     _nb_rx_mask_bit0, c             ; Store first mask bit
        setb    _nb_rx_mask_state1
        sjmp    read_buf

s1_bit:

        jb		_nb_rx_mask_state2, read_buf	; s1 bit?

s_mask:
        ; We set the side mask only once (after receiving s1_bit).

        ; Finished receiving the mask bits. For future bits, we want to set the mask only
        ; to the exact side that we need. This serves as a check for the side bits we
        ; received. This check is important, since there is otherwise no way to valdiate
        ; the received side bits.
        ;
        ; At this point, the side bits have been stored independently in nb_rx_mask_bit0 and cy.
        ; We decode them rapidly using a jump tree.

       	#ifdef NBR_RX
top:
        jb  	_nb_rx_mask_bit0, bottom
        jc		left
  	  	#ifdef NBR_SQUELCH_ENABLE
        orl   	_MISC_DIR, #(MISC_NB_0_TOP)
  	  	#else
        mov   	_MISC_DIR, #((MISC_DIR_VALUE ^ MISC_NB_OUT) | MISC_NB_0_TOP)
  	    #endif
        sjmp  	s_mask_done

left:
  	    #ifdef NBR_SQUELCH_ENABLE
        orl   	_MISC_DIR, #(MISC_NB_1_LEFT)
  	  	#else
        mov   	_MISC_DIR, #((MISC_DIR_VALUE ^ MISC_NB_OUT) | MISC_NB_1_LEFT)
  	  	#endif
        sjmp  	s_mask_done

bottom:
        jc		right
  	    #ifdef NBR_SQUELCH_ENABLE
        orl   	_MISC_DIR, #(MISC_NB_2_BOTTOM)
        #else
        mov   	_MISC_DIR, #((MISC_DIR_VALUE ^ MISC_NB_OUT) | MISC_NB_2_BOTTOM)
  	  	#endif
        sjmp  	s_mask_done

right:
  	    #ifdef NBR_SQUELCH_ENABLE
        orl   	_MISC_DIR, #(MISC_NB_3_RIGHT)
  	    #else
        mov   	_MISC_DIR, #((MISC_DIR_VALUE ^ MISC_NB_OUT) | MISC_NB_3_RIGHT)
  	    #endif

		#endif	//NBR_RX

s_mask_done:

        setb    _nb_rx_mask_state2

read_buf:

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

#ifdef NBR_TX
        anl     _MISC_DIR, #~MISC_NB_OUT        ; TODO: do this just once before initiating tx
        orl     MISC_PORT, #MISC_NB_OUT
#endif
2$:
        mov     a, (_nb_buffer + 1)             ; 2  Now do a proper 16-bit shift.
        clr     c                               ; 1  Make sure to shift in a zero,
        rlc     a                               ; 1    so our suffix bit is a zero too.
        mov     (_nb_buffer + 1), a             ; 3
        mov     a, _nb_buffer                   ; 2
        rlc     a                               ; 1
        mov     _nb_buffer, a                   ; 3

        mov     a, #0x4                         ; 2
        djnz    ACC, .                          ; 16 (4 iters, 4 cycles each)
        nop                                     ; 1

#ifdef NBR_TX
        anl     MISC_PORT, #~MISC_NB_OUT        ; LOW
#endif

        ;--------------------------------------------------------------------

nb_bit_done:

        djnz    _nb_bits_remaining, nb_irq_ret  ; More bits left?

        clr     _T2CON_T2I0                     ; Nope. Disable the IRQ
        jb      _nb_tx_mode, nb_tx_handoff      ; TX mode? Nothing to store.

        ;--------------------------------------------------------------------
        ; RX Packet Completion
        ;--------------------------------------------------------------------

        mov     a, _nb_buffer
        orl     a, #0xE0                        ; Put the implied bits back in
        cpl     a                               ; Complement
        xrl     a, (_nb_buffer+1)               ; Check byte
#ifdef DEBUG_NBR
        jz		dbg_skip
        mov		a, _nb_buffer
        mov		_nbr_data_invalid, a
        mov		a, (_nb_buffer+1)
        mov		(_nbr_data_invalid+1), a
        sjmp	nb_packet_done
dbg_skip:
#endif
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
        push    0                               ; Need to use r0 for indirect addressing
        mov     r0, a

        mov     a, _nb_buffer                   ; Store in ACK-compatible format
        anl     a, #NB_ID_MASK
        orl     a, #NB_FLAG_SIDE_ACTIVE
        mov     @r0, a

#ifdef DEBUG_NBR
        mov		a, _nb_buffer
        mov		_nbr_data_valid, a
        mov		a, (_nb_buffer+1)
        mov		(_nbr_data_valid+1), a
#endif

        pop     0
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

        mov     _i2c_state, #0            ; Reset our state machine
        mov     _W2CON0, #0               ; Reset I2C master
        mov     _W2CON0, #1               ;   Turn on I2C controller
        mov     _W2CON0, #7               ;   Master mode, 100 kHz.
        mov     _W2DAT, #ACCEL_ADDR_TX    ; Trigger the next I2C transaction
        
        ;--------------------------------------------------------------------

nb_packet_done:

        mov     TL1, #0xFF                      ; RX mode: Interrupt on the next incoming edge
        mov     TH1, #0xFF
        clr     _nb_tx_mode
        setb    _TCON_TR1
#if defined(NBR_TX) || defined(NBR_RX)
        orl     _MISC_DIR, #MISC_NB_OUT         ; Let the LC tanks float
#endif

nb_irq_ret:

        clr     _IR_TF2                         ; Must ack IRQ (TF2 is not auto-clear)

        pop     psw
        pop     acc
        reti

    __endasm;
}
