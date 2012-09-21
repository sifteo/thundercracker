/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * Thundercracker cube firmware
 *
 * M. Elizabeth Scott <beth@sifteo.com>
 * Copyright <c> 2011 Sifteo, Inc. All rights reserved.
 */

#include <cube_hardware.h>
#include <protocol.h>
#include <sifteo/abi/vram.h>
#include "radio.h"
#include "flash.h"
#include "params.h"
#include "power.h"

RF_MemACKType __near ack_data;
uint8_t __near ack_bits;


// Disable to prevent radio from writing to VRAM, for testing
#define WRITE_TO_VRAM

#ifdef WRITE_TO_VRAM
#define   VRAM_MOVX_DPTR_A   movx @dptr, a
#define   VRAM_MOVX_A_DPTR   movx a, @dptr
#else
#define   VRAM_MOVX_DPTR_A   nop
#define   VRAM_MOVX_A_DPTR   movx a, @dptr
#endif


/*
 * Assembly macros.
 */

#define RX_NEXT_NYBBLE                                  __endasm; \
        __asm   djnz    R_NYBBLE_COUNT, rx_loop         __endasm; \
        __asm   ajmp    rx_complete                     __endasm; \
        __asm

#define RX_NEXT_NYBBLE_L                                __endasm; \
        __asm   djnz    R_NYBBLE_COUNT, rx_loop         __endasm; \
        __asm   ljmp    rx_complete                     __endasm; \
        __asm

// Negative sampling delta. MUST wrap at 1kB (both for correctness and security)
#pragma sdcc_hash +
#define DPTR_DELTA(v)                                   __endasm; \
        __asm   mov     a, _DPL1                        __endasm; \
        __asm   add     a, #((v) & 0xFF)                __endasm; \
        __asm   mov     _DPL, a                         __endasm; \
        __asm   mov     a, _DPH1                        __endasm; \
        __asm   addc    a, #((v) >> 8)                  __endasm; \
        __asm   anl     a, #3                           __endasm; \
        __asm   mov     _DPH, a                         __endasm; \
        __asm


/*
 * Register usage in RF_BANK.
 */

#define R_TMP                   r0
#define AR_TMP                  (RF_BANK*8 + 0)

#define R_LOW                   r1              // Low part of 16-bit substate info / run count
#define AR_LOW                  (RF_BANK*8 + 1)

#define R_HIGH                  r2              // High part of 16-bit substate info
#define AR_HIGH                 (RF_BANK*8 + 2)

#define R_SAMPLE                r3              // Low two bits, sample index. Other bits undefined.
#define AR_SAMPLE               (RF_BANK*8 + 3)

#define R_DIFF                  r4              // 4-bit raw diff in low nybble
#define AR_DIFF                 (RF_BANK*8 + 4)

#define R_STATE                 r5              // 8-bit pointer to next state
#define AR_STATE                (RF_BANK*8 + 5)

#define R_INPUT                 r6              // Current byte (current nybble in low half)
#define AR_INPUT                (RF_BANK*8 + 6)

#define R_NYBBLE_COUNT          r7
#define AR_NYBBLE_COUNT         (RF_BANK*8 + 7)

uint16_t vram_dptr;                      // Current VRAM write pointer
uint8_t radio_packet_deadline;           // Time at which we'll enter disconnected mode
__bit radio_state_reset_not_pending;     // Next packet should start with a clean slate
__bit radio_saved_dps;                   // Store DPS in a bit variable, to save space


/*
 * Utility function used by the Radio ISR to splat one or more
 * delta-encoded words into VRAM. Parameters are passed in register
 * bank RF_BANK: R_SAMPLE, R_DIFF, R_LOW (run count - 1).
 *
 * This function is allowed to trash the following registers only:
 *    R_TMP, R_HIGH, R_STATE
 *
 * Since this function is effectively a VRAM-to-VRAM copy, we can operate
 * much more efficiently by using both DPTRs simultaneously. So, we trash
 * DPL/DPH temporarily, then restore them to the state pointer after.
 *
 * We're expected to set R_STATE to 0 after we're done.
 *
 * To save stack space, we jump here rather than calling it.
 * There are only two possible ways to return:
 *
 *   - If R_STATE pointed to rxs_rle on entry, we jump directly
 *     to rxs_default, to re-process the current nybble.
 *
 *   - Otherwise, we use RX_NEXT_NYBBLE to exit.
 * 
 */

static void rx_write_deltas(void) __naked __using(RF_BANK)
{
    __asm

        ; Jump DPL/DPH backwards by the sample distance

        mov     a, R_SAMPLE
        anl     a, #3
        jz      bs0
        dec     a
        jz      bs1
        dec     a
        jz      bs2
bs3:    DPTR_DELTA(-RF_VRAM_SAMPLE_3 * 2)
        sjmp    bsE
bs2:    DPTR_DELTA(-RF_VRAM_SAMPLE_2 * 2)
        sjmp    bsE
bs1:    DPTR_DELTA(-RF_VRAM_SAMPLE_1 * 2)
        sjmp    bsE
bs0:    DPTR_DELTA(-RF_VRAM_SAMPLE_0 * 2)
bsE:

        ; We loop R_LOW+1 times total

        inc     R_LOW

        ; Convert raw Diff to 8-bit signed diff, stow in R_TMP

        mov     a, R_DIFF
        anl     a, #0xF
        add     a, #-7
        clr     c
        rlc     a
        mov     R_TMP, a
        jb      acc.7, 3$       ; Negative diff

        ; ---- Loop for positive diffs

2$:
        VRAM_MOVX_A_DPTR        ; Add and copy low byte
        inc     dptr
        add     a, R_TMP
        mov     _DPS, #1
        VRAM_MOVX_DPTR_A
        inc     dptr
        mov     _DPS, #0

        VRAM_MOVX_A_DPTR        ; Add and copy high byte
        inc     dptr
        jnc     1$
        add     a, #2           ; Add Carry to bit 1
1$:     mov     _DPS, #1
        VRAM_MOVX_DPTR_A
        inc     dptr
        mov     _DPS, #0
        djnz    R_LOW, 2$       ; Loop

        ; ---- Return

5$:
        mov     dptr, #rxs_default      ; Restore DPTR
        clr     a

        ; From RLE state? Go to rxs_default
        cjne    R_STATE, #(rxs_rle - rxs_default), 6$
        mov     R_STATE, a
        ajmp    rxs_default

        ; Otherwise, next nybble.
6$:     mov     R_STATE, a
        ajmp    rx_next_sjmp

        ; ---- Loop for negative diffs

3$:
        VRAM_MOVX_A_DPTR        ; Add and copy low byte
        inc     dptr
        add     a, R_TMP
        mov     _DPS, #1
        VRAM_MOVX_DPTR_A
        inc     dptr
        mov     _DPS, #0

        VRAM_MOVX_A_DPTR        ; Add and copy high byte
        inc     dptr
        jc      4$
        add     a, #0xFE        ; Subtract borrow from bit 1
4$:     mov     _DPS, #1
        VRAM_MOVX_DPTR_A
        inc     dptr
        mov     _DPS, #0
        djnz    R_LOW, 3$       ; Loop

        sjmp    5$

    __endasm ;
}


/*
 * Radio ISR --
 *
 *    Receive one packet from the radio, store it according to our
 *    RF protocol state machine, and write out a new buffered ACK
 *    packet. This is very performance critical, and we can save on
 *    some key sources of overhead by using assembly, and managing
 *    our register usage manually.
 *
 *    Most of this code is devoted to decoding the nybble-based
 *    compression format, as describe in protocol.h
 */

void radio_isr(void) __interrupt(VECTOR_RF) __naked __using(RF_BANK)
{
    __asm
        push    acc
        push    dpl
        push    dph
        push    _DPL1
        push    _DPH1
        push    psw
        mov     psw, #(RF_BANK << 3)

        mov     a, _DPS
        rrc     a
        mov     _radio_saved_dps, c

        mov     _DPL1, _vram_dptr
        mov     _DPH1, _vram_dptr+1

        ; Timestamp the packet arrival time with RTC2. This is supposed to be equivalent
        ; to RTC2 enableExternalCapture, but that does not seem to work correctly?

        orl     _RTC2CON, #0x10     ; Trigger RTC2 capture

rx_begin_packet:

        ;--------------------------------------------------------------------
        ; State machine reset
        ;--------------------------------------------------------------------

        ; If the last radio packet was not maximum length, reset state now.
        ; Also, if we are not connected, we always reset packet state, as we
        ; need the protocol to be stateless while in disconnected mode.

        jnb     _radio_connected, state_reset
        jb      _radio_state_reset_not_pending, no_state_reset

state_reset:
        setb    _radio_state_reset_not_pending

        mov     R_STATE, #0        
        mov     _DPL1, #0
        mov     _DPH1, #0

no_state_reset:

        ;--------------------------------------------------------------------
        ; Packet receive setup
        ;--------------------------------------------------------------------

        ; Read the length of this received packet

        clr     _RF_CSN                         ; Begin SPI transaction
        mov     _SPIRDAT, #RF_CMD_R_RX_PL_WID   ; Read RX Payload Width command
        mov     _SPIRDAT, #0                    ; First dummy byte, keep the TX FIFO full
        SPI_WAIT                                ; Wait for Command/STATUS byte
        mov     a, _SPIRDAT                     ; Keep the STATUS byte

        ; If we notice in STATUS that there is in fact no packet pending
        ; this was a spurious interrupt and we need to get out now. This can
        ; happen if a packet arrived at just the wrong spot in a previous ISR,
        ; after the IRQ flag has already been cleared by hardware but before
        ; we have removed the packet.
        ;
        ; This is similar but not identical to the STATUS check at the bottom
        ; of this loop. In that one, we still want to send an ACK, since we know
        ; that at least one packet was processed. Here we are detecting spurious
        ; IRQs, and we do NOT want to send an ACK if we get one.

        orl     a, #0xF1                        ; Set all bits that we have no interest in
        inc     a                               ; Increment. If that was really 111, we just overflowed
        jnz     1$                              ; Jump if RX non-empty

        SPI_WAIT                                ; End SPI transaction, then go to no_ack (end the ISR)
        mov     a, _SPIRDAT
        setb    _RF_CSN
        ljmp    no_ack

1$:     SPI_WAIT                                ; Wait for width byte
        mov     a, _SPIRDAT                     ; Total packet length
        setb    _RF_CSN                         ; End SPI transaction

        ; If the packet is greater than RF_PAYLOAD_MAX, it is an error. We need
        ; to discard it. This should be really rare, and honestly we only
        ; do this because the data sheet claims it is SUPER IMPORTANT.
        ; 
        ; Since we have some code here to do an RX_FLUSH anyway, we also
        ; send zero-length packets through this path. A zero-length packet
        ; only has the effects of (1) causing an ACK transmission, and (2)
        ; resetting the RX state machine, both of which are things we can do
        ; cheaply through this path. And as we can see below, throwing out
        ; zero-length packets early means much less special-casing in the
        ; RX loop!

        jz      rx_flush                        ; Skip right to RX_FLUSH if zero-length

        rl      a                               ; nybbles = 2 * length
        mov     R_NYBBLE_COUNT, a

        add     a, #(0xFF - (2 * RF_PAYLOAD_MAX))
        jnc     no_rx_flush                     ; Jump if byte length <= RF_PAYLOAD_MAX

rx_flush:
        clr     _radio_state_reset_not_pending  ; Error/empty packets also cause a state reset
        clr     _RF_CSN                         ; Begin SPI transaction
        mov     _SPIRDAT, #RF_CMD_FLUSH_RX      ; RX_FLUSH command
        SPI_WAIT                                ; Wait for command byte
        mov     a, _SPIRDAT                     ; Ignore dummy STATUS byte
        ljmp    #rx_complete_0                  ; Skip the RX loop (and end SPI transaction)

no_rx_flush:
        inc     a                               ; Is packet not max-length?
        jz      #9$
        clr     _radio_state_reset_not_pending  ;   reset state before the next packet
9$:

        ; Start reading the incoming packet, then loop over all bytes.
        ; We try to keep the SPI FIFOs full here, and we expect the
        ; byte processing loop to be at least 16 clock cycles long, so
        ; we do not explicitly check the SPI status after each byte.

        clr     _RF_CSN                         ; Begin SPI transaction
        mov     _SPIRDAT, #RF_CMD_R_RX_PAYLOAD  ; Start reading RX packet
        mov     _SPIRDAT, #0                    ; First dummy byte, keep the TX FIFO full
        SPI_WAIT                                ; Wait for Command/STATUS byte
        mov     a, _SPIRDAT                     ; Ignore status

        ;--------------------------------------------------------------------
        ; State Machine
        ;--------------------------------------------------------------------

        mov     _DPS, #0
        mov     dptr, #rxs_default

rx_loop:                                        ; Fetch the next byte or nybble
        mov     a, R_NYBBLE_COUNT
        jnb     acc.0, 1$
        mov     a, R_INPUT                      ; ... Next nybble
        swap    a
        sjmp    2$

1$:     SPI_WAIT
        mov     a, _SPIRDAT                     ; ... Next byte
        mov     _SPIRDAT, #0
2$:     mov     R_INPUT, a

        ; Branch to state handler.

        ; All state handlers must be within a 255-byte delta of
        ; rxs_default! If you add code to the state machine,
        ; check radio.rst to make sure you have not crossed
        ; this barrier.
        ;
        ; This code is currently VERY close to the limit.
        ; If you are exceeding it, first try to optimize the
        ; other state machine code to squeeze out a few bytes.
        ; If that is not enough, you can put less frequently
        ; used sections of code elsewhere, and ljmp to them.

        ; NOTE dyn_branch rx_j rxs_

        mov     a, R_STATE
rx_j:   jmp     @a+dptr 

        ;-------------------------------------------
        ; 4-bit diff (continued from below)
        ;-------------------------------------------

rxs_diff_2:
        mov     R_DIFF, a
        mov     R_LOW, #0
        ajmp    _rx_write_deltas

        ;-------------------------------------------
        ; Default state (initial nybble)
        ;-------------------------------------------

rxs_default:
        mov     a, R_INPUT
        anl     a, #0xc

        ; ------------ Nybble 00nn -- RLE

        jnz     27$
        mov     AR_LOW, R_INPUT
        mov     R_STATE, #(rxs_rle - rxs_default)
        RX_NEXT_NYBBLE_L

        ; ------------ Nybble 01ss -- Copy

27$:
        cjne    a, #0x4, 11$
        mov     AR_SAMPLE, R_INPUT
        mov     R_DIFF, #RF_VRAM_DIFF_BASE
        mov     R_LOW, #0
        ajmp    _rx_write_deltas
11$:

        ; ------------ Nybble 10ss -- Diff

        cjne    a, #0x8, 12$
        mov     AR_SAMPLE, R_INPUT
        mov     R_STATE, #(rxs_diff_1 - rxs_default)
        RX_NEXT_NYBBLE_L
12$:

        ; ------------ Nybble 11xx -- Literal Index

        mov     a, R_INPUT
        rr      a
        rr      a
        anl     a, #0xC0
        mov     R_HIGH, a       ; Store into R_HIGH, as xx000000

        mov     R_SAMPLE, #0    ; Any subsequent runs will copy this word (S=0 D=0)
        mov     R_DIFF, #RF_VRAM_DIFF_BASE

        mov     R_STATE, #(rxs_literal - rxs_default)

rx_next_sjmp:
        RX_NEXT_NYBBLE_L

        ;-------------------------------------------
        ; 4-bit diff
        ;-------------------------------------------

rxs_diff_1:
        mov     a, R_INPUT
        anl     a, #0xF
        cjne    a, #7, rxs_diff_2
        ljmp    rx_special              ; Redundant copy encoding, special meaning

        ;-------------------------------------------
        ; Literal 14-bit index
        ;-------------------------------------------

rxs_literal:
        mov     a, R_INPUT
        anl     a, #0xF
        mov     R_LOW, a        ; Store into R_LOW, as 0000xxxx

        mov     R_STATE, #(rxs_l1 - rxs_default)
        sjmp    rx_next_sjmp

rxs_l1:
        mov     a, R_INPUT
        swap    a
        anl     a, #0xF0
        orl     AR_LOW, a       ; Add to R_LOW, as xxxx0000

        mov     R_STATE, #(rxs_l2 - rxs_default)
        sjmp    rx_next_sjmp

rxs_l2:
        mov     _DPS, #1        ; Switch to VRAM DPTR

        clr     c               ; Shift a zero into R_LOW, and MSB into C
        mov     a, R_LOW
        rlc     a
        VRAM_MOVX_DPTR_A        ; Store low byte
        inc     dptr

        mov     a, R_INPUT
        rlc     a               ; Shift R_LOW MSB into R_HIGH LSB
        rlc     a               ; And again (dummy bit)
        anl     a, #0x3E        ; Mask covers input nybble plus shifted MSB
        orl     a, R_HIGH       ; Combine with saved two MSBs
        VRAM_MOVX_DPTR_A        ; Store high byte
        inc     dptr
        anl     _DPH1, #3       ; Wrap DPH1 at 1 kB        

        mov     _DPS, #0

        mov     R_STATE, #0     ; Back to default state
        sjmp    rx_next_sjmp

        ;-------------------------------------------
        ; RLE codes
        ;-------------------------------------------

        ; We just saw a single RLE code. If it is followed by another
        ; RLE code, that needs special treatment. Otherwise, handle it
        ; as a normal nybble after processing the runs from our first
        ; RLE nybble.

rxs_rle:

        mov     a, R_INPUT
        anl     a, #0xc

        ; -------- 00nn -- Plain RLE code

        jz      13$

        anl     AR_LOW, #0xF            ; Only the low nybble is part of the valid run length
        ajmp    _rx_write_deltas

13$:
        mov     a, R_LOW        ; Check low two bits of the _first_ RLE nybble

        ; -------- 000n 00nn -- Skip n+1 output words

        jb      acc.1, 14$      ; Jump if not a 000n nybble

        rrc     a               ; Rotate low bit of skip count into C
        mov     a, R_INPUT
        rlc     a               ; Rotate into next nybble
        anl     a, #7           ; Mask to 3 bits
        inc     a               ; Skip n+1 words
        rl      a               ; Words to bytes
        mov     R_LOW, a        ; Stow temporarily

        mov     _DPS, #1        ; Switch to VRAM DPTR
        mov     a, _DPL1        ; 16-bit add, dptr += R_LOW
        add     a, R_LOW
        mov     _DPL1, a
        mov     a, _DPH1
        addc    a, #0
        anl     a, #3           ; Wrap DPH1 at 1 kB
        mov     _DPH1, a
        mov     _DPS, #0

        mov     R_STATE, #0
        sjmp    rx_next_sjmp

14$:

        ; -------- 0010 00nn nnnn -- Write n+5 delta-words

        jb      acc.0, not_wrdelta

        mov     a, R_INPUT
        swap    a
        anl     a, #0x30        ; Mask to 00nn0000
        mov     R_LOW, a

        mov     R_STATE, #(rxs_wrdelta_1 - rxs_default)

rx_next_sjmp2:
        sjmp    rx_next_sjmp

rxs_wrdelta_1:
        sjmp    rxs_wrdelta_1_fragment

not_wrdelta:

        ; -------- 0011 000x xxxx xxxx -- Set literal 9-bit word address

        mov     a, R_INPUT      ; Done checking the first nybble at this point
        jb      acc.1, rx_not_word9

        mov     a, R_INPUT
        anl     a, #1           ; Store bit 9
        mov     R_HIGH, a

        mov     R_STATE, #(rxs_word9 - rxs_default)
        sjmp    rx_next_sjmp2

rxs_word9:
        mov     a, R_INPUT
        anl     a, #0xF         ; Store bits 4321
        mov     R_LOW, a

        mov     R_STATE, #(rxs_w6 - rxs_default)
        sjmp    rx_next_sjmp2

rxs_w6:
        mov     a, R_INPUT
        swap    a               ; Store bits 8765
        anl     a, #0xF0
        orl     a, R_LOW        ; Full low byte (87654321) assembled

        mov     _DPS, #1        ; Switch to VRAM DPTR
        clr     c               ; Words -> Bytes, carry bit 8 over into DPH.
        rlc     a
        mov     _DPL1, a
        mov     a, R_HIGH
        rlc     a
        mov     _DPH1, a        ; Max value 00000011
        mov     _DPS, #0

        mov     R_STATE, #0
        sjmp    rx_next_sjmp2

rx_not_word9:

        ; -------- 0011 0010 xxxx xxxx xxxx xxxx -- Write literal 16-bit word

        jb      acc.0, rxs_w5

        mov     R_STATE, #(rxs_w1 - rxs_default)
        sjmp    rx_next_sjmp2

rxs_w1:
        mov     a, R_INPUT
        anl     a, #0xF
        mov     R_LOW, a        ; Store bits 3210

        mov     R_STATE, #(rxs_w2 - rxs_default)
        sjmp    rx_next_sjmp2

rxs_w2:
        mov     a, R_INPUT
        anl     a, #0xF
        swap    a
        orl     AR_LOW, a       ; Store bits 7654

        mov     R_STATE, #(rxs_w3 - rxs_default)
        sjmp    rx_next_sjmp2

rxs_w3:
        mov     a, R_INPUT
        anl     a, #0xF
        mov     R_HIGH, a       ; Store bits ba98

        mov     R_STATE, #(rxs_w4 - rxs_default)
        sjmp    rx_next_sjmp2

rxs_w4:
        mov     a, R_INPUT
        anl     a, #0xF
        swap    a
        orl     AR_HIGH, a      ; Store bits fedc

        mov     _DPS, #1        ; Switch to VRAM DPTR
        mov     a, R_LOW
        VRAM_MOVX_DPTR_A        ; Store low byte
        inc     dptr
        mov     a, R_HIGH
        VRAM_MOVX_DPTR_A        ; Store high byte
        inc     dptr
        anl     _DPH1, #3       ; Wrap DPH1 at 1 kB
        mov     _DPS, #0

        mov     R_SAMPLE, #0    ; Any subsequent runs will copy this word (S=0 D=0)
        mov     R_DIFF, #RF_VRAM_DIFF_BASE

        mov     R_STATE, #0     ; Back to default state
        sjmp    rx_next_sjmp2

rxs_w5:

        ; -------- 0011 0011 -- Escape to flash mode

        ; This will consume the entire remainder of the packet,
        ; stowing it in the flash FIFO. If there are no full bytes
        ; left in the packet, it acts as a flash reset.

        mov     R_STATE, #0     ; Back to default state
        sjmp    rx_flash


        ; -------- 0010 00nn nnnn -- Write n+5 delta-words
        ; (Continued from above, due to jump length limits)

rxs_wrdelta_1_fragment:

        mov     a, R_INPUT
        anl     a, #0xF
        orl     a, R_LOW        ; Complete word 00nnnnnn
        add     a, #4           ; n+5  (rx_write_deltas already adds 1)
        mov     R_LOW, a

        ljmp    _rx_write_deltas

        ;--------------------------------------------------------------------
        ; Special escape codes (8-bit copy encoding)
        ;--------------------------------------------------------------------

        ; These are all escape codes which return to rx_complete after optionally
        ; dequeueing argument bytes directly from the RX FIFO.

rx_special:

        mov     R_STATE, #0                     ; Catch-all, go back to default state
        anl     AR_SAMPLE, #3                   ; Specific ops are in low two bits...

        ; -------- 1000 0111 <LOW> <HIGH> -- Sensor timer sync escape

        cjne    R_SAMPLE, #0, 1$

        clr     _TCON_TR0                       ; Stop timer
        mov     TL0, _SPIRDAT                   ; Low byte
        mov     _SPIRDAT, #0
        SPI_WAIT
        mov     TH0, _SPIRDAT                   ; High byte
        mov     _SPIRDAT, #0
        setb    _TCON_TR0                       ; Restart timer

        sjmp    rx_complete
1$:

        ; -------- 1001 0111 -- Explicit ACK request

        cjne    R_SAMPLE, #1, 3$

        mov     _ack_bits, #0xFF                ; Do ALL the acks!

        sjmp    rx_complete
3$:

        ; -------- 1010 0111 -- Radio Hop

        cjne    R_SAMPLE, #2, 2$
        ljmp    _radio_hop
2$:

        ; -------- 1011 0111 <LOW> <HIGH> -- Radio nap

        ; Turn off the radio, and turn it on at the given time (measured
        ; in CLKLF ticks) after the current packet timestamp, which was captured
        ; by RTC2 when the radio interrupt occurred.

        mov     a, _SPIRDAT                     ; Low byte
        mov     _SPIRDAT, #0
        add     a, _RTC2CPT00                   ; Add current packet timestamp
        mov     _RTC2CMP0, a                    ; Set RTC2 compare value
        SPI_WAIT                                ; (Does not affect flags)
        mov     a, _SPIRDAT                     ; High byte
        mov     _SPIRDAT, #0
        addc    a, _RTC2CPT01                   ; Add high bytes
        mov     _RTC2CMP1, a

        ; Enable RTC2 compare interrupt, and turn the radio off. We turn it back on
        ; in the RTC2 ISR.

        mov     _RTC2CON, #0x05
        clr     _RF_CE

        sjmp    rx_complete

        ;--------------------------------------------------------------------
        ; Flash FIFO Write
        ;--------------------------------------------------------------------

        ; Check whether we should do a flash reset, and calculate how
        ; many bytes of flash data we are writing. These are all based
        ; off of R_NYBBLE_COUNT:
        ;
        ;   R_NYBBLE_COUNT = 0       Illegal value
        ;   R_NYBBLE_COUNT = 1       This is the last nybble in the packet (reset)
        ;   R_NYBBLE_COUNT = 2       One dummy nybble left in packet (reset)
        ;   R_NYBBLE_COUNT = 3       One byte
        ;   R_NYBBLE_COUNT = 4       One byte followed by a dummy nybble
        ;   R_NYBBLE_COUNT = 5       Two bytes
        ;   R_NYBBLE_COUNT = 6       Two bytes followed by a dummy nybble
        ;   etc.
        ;
        ; We can calculate the number of complete bytes here as (nybble_count - 1) / 2.
        ; If that byte count is zero, we reset. Ignoring the dummy nybbles requries
        ; no special effort, since we pull bytes directly from SPI now.

rx_flash:

        mov     a, R_NYBBLE_COUNT
        dec     a
        clr     c
        rrc     a                               ; Byte count
        jz      rx_flash_reset                  ;    Zero bytes, do a flash reset
        mov     R_NYBBLE_COUNT, a               ;    Otherwise, this is the new loop iterator   
        
rx_flash_loop:
        mov     R_TMP, _flash_fifo_head         ; Load the flash write pointer

        SPI_WAIT
        mov     a, _SPIRDAT                     ; Load next SPI byte
        mov     _SPIRDAT, #0
        mov     @R_TMP, a                       ; Store it in the FIFO

        inc     R_TMP                           ; Advance head pointer
        cjne    R_TMP, #(_flash_fifo + FLS_FIFO_SIZE), 1$
        mov     R_TMP, #_flash_fifo
1$:     mov     _flash_fifo_head, R_TMP

        djnz    R_NYBBLE_COUNT, rx_flash_loop
        sjmp    rx_complete

rx_flash_reset:
        setb    _flash_reset_request

        ; ... fall through to rx_complete


        ;--------------------------------------------------------------------
        ; Packet receive completion
        ;--------------------------------------------------------------------

rx_complete:
        SPI_WAIT
        mov     a, _SPIRDAT     ; Throw away one extra dummy byte
rx_complete_0:
        setb    _RF_CSN         ; End SPI transaction
rx_complete_1:

        ; Push back our disconnection deadline. We disconnect if there have not been
        ; any radio packets in 256 to 512 TF0 ticks. (There is an uncertainty of one
        ; low-byte rollover, making this the minimum nonzero timeout).

        ; The base has a disconnect timeout maximum of 1525 millis
        ; for a single cube. we still want to avoid disconnecting while the
        ; base times out more than one cube before it gets around to us.
        ; we wait up to 3 to 4 seconds.

        mov     a, _sensor_tick_counter_high
        add     a, #4
        mov     _radio_packet_deadline, a

        ; nRF Interrupt acknowledge

        clr     _RF_CSN                                         ; Begin SPI transaction
        mov     _SPIRDAT, #(RF_CMD_W_REGISTER | RF_REG_STATUS)  ; Start writing to STATUS
        mov     _SPIRDAT, #RF_STATUS_RX_DR                      ; Clear interrupt flag
        SPI_WAIT                                                ; RX STATUS byte
        mov     a, _SPIRDAT

        ; We may have had multiple packets queued. Typically we can handle incoming
        ; packets at line rate, but if there is a particularly long VRAM write that
        ; still compresses into one packet, it could take us longer. We do not have
        ; to worry about flow control here, since the nRF ShockBurst protocol handles
        ; that implicitly via retries. But we DO need to account for the fact that we
        ; may have fewer total IRQs than we have packets, and we need to "catch up"
        ; after a long packet comes in.
        ;
        ; Luckily, handling this is simple. We already know the STATUS register
        ; as of the moment we acknowledged this IRQ. If a packet is still ready,
        ; go back to the top of the IRQ handler and do this all again.
        ;
        ; The RX FIFO status is in bits 3:1. '111' means the FIFO is empty.
        ; Check this with as little overhead as we can manage.

        orl     a, #0xF1        ; Set all bits that we have no interest in
        inc     a               ; Increment. If that was really 111, we just overflowed
        jz      1$              ; Jump if RX Empty

        SPI_WAIT                ; End SPI transaction, then go to rx_begin_packet
        mov     a, _SPIRDAT
        setb    _RF_CSN
        ljmp    rx_begin_packet
1$:     SPI_WAIT                ; End SPI transaction, then send ACK
        mov     a, _SPIRDAT
        setb    _RF_CSN

        ;--------------------------------------------------------------------
        ; ACK packet write
        ;--------------------------------------------------------------------

        ; The packet length comes from _ack_bits, and the first portion of
        ; the data from _ack_data. To send data back to the master, we
        ; update bytes in _ack_data, then set the corresponding bit in
        ; ack_bits. We decode those bits into a length here.
        ;
        ; One caveat: We should be sure to reset ack_bits before sampling the
        ; actual ACK packet data, so that it is not possible for ACKs to get
        ; lost if a higher-priority IRQ preempts us and sets an ack_bit.
        ;
        ; Note: We can send up to one ACK packet per receive ISR right here.
        ;       We can also enqueue ACKs from the main thread, as part of an
        ;       asynchronous query. Normally we have no more than one buffered
        ;       ACK packet, but query responses may arbitrarily increase that
        ;       number, up until we hit the hardware limit of 3 packets.
        ;
        ;       Ideally we would avoid transmitting this automatic ACK if
        ;       the hardware buffer is not empty, in order to avoid inflating
        ;       our radio latency. But the nRF does not include this information
        ;       in the STATUS byte. We would have to read FIFO_STATUS in a
        ;       separate SPI transaction. That really is not worth the
        ;       performance penalty of doing this on every packet.

rx_ack:

        mov     a, #0xFF
        jnb     _radio_connected, 1$                    ; Always send full ACK when disconnected
        mov     a, _ack_bits                            ; Leave ack_bits in acc
1$:
        jz      no_ack                                  ; Skip the ACK entirely if empty
        mov     _ack_bits, #0                           ; Reset pending ACK bits

        clr     _RF_CSN                                 ; Begin SPI transaction
        mov     _SPIRDAT, #RF_CMD_W_ACK_PAYLD           ; Start sending ACK packet
        mov     R_TMP, #_ack_data

        ; Decode length of the memory portion of the packet, from ack_bits.
        ; Must be in descending order. If we are also sending the HWID,
        ; set the carry bit.

        clr     c

        jnb     RF_ACK_ABIT_HWID, 10$
        mov     R_INPUT, #RF_MEM_ACK_LEN
        setb    c
        sjmp    20$

10$:    jnb     RF_ACK_ABIT_BATTERY_V, 11$
        mov     R_INPUT, #RF_ACK_LEN_BATTERY_V
        sjmp    20$

11$:    jnb     RF_ACK_ABIT_FLASH_FIFO, 12$
        mov     R_INPUT, #RF_ACK_LEN_FLASH_FIFO
        sjmp    20$

12$:    jnb     RF_ACK_ABIT_NEIGHBOR, 13$
        mov     R_INPUT, #RF_ACK_LEN_NEIGHBOR
        sjmp    20$

13$:    jnb     RF_ACK_ABIT_ACCEL, 14$
        mov     R_INPUT, #RF_ACK_LEN_ACCEL
        sjmp    20$

14$:    mov     R_INPUT, #RF_ACK_LEN_FRAME
20$:

        ; Send the portion of the packet that comes from ack_data.
        
3$:     mov     _SPIRDAT, @R_TMP
        inc     R_TMP
        SPI_WAIT
        mov     a, _SPIRDAT                             ; RX dummy byte
        djnz    R_INPUT, 3$
        
        ; If we still need to, send the hardware ID to complete our full packet.
        
        jnc     4$
        mov     _DPS, #0
        mov     dptr, #PARAMS_HWID
        mov     R_INPUT, #HWID_LEN
5$:     movx    a, @dptr
        mov     _SPIRDAT, a
        inc     dptr
        SPI_WAIT
        mov     a, _SPIRDAT                             ; RX dummy byte
        djnz    R_INPUT, 5$
4$:

        ; End of ACK

        SPI_WAIT                                        ; RX last dummy byte
        mov     a, _SPIRDAT
        setb    _RF_CSN                                 ; End SPI transaction
no_ack:

        ;--------------------------------------------------------------------
        ; Touch sensing
        ;--------------------------------------------------------------------

        ; This complements the touch sensing code in TF0. There, we only SET the
        ; touch bit, in order to stretch brief pulses that may not make it into
        ; a radio packet otherwise. Here, we already sent the ACK buffer back to
        ; the master, so we can finally unset any touch state that may have been
        ; set by TF0.
        ;
        ; At this point, our job is as easy as copying the value of the
        ; MISC_TOUCH pin to the NB0_FLAG_TOUCH bit in our ACK packet.
        ;
        ; Warning: This code assumes that (MISC_TOUCH >> 1) == NB0_FLAG_TOUCH.

        mov     a, _MISC_PORT
        rr      a
        xrl     a, (_ack_data + RF_ACK_NEIGHBOR + 0)
        anl     a, #NB0_FLAG_TOUCH
        jz      1$
        xrl     (_ack_data + RF_ACK_NEIGHBOR + 0), a
        orl     _ack_bits, #RF_ACK_BIT_NEIGHBOR
1$:

        ;--------------------------------------------------------------------
        ; Cleanup
        ;--------------------------------------------------------------------

        mov     c, _radio_saved_dps
        rlc     a
        mov     _DPS, a

        mov     _vram_dptr, _DPL1
        mov     _vram_dptr+1, _DPH1

        pop     psw
        pop     _DPH1
        pop     _DPL1
        pop     dph
        pop     dpl
        pop     acc
        reti
        
    __endasm ;
}


/*
 * radio_hop --
 *
 *    This handles the Radio Hop command, which escapes from nybble
 *    mode and takes up to 7 bytes of arguments:
 *
 *      - Radio channel
 *      - Radio address (5 byte)
 *      - Neighbor ID
 *
 *    This command always switches to Connected mode, if we aren't
 *    there already. Also, we always cause a state reset, in part because
 *    this function requires re-using a bunch of our state registers
 *    as temporary space to store the new radio address.
 *
 *    Note that the hop occurs immediately without leaving the ISR, but
 *    after the radio has already sent the ACK. It's possible that this
 *    ACK may get dropped, so if the master sees a timeout it may need
 *    to retry on our new channel/address.
 *
 *    Temporary space used:
 *
 *      r0                      Loop iterator / pointer
 *      R_LOW+ (r1-r7)          Parameter bytes
 *      _vram_dptr              Count of argument bytes
 *      _vram_dptr+1            Copy of argument count
 */

void radio_hop(void) __naked __using(RF_BANK)
{
    __asm
        ; State changes

        clr     _radio_state_reset_not_pending
        setb    _radio_connected

        ;--------------------------------------------------------------------
        ; Read Arguments
        ;--------------------------------------------------------------------

        mov     a, R_NYBBLE_COUNT               ; Convert nybble count to byte count
        dec     a
        clr     c
        rrc     a
        jz      10$                             ; Zero bytes (done)
        mov     _vram_dptr, a                   ; Otherwise, this is the new byte count

        add     a, #(256 - 7)                   ; Clamp count to 7 (ignore remainder)
        jnc     1$
        mov     _vram_dptr, #7
1$:

        mov     _vram_dptr+1, _vram_dptr        ; Save copy of argument count
        mov     R_TMP, #AR_LOW                  ; Start writing to r1

2$:
        SPI_WAIT
        mov     a, _SPIRDAT                     ; Load next SPI byte
        mov     _SPIRDAT, #0                    ; Dummy write
        mov     @R_TMP, a                       ; Store it in our temporary space

        inc     R_TMP                           ; Advance head pointer
        djnz    _vram_dptr, 2$                  ; Next byte

        ;--------------------------------------------------------------------
        ; Write Channel
        ;--------------------------------------------------------------------

        SPI_WAIT
        mov     a, _SPIRDAT                     ; Throw away one extra dummy byte
        setb    _RF_CSN                         ; End SPI transaction
        clr     _RF_CE                          ; Turn off radio receiver temporarily
        clr     _RF_CSN                         ; Begin SPI transaction

        mov     _SPIRDAT, #(RF_CMD_W_REGISTER | RF_REG_RF_CH)

        mov     a, R_LOW                        ; Channel is masked with 0x7F to stay in hardware spec
        anl     a, #0x7F
        mov     _SPIRDAT, a

        SPI_WAIT                                ; Flush one byte, leave a single dummy byte in FIFO
        mov     a, _SPIRDAT

        ;--------------------------------------------------------------------
        ; Write Address
        ;--------------------------------------------------------------------

        mov     a, _vram_dptr+1                 ; Only proceed if we have 6+ bytes
        add     a, #(256 - 6)
        jnc     10$

        SPI_WAIT
        setb    _RF_CSN                         ; End SPI transaction
        mov     a, _SPIRDAT                     ; Throw away one extra dummy byte from FIFO
        clr     _RF_CSN                         ; Begin SPI transaction

        mov     _SPIRDAT, #(RF_CMD_W_REGISTER | RF_REG_RX_ADDR_P0)

        mov     R_TMP, #(AR_LOW + 1)            ; Write 5 bytes, starting with the 2nd buffered byte
3$:
        mov     a, @R_TMP
        mov     _SPIRDAT, a
        inc     R_TMP
        SPI_WAIT
        mov     a, _SPIRDAT
        cjne    R_TMP, #(AR_LOW + 1 + 5), 3$

        ;--------------------------------------------------------------------
        ; Neighbor ID
        ;--------------------------------------------------------------------

        ; Note: We expect the neighbor ID to already have the three high bits set,
        ;       but we do not enforce this. Maybe it will be useful in the future to
        ;       transmit slightly different neighbor packets under master control.

        mov     a, _vram_dptr+1
        cjne    a, #7, 10$                      ; Only proceed if we have exactly 7 bytes
        mov     _nb_tx_id, (AR_LOW + 6)         ;   Save byte 7 as new neighbor ID

        ;--------------------------------------------------------------------
        ; Finish packet
        ;--------------------------------------------------------------------

10$:

        SPI_WAIT
        mov     a, _SPIRDAT     ; Throw away one extra dummy byte
        setb    _RF_CSN         ; End SPI transaction
        setb    _RF_CE          ; Turn radio receiver back on

        ljmp    rx_complete_1

    __endasm ;
}
