/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * Thundercracker cube firmware
 *
 * Micah Elizabeth Scott <micah@misc.name>
 * Copyright <c> 2011 Sifteo, Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
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
#include "lcd.h"
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
#define R_SHIFT         r4
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

extern uint16_t fls_lut[FLS_LUT_SIZE];
extern uint8_t fls_state;
extern uint8_t fls_tail;

// State-specific temporaries
extern uint8_t fls_st[5];
__bit fls_bit0;
__bit fls_bit1;
__bit fls_bit2;

/*
 * State transition macro
 */

#pragma sdcc_hash +

// Value of a state, by label
#define STATE(_label)       #(_label - _flash_state_fn)

// Return to a label on the next loop iteration
#define NEXT(_label)                                            __endasm; \
    __asm mov   _fls_state, STATE(_label)                       __endasm; \
    __asm ajmp  flash_loop                                      __endasm; \
    __asm

// Return to the current state on the next loop iteration
#define AGAIN()                                                 __endasm; \
    __asm ajmp  flash_loop                                      __endasm; \
    __asm


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
        acall   _flash_dequeue
        mov     R_PTR, _fls_st+0
        mov     @R_PTR, a
        inc     _fls_st+0
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
 *    The low 4 bits of A are an index into the LUT.
 *
 *    Convert that into a pointer to the low byte, and store
 *    that in A and R_PTR.
 */
static void flash_addr_lut() __naked
{
    __asm
        anl     a, #0xF
        rl      a
        add     a, #_fls_lut
        mov     R_PTR, a
        ret
    __endasm ;
}


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
        mov     r1, #(FLS_LUT_SIZE * 2)
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
        ; We acknowledge a reset by toggling NB1_FLAG_FLS_RESET.

        xrl     (_ack_data + RF_ACK_NEIGHBOR + 1), #(NB1_FLAG_FLS_RESET)
        orl     _ack_bits, #RF_ACK_BIT_HWID

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
        jz      3$                          ; Return if FIFO is empty
        jnc     4$                          ; Handle wrap
        add     a, #FLS_FIFO_SIZE
4$:     mov     R_BYTE_COUNT, a             ; Save byte count

        setb    _global_busy_flag           ; System is definitely not idle now

        mov     a, R_DEADLINE               ; Check for deadline
        cjne    a, _sensor_tick_counter, 2$
3$:     ret
2$:

        ; Branch to state handler. All of our states are of the form flst_*
        ; NOTE dyn_branch fls_j flst_

        mov     a, _fls_state
fls_j:  jmp     @a+dptr 

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
 *
 *    Warning: This is another almost-full 8-bit state machine. Be
 *             careful when adding new code here, and be sure to check
 *             the state offsets afterward.
 */

static void flash_state_fn() __naked
{
    __asm
        ;--------------------------------------------------------------------
        ; Opcode Lookup Table
        ;--------------------------------------------------------------------

        ; NOTE dyn_branch flo_j flop_

flop_0: sjmp    op_lut1         ; 0x00
flop_1: sjmp    op_lut16        ; 0x20
flop_2: sjmp    op_tile_p0      ; 0x40
flop_3: sjmp    op_tile_p1_r4   ; 0x60
flop_4: sjmp    op_tile_p2_r4   ; 0x80
flop_5: sjmp    op_tile_p4_r4   ; 0xa0
flop_6: sjmp    op_tile_p16     ; 0xc0
flop_7: sjmp    op_special_t    ; 0xe0

        ;--------------------------------------------------------------------
        ; Opcode Dispatch (DEFAULT STATE)
        ;--------------------------------------------------------------------

flst_opcode:

        acall   _flash_dequeue      ; Consume opcode byte
        swap    a                   ; Directly convert to an offset in our LUT
        anl     a, #0x0E
flo_j:  jmp     @a+dptr

        ;--------------------------------------------------------------------
        ; Opcode: LUT1
        ;--------------------------------------------------------------------

op_lut1:

        mov     a, R_BYTE
        acall   _flash_addr_lut         ; Point to LUT entry from opcode argument
        mov     _fls_st+0, a

        NEXT(flst_1)
flst_1: acall   _flash_dequeue_st1p     ; Store low byte
        NEXT(flst_2)
flst_2: acall   _flash_dequeue_st1p     ; Store high byte

flst_opcode_n:
        NEXT(flst_opcode)

        ;--------------------------------------------------------------------
        ; Opcode: TILE_P0
        ;--------------------------------------------------------------------

        ; No additional data necessary to write the entire tile.

op_tile_p0:
        ajmp    _flash_tile_p0

        ;--------------------------------------------------------------------
        ; Opcode: TILE_P*_R4
        ;--------------------------------------------------------------------

op_tile_p1_r4:
        clr     _fls_bit0
        clr     _fls_bit1
        sjmp    op_tile_r4

op_tile_p2_r4:
        setb    _fls_bit0
        clr     _fls_bit1
        sjmp    op_tile_r4

op_tile_p4_r4:
        clr     _fls_bit0
        setb    _fls_bit1

op_tile_r4:
        acall   _flash_tile_init
        NEXT(flst_3)
flst_3: ajmp    _flash_tile_r4

        ;--------------------------------------------------------------------
        ; Opcode: TILE_P16
        ;--------------------------------------------------------------------

op_tile_p16:
        acall   _flash_tile_init
        NEXT(flst_4)
flst_4: ajmp    _flash_tile_p16

        ;--------------------------------------------------------------------
        ; Trampolines
        ;--------------------------------------------------------------------

op_special_t:
        sjmp    op_special

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
        NEXT(flst_5)
flst_5: acall   _flash_dequeue_st1p
        NEXT(flst_6)
flst_6: acall   _flash_dequeue_st1p
        mov     _fls_st, #_fls_lut          ; Write to beginning of LUT
        NEXT(flst_7)
flst_7:

        ; In this state, we iterate over the LUT, consuming bytes any time
        ; there is a corresponding bit set in the bitmap. 

        mov     a, _fls_st                  ; Exit when we reach the end of the LUT
        cjne    a, #_fls_lut+32, 4$
        ajmp    flst_opcode
4$:
        acall   _flash_rr16                 ; 16-bit right rotate into C
        jc      5$

        ; C=0, skip this bit.
        inc     _fls_st
        inc     _fls_st
        AGAIN()

        ; C=1, read two bytes then back

5$:     acall   _flash_dequeue_st1p
        NEXT(flst_8)
flst_8: acall   _flash_dequeue_st1p
        NEXT(flst_7)

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

        NEXT(flst_9)
flst_9: acall   _flash_dequeue
        anl     a, #0xfe                ; Must keep LSB clear
        mov     _flash_addr_lat1, a

        NEXT(flst_A)
flst_A: acall   _flash_dequeue
        rrc     a
        mov     _flash_addr_a21, c
        clr     c                       ; Keep LSB clear here too
        rlc     a
        mov     _flash_addr_lat2, a

        ajmp    flst_opcode_n
op_address_end:

        ;---------------------------------
        ; Special symbol: QUERY_CRC
        ;---------------------------------

        cjne    a, #FLS_OP_QUERY_CRC, op_query_crc_end

        NEXT(flst_B)
flst_B: acall   _flash_dequeue
        orl     a, #QUERY_ACK_BIT       ; Ensure query bit is set
        mov     _radio_query, a

        NEXT(flst_C)
flst_C: ajmp    _flash_query_crc

op_query_crc_end:

        ;---------------------------------
        ; Special symbol: CHECK_QUERY
        ;---------------------------------

        cjne    a, #FLS_OP_CHECK_QUERY, op_check_query_end

        NEXT(flst_D)
flst_D: acall   _flash_dequeue
        mov     _fls_st+0, a            ; Save byte count

        NEXT(flst_E)
flst_E: ajmp    _flash_check_query

op_check_query_end:

        ;---------------------------------

        ; Unrecognized special symbol. Ignore it...
        ; Also use this as a state which always loops back to itself (hang)

flst_hang:
        AGAIN();

    __endasm ;
}


/*
 * flash_query_crc --
 *
 *    Implements the FLS_OP_QUERY_CRC special opcode.
 *
 *    This op takes two byte-long arguments:
 *
 *     - A query ID (7 bits) which is sent back over the radio
 *       (Already stowed in radio_query[0], with QUERY_ACK_BIT set)
 *
 *     - Count of 16-tile blocks to process. Zero is interpreted
 *       as 256 blocks. (Sitting in the FIFO on entry)
 *
 *    Generates a 16-byte result. This is pushed into the radio
 *    ACK buffer, with the one-byte query ID prepended.
 */

static void flash_query_crc() __naked
{
    __asm

        ;--------------------------------------------------------------------
        ; Setup
        ;--------------------------------------------------------------------

        mov     c, _flash_addr_a21
        mov     _i2c_a21_target, c
        lcall   _i2c_a21_wait

        acall   _flash_dequeue      ; Store block count
        mov     R_COUNT1, a

        mov     _CCPDATIB, #0x84    ; Generator polynomial (See tools/gfm.py)

        mov     R_PTR, #(_radio_query+1)
        clr     a
1$:     mov     @R_PTR, a           ; Zero the CRC buffer
        inc     R_PTR
        cjne    R_PTR, #(_radio_query+17), 1$

        mov     CTRL_PORT, #CTRL_IDLE

        ;--------------------------------------------------------------------
        ; One allocation block (16 tiles)
        ;--------------------------------------------------------------------

2$:
        mov     ADDR_PORT, _flash_addr_lat2
        mov     CTRL_PORT, #(CTRL_IDLE | CTRL_FLASH_LAT2)
        mov     ADDR_PORT, _flash_addr_lat1
        mov     CTRL_PORT, #(CTRL_IDLE | CTRL_FLASH_LAT1)

        mov     ADDR_PORT, #0               ; Initial sampling point
        mov     _CCPDATIA, #0xFF            ; Initial conditions for CRC

        mov     R_PTR, #(_radio_query+1)    ; Init output pointer / counter

        ;--------------------------------------------------------------------
        ; One tile
        ;--------------------------------------------------------------------

3$:
        ; Four rounds of CRC + addressing feedback, for every tile.
        ; This CRCs the byte pointed to by ADDR_PORT using our GF
        ; multiplier, then uses the upper bits of the intermediate
        ; CRC result as the sampling point for the next round.

        mov     CTRL_PORT, #CTRL_FLASH_OUT

        mov     a, BUS_PORT         ; Load flash data byte
        xrl     a, _CCPDATO         ; XOR data byte with multiplier output
        mov     ADDR_PORT, a        ; Use new CRC as feedback for sampling point
        mov     _CCPDATIA, a        ; Save updated CRC

        mov     a, BUS_PORT         ; Load flash data byte
        xrl     a, _CCPDATO         ; XOR data byte with multiplier output
        mov     ADDR_PORT, a        ; Use new CRC as feedback for sampling point
        mov     _CCPDATIA, a        ; Save updated CRC

        mov     a, BUS_PORT         ; Load flash data byte
        xrl     a, _CCPDATO         ; XOR data byte with multiplier output
        mov     ADDR_PORT, a        ; Use new CRC as feedback for sampling point
        mov     _CCPDATIA, a        ; Save updated CRC

        mov     a, BUS_PORT         ; Load flash data byte
        xrl     a, _CCPDATO         ; XOR data byte with multiplier output
        mov     _CCPDATIA, a        ; Save updated CRC

        mov     R_TMP0, a           ; Remember it as the next sampling point
        xrl     a, @R_PTR           ; XOR with the output buffer
        mov     @R_PTR, a

        mov     a, _flash_addr_lat1 ; Advance to next tile
        add     a, #2
        mov     _flash_addr_lat1, a
        mov     ADDR_PORT, a
        mov     CTRL_PORT, #(CTRL_FLASH_OUT | CTRL_FLASH_LAT1)

        jnz     4$                  ; Handle lat1 overflow if necessary
        inc     _flash_addr_lat2
        inc     _flash_addr_lat2
4$:

        mov     ADDR_PORT, R_TMP0   ; Now go to the next sampling point

        inc     R_PTR               ; Loop until we finish the allocation block
        cjne    R_PTR, #(_radio_query+17), 3$

        djnz    R_COUNT1, 2$        ; Loop over all blocks

        ;--------------------------------------------------------------------
        ; Query response
        ;--------------------------------------------------------------------

        mov     CTRL_PORT, #CTRL_IDLE

        mov     r0, #17             ; Send back 17-byte response
        lcall   _radio_ack_query

        ajmp    flst_opcode_n       ; Done, next opcode.

    __endasm ;
}


/*
 * flash_check_query --
 *
 *    Implements the FLS_OP_CHECK_QUERY special opcode.
 *
 *    This op takes two byte-long arguments:
 *
 *     - A byte count, stowed in fls_st[0]
 *     - Bytes to compare with the query buffer
 *
 *    If the supplied bytes match the query buffer, this
 *    command acts like a no-op. If they do not match,
 *    it is an error and this command will lock the
 *    state machine in an infinite loop after completing.
 *
 *    (This error state can be tested by sending a NOP
 *    command and waiting for the lack of acknowledgment)
 */

static void flash_check_query() __naked
{
    __asm

        mov     a, R_BYTE_COUNT         ; Wait until we have the whole command
        clr     c
        subb    a, _fls_st+0
        jnc     1$
        AGAIN()
1$:

        mov     R_COUNT2, #0            ; Keep track of mismatches in R_COUNT2
        mov     R_TMP0, #_radio_query   ; Loop over the query buffer
2$:
        acall   _flash_dequeue          ; Clobbers R_PTR, R_BYTE
        xrl     a, @R_TMP0              ; XOR with command byte,
        orl     a, R_COUNT2             ;   OR difference with R_COUNT2
        mov     R_COUNT2, a

        inc     R_TMP0                  ; Next byte
        djnz    _fls_st+0, 2$

        mov     a, R_COUNT2             ; Check result
        jz      3$

        NEXT(flst_hang)                 ; Failed, enter hang state
3$:     ajmp    flst_opcode_n           ; Succeeded, back to opcode state

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
        mov     a, R_BYTE
        acall   _flash_addr_lut     ; Point to LUT entry from opcode argument

        ; Output one whole tile (4 buffers, 16 words per buffer) with the same color.

        mov     R_COUNT1, #4
1$:     acall   _flash_buffer_begin
        mov     R_COUNT2, #16
2$:     acall   _flash_buffer_word
        djnz    R_COUNT2, 2$
        acall   _flash_buffer_commit
        djnz    R_COUNT1, 1$

        AGAIN()
    __endasm ;
}


/*
 * flash_tile_init --
 *
 *    Initialization for decompression state shared by all tile codes.
 *
 *    RLE-4
 *    -----
 *
 *    Here, we handle all of the similar 4-bit RLE codes. They only
 *    differ in the way our final nybbles are expanded after the RLE
 *    decompression stage. The state machine proper has already set
 *    fls_bit[1:0] to reflect this.
 *
 *    In all of these codes, the opcode argument is a count of the number
 *    of tiles to generate. Each of these tiles will be four programming
 *    buffers worth of uncompressed data. We need to decode 16 pixels
 *    at a time.
 *
 *    During RLE, we use the following temporaries:
 *
 *    fls_st[0]: Older stored RLE nybble
 *    fls_st[1]: Newer stored RLE nybble
 *    fls_st[2]: Count of remaining 16-pixel buffers
 *    fls_st[3]: Next buffered nybble (if fls_bit2 is set)
 *    fls_st[4]: Run length, number of times to repeat fls_st[1]
 *
 *    fls_bit[1:0] indicates which flavor of RLE we are dealing with:
 *
 *       00 = 1-bit
 *       01 = 2-bit
 *       10 = 4-bit
 *       11 = not allowed
 *
 *    Masked 16-bit
 *    -------------
 *
 *    This decoder needs less state. We can get away with sharing the same
 *    initialization code:
 *
 *    fls_st[0]: (Temporary)
 *    fls_st[2]: Count of remaining 16-pixel buffers
 */

static void flash_tile_init() __naked
{
    __asm

        ; Initialize RLE state to something that will never match.

        clr     a
        clr     _fls_bit2       ; No buffered nybble
        mov     _fls_st+4, a    ; Not in a run
        mov     _fls_st+0, a    ; Init st[0] (older) to 0x00
        cpl     a
        mov     _fls_st+1, a    ; Init st[1] (newer) to 0xFF

        ; Convert our tile count to a buffer count, and store that

        mov     a, R_BYTE
        anl     a, #0x1F        ; Ignore opcode part
        inc     a               ; Count is stored as N-1
        rl      a               ; Each tile has four buffers
        rl      a
        mov     _fls_st+2, a

fls_ret:
        ret
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
        add     a, #(256 - FLS_MIN_TILE_P16)
        jnc     fls_ret

        ; We are guaranteed to produce 16 pixels as output, which always
        ; means we handle exactly two mask bytes with a variable number
        ; of input pixel bytes.
        ;
        ; We do this with two nested loops: R_COUNT1 counts mask bytes,
        ; and R_COUNT2 counts bits in our shift register. The shift register
        ; itself is in R_SHIFT.

        acall   _flash_buffer_begin

        mov     R_COUNT1, #2
1$:
        acall   _flash_dequeue  ; Next byte is 8-bit mask
        mov     R_SHIFT, a
        mov     R_COUNT2, #8
2$:
        mov     a, R_SHIFT      ; Shift out next bit
        rrc     a
        mov     R_SHIFT, a
        jnc     3$              ; Zero bit? Skip pixel load.

        mov     _fls_st+0, #(_fls_lut + 15*2)   ; Load pixel from FIFO into lut[15]
        acall   _flash_dequeue_st1p
        acall   _flash_dequeue_st1p
3$:

        mov     R_PTR, #(_fls_lut + 15*2)       ; Output pixel from lut[15]
        acall   _flash_buffer_word  

        djnz    R_COUNT2, 2$
        djnz    R_COUNT1, 1$
        sjmp    flr4_buffer_done

    __endasm ;
}


/*
 * flash_tile_r4 --
 *
 *    Tiles at 1/2/4 bit per pixel, with 4-bit RLE.
 *
 *    This function implements the state we're in after receiving a
 *    relevant opcode, after the RLE state-specific data has been
 *    initialized.
 *
 *    We check up-front to guarantee that we have enough data
 *    to decompress a full 16-pixel buffer. If not, we return
 *    back to the firmware's main loop.
 *
 *    The worst-case amount of data we need for 16 pixels is 12 bytes.
 *    This happens if we use the 4-bit codec, and every pair of nybbles
 *    is followed by a repeat count of zero. We could come up with a
 *    tighter worst-case estimate for each variant of this codec, but
 *    12 bytes still gives us plenty of room to keep the FIFO full
 *    ahead of us, so no need to bother. Also, the 4-bit codec is usually
 *    the most common, so we might as well optimize for it.
 *
 *    If we do continue on to program a buffer of pixels to the flash,
 *    we should return via AGAIN(), so we can test whether we can
 *    program another buffer before exiting all the way back to
 *    the main loop.
 */

static void flash_tile_r4() __naked
{
    __asm

        ; Wait until we have 12 bytes ahead of us in the FIFO.

        mov     a, R_BYTE_COUNT
        add     a, #(256 - FLS_MIN_TILE_R4)
        jnc     fls_ret

        ; Guaranteed to produce 16 pixels now. Begin the buffer,
        ; and use R_COUNT1 to count the number of pixels left.
        ; Note that 16 pixels is always a multiple of 1 nybble,
        ; in any of our supported bit depths.

        acall   _flash_buffer_begin
        mov     R_COUNT1, #16
flr4_pixel_loop:

        ;---------------------------------
        ; Assemble Nybble Runs
        ;---------------------------------

        ; fls_st[4] tells us how many copies of the nybble in fls_st[1]
        ; need to be unpacked. If fls_st[4] is zero, we need to un-RLE
        ; another nybble. If not, go straight to the output stage below

        mov     a, _fls_st+4
        jnz     flr4_output

        ; Now, dequeue another nybble. This may come from the FIFO
        ; itself or from our nybble buffer. Either way, it ends
        ; up in the low nybble of R_BYTE.

        mov     R_BYTE, _fls_st+3       ; Assume nybble is buffered
        cpl     _fls_bit2               ; Negate buffer status
        jnb      _fls_bit2, 1$          ; Done if we had a nybble indeed

        acall   _flash_dequeue          ; Refill our buffer
        swap    a                       ; Save the most significant nybble for later
        mov     _fls_st+3, a
1$:

        ; Is this a run count or a normal nybble? If the last two RLE bytes
        ; match, this is a run count and we should interpret it as such.
        ; Otherwise, it is a run of one nybble.

        mov     a, R_BYTE               ; Literal masked nybble
        anl     a, #0xF
        mov     _fls_st+4, a            ; Store it as a run count for now
        mov     a, _fls_st+0            ; Look at the previous two nybbles.
        xrl     a, _fls_st+1            ;   Z = run
        jnz     2$                      ; Branch if not a run

        orl     _fls_st+1, #0xF0        ; Prevent re-triggering the run detector
        sjmp    flr4_pixel_loop         ; Go back up, to check for zero-len runs

2$:
        mov     _fls_st+0, _fls_st+1    ; Shift nybble into our run buffer
        mov     _fls_st+1, _fls_st+4
        mov     _fls_st+4, #1           ; Single-nybble run

        ; Fall through to output the single-nybble run...

        ;---------------------------------
        ; Output Nybble Runs
        ;---------------------------------

flr4_output:

        dec     _fls_st+4               ; Pre-decrement the run counter

        jb      _fls_bit1, flr4_out_4bpp
        jb      _fls_bit0, flr4_out_2bpp

        ; Fall through to 1bpp...

        ;---------------------------------
        ; Output nybble - 1 bit per pixel
        ;---------------------------------

flr4_out_1bpp:

        mov     R_COUNT2, #4    ; Pixel count
        mov     a, _fls_st+1    ; Loop begins with shift register in A
1$:
        mov     R_BYTE, a       ; Output least significant pixel
        anl     a, #1
        acall   _flash_addr_lut
        acall   _flash_buffer_word

        mov     a, R_BYTE       ; Next pixel
        rr      a
        dec     R_COUNT1
        djnz    R_COUNT2, 1$

        sjmp    flr4_next_pixel

        ;---------------------------------
        ; Output nybble - 2 bit per pixel
        ;---------------------------------

flr4_out_2bpp:

        mov     R_COUNT2, #2    ; Pixel count
        mov     a, _fls_st+1    ; Loop begins with shift register in A
1$:
        mov     R_BYTE, a       ; Output least significant pixel
        anl     a, #3
        acall   _flash_addr_lut
        acall   _flash_buffer_word

        mov     a, R_BYTE       ; Next pixel
        rr      a
        rr      a
        dec     R_COUNT1
        djnz    R_COUNT2, 1$

        sjmp    flr4_next_pixel

        ;---------------------------------
        ; Output nybble - 4 bit per pixel
        ;---------------------------------

flr4_out_4bpp:

        mov     a, _fls_st+1    ; Whole nybble is color index
        acall   _flash_addr_lut
        acall   _flash_buffer_word

        dec     R_COUNT1

        ; Fall through to next...

        ;---------------------------------
        ; Next pixel loop
        ;---------------------------------

flr4_next_pixel:

        ; All counters have already been updated.
        ; If we still have more pixels to go, loop up,
        ; otherwise end this flash buffer.

        mov     a, R_COUNT1
        jnz     flr4_pixel_loop

        ; Done with this buffer. If we still have more,
        ; stay in this state. Otherwise, look for the next opcode.
        ; This code fragment is shared by P16.

flr4_buffer_done:

        acall   _flash_buffer_commit
        
        djnz    _fls_st+2, 1$
        mov     _fls_state, STATE(flst_opcode)
1$:     ajmp    flash_loop

    __endasm ;
}
