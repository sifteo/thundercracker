/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * Thundercracker cube firmware
 *
 * M. Elizabeth Scott <beth@sifteo.com>
 * Copyright <c> 2011 Sifteo, Inc. All rights reserved.
 */

#include "hardware.h"
#include "radio.h"
#include "flash.h"
#include <protocol.h>

RF_ACKType __near ack_data;
uint8_t __near ack_len;


/*
 * Assembly macros.
 */

#define SPI_WAIT                                        __endasm; \
        __asm   mov     a, _SPIRSTAT                    __endasm; \
        __asm   jnb     acc.2, (.-2)                    __endasm; \
        __asm

#define RX_NEXT_NYBBLE                                  __endasm; \
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

static uint16_t vram_dptr;                      // Current VRAM write pointer
static __bit radio_state_reset_not_pending;     // Next packet should start with a clean slate


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

        ; Convert raw Diff to 8-bit signed diff, stow in R_STATE

        mov     a, R_DIFF
        anl     a, #0xF
        add     a, #-7
        clr     c
        rlc     a
        mov     R_STATE, a
        jb      acc.7, 3$       ; Negative diff

        ; ---- Loop for positive diffs

2$:
        movx    a, @dptr        ; Add and copy low byte
        inc     dptr
        add     a, R_STATE
        mov     _DPS, #1
        movx    @dptr, a
        inc     dptr
        mov     _DPS, #0

        movx    a, @dptr        ; Add and copy high byte
        inc     dptr
        jnc     1$
        add     a, #2           ; Add Carry to bit 1
1$:     mov     _DPS, #1
        movx    @dptr, a
        inc     dptr
        mov     _DPS, #0
        djnz    R_LOW, 2$       ; Loop

        ; Restore registers and exit
        mov     R_STATE, #0
        mov     dptr, #rxs_default
        ret

        ; ---- Loop for negative diffs

3$:
        movx    a, @dptr        ; Add and copy low byte
        inc     dptr
        add     a, R_STATE
        mov     _DPS, #1
        movx    @dptr, a
        inc     dptr
        mov     _DPS, #0

        movx    a, @dptr        ; Add and copy high byte
        inc     dptr
        jc      4$
        add     a, #0xFE        ; Subtract borrow from bit 1
4$:     mov     _DPS, #1
        movx    @dptr, a
        inc     dptr
        mov     _DPS, #0
        djnz    R_LOW, 3$       ; Loop

        ; Restore registers and exit
        mov     R_STATE, #0
        mov     dptr, #rxs_default
        ret

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
        push    _DPS
        push    _DPL1
        push    _DPH1
        push    psw
        mov     psw, #(RF_BANK << 3)
        mov     _DPL1, _vram_dptr
        mov     _DPH1, _vram_dptr+1

rx_begin_packet:

        ;--------------------------------------------------------------------
        ; State machine reset
        ;--------------------------------------------------------------------

        ; If the last radio packet was not maximum length, reset state now.

        jb      _radio_state_reset_not_pending, no_state_reset
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
        SPI_WAIT                                ; Wait for first data byte

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

1$:     mov     a, _SPIRDAT                     ; ... Next byte
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

        mov     a, R_STATE
        jmp     @a+dptr 

        ;-------------------------------------------
        ; 4-bit diff (continued from below)
        ;-------------------------------------------

rxs_diff_2:
        mov     R_DIFF, a
        mov     R_LOW, #0
        lcall   #_rx_write_deltas
        sjmp    #rx_next_sjmp


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
        RX_NEXT_NYBBLE

        ; ------------ Nybble 01ss -- Copy

27$:
        cjne    a, #0x4, 11$
        mov     AR_SAMPLE, R_INPUT
        mov     R_DIFF, #RF_VRAM_DIFF_BASE
        mov     R_LOW, #0
        lcall   #_rx_write_deltas
        RX_NEXT_NYBBLE
11$:

        ; ------------ Nybble 10ss -- Diff

        cjne    a, #0x8, 12$
        mov     AR_SAMPLE, R_INPUT
        mov     R_STATE, #(rxs_diff_1 - rxs_default)
        RX_NEXT_NYBBLE
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
        RX_NEXT_NYBBLE

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

        mov     R_STATE, #(19$ - rxs_default)
        sjmp    rx_next_sjmp

19$:
        mov     a, R_INPUT
        swap    a
        anl     a, #0xF0
        orl     AR_LOW, a       ; Add to R_LOW, as xxxx0000

        mov     R_STATE, #(20$ - rxs_default)
        sjmp    rx_next_sjmp

20$:
        mov     _DPS, #1        ; Switch to VRAM DPTR

        clr     c               ; Shift a zero into R_LOW, and MSB into C
        mov     a, R_LOW
        rlc     a
        movx    @dptr, a        ; Store low byte
        inc     dptr

        mov     a, R_INPUT
        rlc     a               ; Shift R_LOW MSB into R_HIGH LSB
        rlc     a               ; And again (dummy bit)
        anl     a, #0x3E        ; Mask covers input nybble plus shifted MSB
        orl     a, R_HIGH       ; Combine with saved two MSBs
        movx    @dptr, a        ; Store high byte
        inc     dptr

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
        lcall   #_rx_write_deltas
        ljmp    rxs_default             ; Re-process this nybble starting from the default state

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

        mov     R_STATE, #(21$ - rxs_default)
        sjmp    rx_next_sjmp2

21$:
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

        jb      acc.0, 22$

        mov     R_STATE, #(23$ - rxs_default)
        sjmp    rx_next_sjmp2

23$:
        mov     a, R_INPUT
        anl     a, #0xF
        mov     R_LOW, a        ; Store bits 3210

        mov     R_STATE, #(24$ - rxs_default)
        sjmp    rx_next_sjmp2

24$:
        mov     a, R_INPUT
        anl     a, #0xF
        swap    a
        orl     AR_LOW, a       ; Store bits 7654

        mov     R_STATE, #(25$ - rxs_default)
        sjmp    rx_next_sjmp2

25$:
        mov     a, R_INPUT
        anl     a, #0xF
        mov     R_HIGH, a       ; Store bits ba98

        mov     R_STATE, #(26$ - rxs_default)
        sjmp    rx_next_sjmp2

26$:
        mov     a, R_INPUT
        anl     a, #0xF
        swap    a
        orl     AR_HIGH, a      ; Store bits fedc

        mov     _DPS, #1        ; Switch to VRAM DPTR
        mov     a, R_LOW
        movx    @dptr,a         ; Store low byte
        inc     dptr
        mov     a, R_HIGH
        movx    @dptr,a         ; Store high byte
        inc     dptr
        mov     _DPS, #0

        mov     R_SAMPLE, #0    ; Any subsequent runs will copy this word (S=0 D=0)
        mov     R_DIFF, #RF_VRAM_DIFF_BASE

        mov     R_STATE, #0     ; Back to default state
        sjmp    rx_next_sjmp2

22$:

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

        lcall   #_rx_write_deltas
        sjmp    #rx_next_sjmp2


        ;--------------------------------------------------------------------
        ; Special codes (8-bit copy encoding)
        ;--------------------------------------------------------------------

rx_special:

        ; -------- 1000 0111 <LOW> <HIGH> -- Sensor timer sync escape

        anl     AR_SAMPLE, #3
        cjne    R_SAMPLE, #0, 1$

        clr     _TCON_TR0                       ; Stop timer
        mov     TL0, _SPIRDAT                   ; Low byte
        mov     _SPIRDAT, #0
        SPI_WAIT
        mov     TH0, _SPIRDAT                   ; High byte
        mov     _SPIRDAT, #0
        setb    _TCON_TR0                       ; Restart timer
1$:

        ; -------- 

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
        ;
        ; This is a pretty tight loop- we need to take at least 16 clock cycles
        ; between SPI reads, and the timing here is kind of close. So, the loop
        ; is annotated with cycle counts.

rx_flash:

        mov     a, R_NYBBLE_COUNT
        dec     a
        clr     c
        rrc     a                               ; Byte count
        jz      rx_flash_reset                  ;    Zero bytes, do a flash reset
        mov     R_NYBBLE_COUNT, a               ;    Otherwise, this is the new loop iterator   

rx_flash_loop:
        mov     a, _flash_fifo_head             ; 2  Load the flash write pointer
        add     a, #_flash_fifo                 ; 2  Address relative to flash_fifo[]
        mov     R_TMP, a                        ; 2

        mov     a, _SPIRDAT                     ; 2  Load next SPI byte
        mov     _SPIRDAT, #0                    ; 3
        mov     @R_TMP, a                       ; 3  Store it in the FIFO

        mov     a, _flash_fifo_head             ; 2  Advance head pointer
        inc     a                               ; 1
        anl     a, #(FLS_FIFO_SIZE - 1)         ; 2
        mov     _flash_fifo_head, a             ; 3

        djnz    R_NYBBLE_COUNT, rx_flash_loop   ; 3
        sjmp    rx_complete                     ; 3

rx_flash_reset:
        mov     _flash_fifo_head, #FLS_FIFO_RESET

        ; ... fall through to rx_complete


        ;--------------------------------------------------------------------
        ; Packet receive completion
        ;--------------------------------------------------------------------

rx_complete:
        SPI_WAIT
        mov     a, _SPIRDAT     ; Throw away one extra dummy byte
rx_complete_0:
        setb    _RF_CSN         ; End SPI transaction

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

rx_ack:
        mov     a, _ack_len
        jz      no_ack                                  ; Skip the ACK entirely if empty
        mov     _ack_len, #RF_ACK_LEN_EMPTY

        clr     _RF_CSN                                 ; Begin SPI transaction
        mov     _SPIRDAT, #RF_CMD_W_ACK_PAYLD           ; Start sending ACK packet
        mov     R_TMP, #_ack_data
        mov     R_INPUT, a                              ; Packet length

3$:     mov     _SPIRDAT, @R_TMP
        inc     R_TMP
        SPI_WAIT                                        ; RX dummy byte
        mov     a, _SPIRDAT
        djnz    R_INPUT, 3$

        SPI_WAIT                                        ; RX last dummy byte
        mov     a, _SPIRDAT
        setb    _RF_CSN                                 ; End SPI transaction
no_ack:

        ;--------------------------------------------------------------------
        ; Cleanup
        ;--------------------------------------------------------------------

        mov     _vram_dptr, _DPL1
        mov     _vram_dptr+1, _DPH1
        pop     psw
        pop     _DPH1
        pop     _DPL1
        pop     _DPS
        pop     dph
        pop     dpl
        pop     acc
        reti
        
    __endasm ;
}

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

        /*
         * XXX: Hardcoded cube addresses, for testing only
         */
#ifdef CUBE_ADDR
        2, RF_CMD_W_REGISTER | RF_REG_RF_CH,          0x02,
        6, RF_CMD_W_REGISTER | RF_REG_TX_ADDR,        CUBE_ADDR, 0xe7, 0xe7, 0xe7, 0xe7,
        6, RF_CMD_W_REGISTER | RF_REG_RX_ADDR_P0,     CUBE_ADDR, 0xe7, 0xe7, 0xe7, 0xe7,
#endif

        0,
    };

    RF_CKEN = 1;                        // Radio clock running
    radio_transfer_table(table);        // Send initialization commands
    IEN_RF = 1;                         // Interrupt enabled
    RF_CE = 1;                          // Receiver enabled
}


uint8_t radio_get_cube_id(void)
{
    /*
     * XXX: This is temporary, until we have a real pairing mechanism.
     *      Our cube will be identified by radio address and channel, but
     *      also by an ID between 0 and 31. This will eventually come from
     *      the master cube, but for now we take it from the LSB of the
     *      radio address. That can be set at compile time with CUBE_ADDR,
     *      or it could be provided as a hardware default by the simulator.
     */

    uint8_t id;

    RF_CSN = 0;

    SPIRDAT = RF_CMD_R_REGISTER | RF_REG_TX_ADDR;
    SPIRDAT = 0;
    while (!(SPIRSTAT & SPI_RX_READY));
    SPIRDAT;
    while (!(SPIRSTAT & SPI_RX_READY));
    id = SPIRDAT;

    RF_CSN = 1;

    return id;
}
