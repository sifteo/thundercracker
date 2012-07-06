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
static uint8_t fls_st_1;                                // State-specific temporary 1
static uint8_t fls_st_2;                                // State-specific temporary 2
static uint8_t fls_tail;                                // Current read location in the FIFO


void flash_init(void)
{
    /*
     * Handle power-on initialization _or_ a "flash reset" command.
     */

    __asm

        ; Zero out the whole LUT

        mov     r0, #_fls_lut
        mov     r1, #((FLS_LUT_SIZE + 1) * 2)
        clr     a
1$:     mov     @r0, a
        inc     r0
        djnz    r1, 1$

        ; Reset the state machine

        mov     _fls_state, a
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


void flash_handle_fifo() __naked
{
    /*
     * Register usage during state machine:
     *
     *    R0 - temp
     *    R1 - temp
     *    R2 - temp
     *    R3 - temp
     *    R4 - temp
     *    R5 - Current byte
     *    R6 - Count of bytes available
     *    R7 - Deadline timer
     */

    __asm

        ; Handle reset requests with a tailcall to init

        jb      _flash_reset_request, _flash_init

        ; Set up time deadline

        mov     a, _sensor_tick_counter
        add     a, #FLASH_TIMESLICE_TICKS
        mov     r7, a

10$:                                        ; Loop over incoming bytes

        mov     a, _flash_fifo_head         ; Start loading FIFO state
        clr     c
        subb    a, _fls_tail                ; Calculate number of bytes available
        anl     a, #(FLS_FIFO_SIZE - 1)
        jz      3$                          ; Return if FIFO is empty
        mov     r6, a                       ; Otherwise, save byte count

        setb    _global_busy_flag           ; System is definitely not idle now

        mov     a, r7                       ; Check for deadline
        cjne    a, _sensor_tick_counter, 2$
3$:     ret
2$:

        ; Dequeue one more byte from the FIFO

        mov     r0, _fls_tail               ; Read next byte from FIFO
        mov     a, @r0
        mov     r5, a
        inc     r0                          ; Advance tail pointer
        cjne    r0, #(_flash_fifo + FLS_FIFO_SIZE), 4$
        mov     r0, #_flash_fifo            ; Wrap
4$:     mov     _fls_tail, r0

        ; Acknowledge receipt. After this point, the radio may resue this FIFO location.
    
        inc     (_ack_data + RF_ACK_FLASH_FIFO)
        orl     _ack_bits, #RF_ACK_BIT_FLASH_FIFO

        ; XXX

        mov     _DEBUG_REG, r6
        ret
    __endasm ;
}

#if 0
        mov     a, _fls_tail                ; Read next byte from FIFO
        add     a, #_flash_fifo
        mov     r0, a
        mov     a, @r0
        mov     r5, a

        mov     a, _flash_fifo_head
        clr     c
        subb    a, _fls_tail
        


    /*
     * Dequeue one byte from the FIFO.
     *
     * As soon as we increment flash_fifo_bytes, the master may
     * overwrite the location we just freed up in the FIFO buffer.
     */

//    byte = flash_fifo[fls_tail];
    fls_tail = (fls_tail + 1) & (FLS_FIFO_SIZE - 1);


#endif

////////////////////////////////////////////////////////////////////////////CRUFT


#if 0

/*
 * State machine.
 *
 * This is a state machine, in which each state is implemented as a
 * function chunk that we jump to. The state transition functions
 * process exactly one byte per invocation.
 */

static void state_OPCODE(void) __naked
{
    opcode = byte;
    switch (opcode & FLS_OP_MASK) {

    case FLS_OP_LUT1:
        state = state_LUT1_COLOR1;
        STATE_RETURN();

    case FLS_OP_LUT16:
        state = state_LUT16_VEC1;
        STATE_RETURN();

    case FLS_OP_TILE_P0:
        // Trivial solid-color tile, no repeats
        __asm
            mov   a, _byte
            anl   a, #0xF
            rl    a 
            add   a, #_lut
            mov   r0, a
            mov   DPL, @r0
            inc   r0
            mov   DPH, @r0

            mov   r0, #64
            1$:
            lcall _flash_program_word
            djnz  r0, 1$
        __endasm ;
        STATE_RETURN();
        
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

    case FLS_OP_SPECIAL:
        switch (opcode) {
            
        case FLS_OP_ADDRESS:
            state = state_ADDR_LOW;
            STATE_RETURN();

        default:
            // Reserved
            STATE_RETURN();
        }
        
    default:
        // Undefined opcode
        STATE_RETURN();
    }
}

static void state_ADDR_LOW(void) __naked
{
    flash_addr_low = 0;
    flash_addr_lat1 = byte & 0xFE;
    state = state_ADDR_HIGH;
    STATE_RETURN();
}

static void state_ADDR_HIGH(void) __naked
{
    /*
     * Note, we must not change LAT2 or A21 between start/end. The HAL assumes
     * LAT2 doesn't change unless a rollover occurs. The most space-efficient
     * way for us to change LAT2 is to end (finish the last op) then begin
     * (reprogram LAT2) again.
     */
    flash_program_end();
    flash_addr_lat2 = byte & 0xFE;
    flash_a21 = byte & 1;
    flash_need_autoerase = 1;
    flash_program_start();

    state = state_OPCODE;
    STATE_RETURN();
}

static void state_LUT1_COLOR1(void) __naked
{
    lut.colors[opcode & 0xF].low = byte;
    state = state_LUT1_COLOR2;
    STATE_RETURN();
}

static void state_LUT1_COLOR2(void) __naked
{
    lut.colors[opcode & 0xF].high = byte;
    state = state_OPCODE;
    STATE_RETURN();
}

static void state_LUT16_VEC1(void) __naked
{
    ovl.lutvec.low = byte;
    state = state_LUT16_VEC2;
    STATE_RETURN();
}

static void state_LUT16_VEC2(void) __naked
{
    ovl.lutvec.high = byte;
    counter = 0;
    state = state_LUT16_COLOR1;
    STATE_RETURN();
}

static void state_LUT16_COLOR1(void) __naked
{
    while (!(ovl.lutvec.low & 1)) {
        // Skipped LUT entry
        ovl.lutvec.word >>= 1;
        counter++;
    }
    lut.colors[counter].low = byte;
    state = state_LUT16_COLOR2;
    STATE_RETURN();
}

static void state_LUT16_COLOR2(void) __naked
{
    lut.colors[counter++].high = byte;
    ovl.lutvec.word >>= 1;
    state = ovl.lutvec.word ? state_LUT16_COLOR1 : state_OPCODE;
    STATE_RETURN();
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