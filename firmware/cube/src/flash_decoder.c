/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * Thundercracker cube firmware
 *
 * M. Elizabeth Scott <beth@sifteo.com>
 * Copyright <c> 2011 Sifteo, Inc. All rights reserved.
 */

/*
 * This file is responsible for programming and erasing the flash
 * memory.  All program/erase operations come from the master in the
 * form of a "loadstream", a sequence of optimized opcodes that
 * instruct us on how to reconstruct the intended data in flash.
 *
 * Because flash programming is relatively slow, and because it can't
 * happen during graphics rendering, we buffer the incoming loadstream
 * in iram. The radio ISR will write incoming commands to this
 * lockless FIFO, which we process in-between frames.
 *
 * We have an end-to-end flow control mechanism in which the master
 * receives feedback in our ACK packet, indicating how many bytes
 * we've fully processed. It uses this to determine how much space is
 * available in the FIFO.
 *
 * The FIFO is relatively small; just large enough to give us time to
 * re-start the radio streaming when the buffer has room, without it
 * becoming empty in the mean time. This means that the required
 * buffer space is proportional to radio latency, and if we can keep
 * the radio latency low, we can keep the buffer size to a minimum.
 *
 * We use buffered flash programming, meaning that we generate 32 bytes
 * in one burst, then wait for it all to program. We must generate
 * exactly 32 bytes, so we handle this by waiting until we have enough
 * data in our input FIFO to yield 32 bytes of output, even with
 * worst-case compression ratios.
 */

#include "flash.h"
#include "cube_hardware.h"
#include "radio.h"
#include "sensors.h"
#include "main.h"
#include <protocol.h>

/*
 * Register usage during state machine.
 *
 * Other invariants:
 *    - DPTR always contains the origin of the state machine
 */

#define R_TMP0          r0
#define R_PTR           r1      // Used as input by flash_buffer_word()
#define R_COUNT1        r2
#define R_COUNT2        r3

#define R_BYTE          r5
#define R_BYTE_COUNT    r6
#define R_DEADLINE      r7

/*
 * To preserve interactivity even if our FIFO is staying full, we limit
 * the total amount of time we can spend on flash programming without
 * coming up for air. To measure this in a low-overhead way, we use
 * the existing low-frequency timer that we run for sensor polling.
 *
 * The actual max duration of a timeslice varies between this many ticks
 * and this many ticks minus one, since we don't align ourselves to the
 * tick clock when starting.
 */
 
#define FLASH_TIMESLICE_TICKS  2

/*
 * FIFO buffer, written by the radio ISR or testjig interface
 */
 
volatile uint8_t __idata flash_fifo[FLS_FIFO_SIZE];
volatile uint8_t flash_fifo_head;                        // Pointer to next write location
volatile __bit flash_reset_request;                      // Pending reset?

/*
 * Loadstream codec state
 */

static __idata uint16_t fls_lut[FLS_LUT_SIZE + 1];      // LUT proper, plus P16 value
static uint8_t fls_state;                               // 8-bit state machine offset
static uint8_t fls_tail;                                // Current read location in the FIFO
static uint8_t fls_st[3];                               // State-specific temporaries

/*
 * State transition macro
 */

#pragma sdcc_hash +

// Value of a state, by label
#define STATE(_label)       #(_label - _flash_state_fn)

// Return to a label on the next loop iteration
#define NEXT(_label)                                            __endasm; \
    __asm mov   _fls_state, STATE(_label)                       __endasm; \
    __asm ljmp  flash_loop                                      __endasm; \
    __asm

// Return to the current state on the next loop iteration
#define AGAIN()                                                 __endasm; \
    __asm ljmp  flash_loop                                      __endasm; \
    __asm


/*
 * flash_init --
 *
 *    Handle power-on initialization _or_ a "flash reset" command.
 */

void flash_init(void)
{
    __asm

        ; Zero out the whole LUT

        mov     r0, #_fls_lut
        mov     r1, #((FLS_LUT_SIZE + 1) * 2)
        clr     a
1$:     mov     @r0, a
        inc     r0
        djnz    r1, 1$

        ; Reset the state machine

        mov     _fls_state, STATE(flst_opcode)
        clr     _flash_reset_request

        ; Reset FIFO pointers

        mov     a, #_flash_fifo
        mov     _fls_tail, a
        mov     _flash_fifo_head, a

        ; Flash reset must send back a full packet, including the HWID.
        ; We acknowledge a reset as if it were one data byte.

        inc     (_ack_data + RF_ACK_FLASH_FIFO)
        orl     _ack_bits, #RF_ACK_BIT_HWID

    __endasm ;
}


/*
 * flash_dequeue --
 *
 *    Assembly-callable utility for dequeueing one byte from the FIFO.
 *    The space used by this byte is available immediately for reuse
 *    by the radio ISR.
 *
 *    Returns the next byte in R_BYTE *and* in 'a'.
 *    Trashes R_PTR.
 *    Does NOT update R_BYTE_COUNT.
 */

static void flash_dequeue() __naked
{
    __asm
        mov     R_PTR, _fls_tail                            ; Read next byte from FIFO
        mov     a, @R_PTR

        inc     (_ack_data + RF_ACK_FLASH_FIFO)             ; Acknowledge ASAP
        orl     _ack_bits, #RF_ACK_BIT_FLASH_FIFO

        inc     R_PTR                                       ; Advance tail pointer
        cjne    R_PTR, #(_flash_fifo + FLS_FIFO_SIZE), 1$
        mov     R_PTR, #_flash_fifo                         ; Wrap
1$:     mov     _fls_tail, R_PTR

        mov     R_BYTE, a                                   ; Also return byte in R_BYTE
        ret
    __endasm ;
}


/*
 * flash_dequeue_st1p --
 *
 *    Dequeue the next byte, copy it to the byte pointed to by
 *    fls_st[0], and increment fls_st[0].
 *
 *    Trashes R_PTR.
 */

static void flash_dequeue_st1p() __naked
{
    __asm
        lcall   _flash_dequeue
        mov     R_PTR, _fls_st
        mov     @R_PTR, a
        inc     _fls_st
        ret
    __endasm ;
}


/*
 * flash_rr16 --
 *
 *     16-bit right rotate: 0 -> fls_st[2:1] -> C.
 */

static void flash_rr16() __naked
{
    __asm
        clr     c
        mov     a, _fls_st+2
        rrc     a
        mov     _fls_st+2, a
        mov     a, _fls_st+1
        rrc     a
        mov     _fls_st+1, a
        ret
    __endasm ;
}


/*
 * flash_addr_lut --
 *
 *    The low 4 bits of the R_BYTE are an index into the LUT.
 *    Convert that into a pointer to the low byte, and store
 *    that in fls_st[0] and R_PTR.
 */
static void flash_addr_lut() __naked
{
    __asm
        mov     a, R_BYTE
        anl     a, #0xF
        rl      a
        add     a, #_fls_lut
        mov     _fls_st, a
        mov     R_PTR, a
        ret
    __endasm ;
}


/*
 * flash_handle_fifo --
 *
 *    Entry point for flash loadstream processing. Called periodically by
 *    the main loop. Can run for a while, but should be bounded in duration.
 */

void flash_handle_fifo() __naked
{
    __asm

        ; Handle reset requests with a tailcall to init

        jb      _flash_reset_request, _flash_init

        ; Set up time deadline

        mov     a, _sensor_tick_counter
        add     a, #FLASH_TIMESLICE_TICKS
        mov     R_DEADLINE, a

        ; Keep DPTR loaded to the state machine address

        mov     dptr, #_flash_state_fn

        ; Individual states can jump to flash_loop to transition to the
        ; next state, or they can return to abort processing for now.
        ; This does not automatically dequeue bytes, since some states may
        ; want to retry later if R_BYTE_COUNT is too small.

flash_loop:

        mov     a, _flash_fifo_head         ; Start loading FIFO state
        clr     c
        subb    a, _fls_tail                ; Calculate number of bytes available
        anl     a, #(FLS_FIFO_SIZE - 1)
        jz      3$                          ; Return if FIFO is empty
        mov     R_BYTE_COUNT, a             ; Otherwise, save byte count

        setb    _global_busy_flag           ; System is definitely not idle now

        mov     a, R_DEADLINE               ; Check for deadline
        cjne    a, _sensor_tick_counter, 2$
3$:     ret
2$:

        ; Branch to state handler

        mov     a, _fls_state
        jmp     @a+dptr 

    __endasm ;
}


/*
 * flash_state_fn --
 *
 *    This is an 8-bit state machine, called by flash_handle_fifo.
 *
 *    Each state can take several actions:
 *
 *      - Writing a complete buffer of 16 pixels to flash
 *      - Calling flash_dequeue() to get another byte from the FIFO
 *      - Jumping to flash_loop to re-enter the state machine
 *      - Returning, to go back to the main loop
 *
 *    States are guaranteed to be called with at least one byte available
 *    in the FIFO. See the register declarations at the top of the file,
 *    for registers which are maintained across state invocations.
 */

static void flash_state_fn() __naked
{
    __asm
        ;--------------------------------------------------------------------
        ; Opcode Lookup Table
        ;--------------------------------------------------------------------

        sjmp    op_lut1         ; 0x00
        sjmp    op_lut16        ; 0x20
        sjmp    op_tile_p0      ; 0x40
        sjmp    op_tile_p1_r4   ; 0x60
        sjmp    op_tile_p2_r4   ; 0x80
        sjmp    op_tile_p4_r4   ; 0xa0
        sjmp    op_tile_p16     ; 0xc0
        sjmp    op_special      ; 0xe0

        ;--------------------------------------------------------------------
        ; Opcode Dispatch (DEFAULT STATE)
        ;--------------------------------------------------------------------

flst_opcode:

        lcall   _flash_dequeue      ; Consume opcode byte
        swap    a                   ; Directly convert to an offset in our LUT
        anl     a, #0x0E
        jmp     @a+dptr

        ;--------------------------------------------------------------------
        ; Opcode: LUT1
        ;--------------------------------------------------------------------

op_lut1:

        lcall   _flash_addr_lut         ; Point to LUT entry from opcode argument
        NEXT(1$)
1$:     lcall   _flash_dequeue_st1p     ; Store low byte
        NEXT(2$)
2$:     lcall   _flash_dequeue_st1p     ; Store high byte

flst_opcode_n:
        NEXT(flst_opcode)

        ;--------------------------------------------------------------------
        ; Opcode: TILE_P0
        ;--------------------------------------------------------------------

        ; No additional data necessary to write the entire tile.

op_tile_p0:
        ljmp    _flash_tile_p0

        ;--------------------------------------------------------------------
        ; Opcode: TILE_P1_R4
        ;--------------------------------------------------------------------

op_tile_p1_r4:

        ; One-time setup for this opcode


        NEXT(1$)
1$:     ljmp    _flash_tile_p1_r4

        ;--------------------------------------------------------------------
        ; Opcode: TILE_P2_R4
        ;--------------------------------------------------------------------

op_tile_p2_r4:

        ; One-time setup for this opcode


        NEXT(1$)
1$:     ljmp    _flash_tile_p2_r4

        ;--------------------------------------------------------------------
        ; Opcode: TILE_P4_R4
        ;--------------------------------------------------------------------

op_tile_p4_r4:

        ; One-time setup for this opcode


        NEXT(1$)
1$:     ljmp    _flash_tile_p4_r4

        ;--------------------------------------------------------------------
        ; Opcode: TILE_P16
        ;--------------------------------------------------------------------

op_tile_p16:

        ; One-time setup for this opcode

        NEXT(1$)
1$:     ljmp    _flash_tile_p16

        ;--------------------------------------------------------------------
        ; Opcode: SPECIAL
        ;--------------------------------------------------------------------

op_special:
        mov     a, R_BYTE

        ;---------------------------------
        ; Special symbol: ADDRESS
        ;---------------------------------

        cjne    a, #FLS_OP_ADDRESS, op_address_end

        mov     _flash_addr_low, #0

        NEXT(1$)
1$:     lcall   _flash_dequeue
        anl     a, #0xfe                ; Must keep LSB clear
        mov     _flash_addr_lat1, a

        NEXT(2$)
2$:     lcall   _flash_dequeue
        rrc     a
        mov     _flash_addr_a21, c
        clr     c                       ; Keep LSB clear here too
        rlc     a
        mov     _flash_addr_lat2, a

        ljmp    flst_opcode_n
op_address_end:

        ;---------------------------------

        ; Unrecognized special symbol. Ignore it...
        AGAIN();

        ;--------------------------------------------------------------------
        ; Opcode: LUT16
        ;--------------------------------------------------------------------

op_lut16:

        ; The next two bytes are a 16-bit vector indicating which LUT entries
        ; are being set. Entries are numbered from LSB to MSB. A zero bit
        ; means we leave it alone, a one bit means that a new value for that
        ; LUT entry is included.
        ;
        ; In these states, we keep the 16-bit shift register in fls_st[2:1],
        ; with the write pointer in fls_st[0].

        mov     _fls_st, #_fls_st+1
        NEXT(1$)
1$:     lcall   _flash_dequeue_st1p
        NEXT(2$)
2$:     lcall   _flash_dequeue_st1p
        mov     _fls_st, #_fls_lut          ; Write to beginning of LUT
        NEXT(3$)
3$:

        ; In this state, we iterate over the LUT, consuming bytes any time
        ; there is a corresponding bit set in the bitmap. 

        mov     a, _fls_st                  ; Exit when we reach the end of the LUT
        cjne    a, #_fls_lut+32, 4$
        ljmp    flst_opcode
4$:
        lcall   _flash_rr16                 ; 16-bit right rotate into C
        jc      5$

        ; C=0, skip this bit.
        inc     _fls_st
        inc     _fls_st
        AGAIN()

        ; C=1, read two bytes then back to this state.

5$:     lcall   _flash_dequeue_st1p
        NEXT(6$)
6$:     lcall   _flash_dequeue_st1p
        NEXT(3$)

    __endasm ;
}


/*
 * flash_tile_p0 --
 *
 *    Output one solid-color tile, using the LUT entry from
 *    the current byte.
 */

static void flash_tile_p0() __naked
{
    __asm
        lcall   _flash_addr_lut     ; Point to LUT entry from opcode argument

        ; Output one whole tile (4 buffers, 16 words per buffer) with the same color.

        mov     R_COUNT1, #4
1$:     lcall   _flash_buffer_begin
        mov     R_COUNT2, #16
2$:     lcall   _flash_buffer_word
        djnz    R_COUNT2, 2$
        lcall   _flash_buffer_commit
        djnz    R_COUNT1, 1$

        AGAIN()
    __endasm ;
}


/*
 * flash_tile_p1_r4 --
 *
 *    Tiles at 1 bit per pixel, with 4-bit RLE.
 */

static void flash_tile_p1_r4() __naked
{
    __asm
    

        ; To decode a full 16 pixels, we require a worst-case of 4 bytes.
        ; Check this before every entry to the state function.

        mov     a, R_BYTE_COUNT
        anl     a, #0xFC
        jz      1$

        AGAIN()

    1$: ret

    __endasm ;
}


/*
 * flash_tile_p2_r4 --
 *
 *    Tiles at 2 bits per pixel, with 4-bit RLE.
 */

static void flash_tile_p2_r4() __naked
{
    __asm
    

        ; To decode a full 16 pixels, we require a worst-case of 8 bytes.
        ; Check this before every entry to the state function.

        mov     a, R_BYTE_COUNT
        anl     a, #0xF8
        jz      1$

        AGAIN()

    1$: ret

    __endasm ;
}


/*
 * flash_tile_p4_r4 --
 *
 *    Tiles at 4 bits per pixel, with 4-bit RLE.
 */

static void flash_tile_p4_r4() __naked
{
    __asm
    

        ; To decode a full 16 pixels, we require a worst-case of 16 bytes.
        ; Check this before every entry to the state function.

        mov     a, R_BYTE_COUNT
        anl     a, #0xF0
        jz      1$

        AGAIN()

    1$: ret

    __endasm ;
}


/*
 * flash_tile_p16 --
 *
 *    Tiles at 16 bits per pixel, with an 8-bit repeat mask for each 8 pixels.
 */

static void flash_tile_p16() __naked
{
    __asm


        ; To decode a full 16 pixels, we require a worst-case of 34 bytes.
        ; Check this before every entry to the state function.

        mov     a, R_BYTE_COUNT
        add     a, #(256-34)
        jnc     1$

        AGAIN()

1$:     ret

    __endasm ;
}


////////////////////////////////////////////////////////////////////////////CRUFT


#if 0

    case FLS_OP_TILE_P1_R4:
        counter = 64;
        ovl.rle1 = 0xFF;
        state = state_TILE_P1_R4;
        STATE_RETURN();

    case FLS_OP_TILE_P2_R4:
        counter = 64;
        ovl.rle1 = 0xFF;
        state = state_TILE_P2_R4;
        STATE_RETURN();

    case FLS_OP_TILE_P4_R4:
        counter = 64;
        ovl.rle1 = 0xFF;
        state = state_TILE_P4_R4;
        STATE_RETURN();
        
    case FLS_OP_TILE_P16:
        counter = 8;
        state = state_TILE_P16_MASK;
        STATE_RETURN();

    default:
        // Undefined opcode
        STATE_RETURN();
    }
}

static void state_TILE_P1_R4(void) __naked
{
    uint8_t nybble = byte & 0x0F;
    uint8_t runLength;
    nibIndex = 1;

    for (;;) {
        if (ovl.rle1 == ovl.rle2) {
            runLength = nybble;
            nybble = ovl.rle1 | swap(ovl.rle1);
            ovl.rle1 = 0xF0;
            
            if (!runLength)
                goto no_runs;
            
            runLength = rl(runLength);
            runLength = rl(runLength);
            counter -= runLength;
            
        } else {
            runLength = 4;
            counter -= 4;
            ovl.rle2 = ovl.rle1;
            ovl.rle1 = nybble;
        }

        do {
            uint8_t idx = nybble & 1;
            flash_program_word(lut.colors[idx].word);
            nybble = rr(nybble);
        } while (--runLength);

    no_runs:
        if (!counter || (counter & 0x80))
            if (opcode & FLS_ARG_MASK) {
                opcode--;
                counter += 64;
            } else {
                state = state_OPCODE;
                STATE_RETURN();
            }
        if (!nibIndex)
            STATE_RETURN();
        nibIndex = 0;
        nybble = byte >> 4;
    }
}
                
static void state_TILE_P2_R4(void) __naked
{
    uint8_t nybble = byte & 0x0F;
    uint8_t runLength;
    nibIndex = 1;
    
    for (;;) {
        if (ovl.rle1 == ovl.rle2) {
            runLength = nybble;
            nybble = ovl.rle1 | swap(ovl.rle1);
            ovl.rle1 = 0xF0;
            
            if (!runLength)
                goto no_runs;
            
            runLength = rl(runLength);
            counter -= runLength;
            
        } else {
            runLength = 2;
            counter -= 2;
            ovl.rle2 = ovl.rle1;
            ovl.rle1 = nybble;
        }
        
        do {
            uint8_t idx = nybble & 3;
            flash_program_word(lut.colors[idx].word);
            nybble = rr(nybble);
            nybble = rr(nybble);
        } while (--runLength);

    no_runs:
        if (!counter || (counter & 0x80))
            if (opcode & FLS_ARG_MASK) {
                opcode--;
                counter += 64;
            } else {
                state = state_OPCODE;
                STATE_RETURN();
            }
        if (!nibIndex)
            STATE_RETURN();
        nibIndex = 0;
        nybble = byte >> 4;
    }
}

static void state_TILE_P4_R4(void) __naked
{
    uint8_t nybble = byte & 0x0F;
    register uint8_t runLength;
    nibIndex = 1;

    for (;;) {
       if (ovl.rle1 == ovl.rle2) {
           counter -= (runLength = nybble);
           nybble = ovl.rle1;
           ovl.rle1 = 0xF0;
           
           if (!runLength)
               goto no_runs;

       } else {
           runLength = 1;
           counter --;
           ovl.rle2 = ovl.rle1;
           ovl.rle1 = nybble;
       }

       {
           register uint16_t color = lut.colors[nybble].word;
           do {
               flash_program_word(color);
           } while (--runLength);
       }
    
    no_runs:
       if (!counter || (counter & 0x80))
           if (opcode & FLS_ARG_MASK) {
               opcode--;
               counter += 64;
           } else {
               state = state_OPCODE;
               STATE_RETURN();
           }
       if (!nibIndex)
           STATE_RETURN();
       nibIndex = 0;
       nybble = byte >> 4;
    }
}

#define P16_EMIT_RUNS() {                       \
        do {                                    \
            if (ovl.rle1 & 1) {                 \
                state = state_TILE_P16_LOW;     \
                STATE_RETURN();                 \
            }                                   \
            flash_program_word(lut.p16.word);   \
            ovl.rle1 = rr(ovl.rle1);            \
        } while (--ovl.rle2);                   \
    }

#define P16_NEXT_MASK() {                       \
        if (--counter) {                        \
            state = state_TILE_P16_MASK;        \
        } else if (opcode & FLS_ARG_MASK) {     \
            opcode--;                           \
            counter = 8;                        \
            state = state_TILE_P16_MASK;        \
        } else {                                \
            state = state_OPCODE;               \
        }                                       \
    }

static void state_TILE_P16_MASK(void) __naked
{
    ovl.rle1 = byte;  // Mask byte for the next 8 pixels
    ovl.rle2 = 8;     // Remaining pixels in mask
    
    P16_EMIT_RUNS();
    P16_NEXT_MASK();
    STATE_RETURN();
}

static void state_TILE_P16_LOW(void) __naked
{
    lut.p16.low = byte;
    state = state_TILE_P16_HIGH;
    STATE_RETURN();
}

static void state_TILE_P16_HIGH(void) __naked
{
    lut.p16.high = byte;
    flash_program_word(lut.p16.word);

    if (--ovl.rle2) {
        // Still more pixels in this mask.
        ovl.rle1 = rr(ovl.rle1);

        P16_EMIT_RUNS();
    }

    P16_NEXT_MASK();
    STATE_RETURN();
}

#endif