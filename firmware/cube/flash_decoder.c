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
 */

#include "flash.h"
#include "hardware.h"
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
 
#define FLASH_TIMESLICE_TICKS  3

/*
 * FIFO buffer, written by the radio ISR
 */
 
volatile uint8_t __idata flash_fifo[FLS_FIFO_SIZE];
volatile uint8_t flash_fifo_head;

/*
 * Loadstream codec state
 */

union word16 {
    uint16_t word;
    struct {
        uint8_t low, high;
    };
};

// Color lookup table
static __idata struct {
    union word16 colors[FLS_LUT_SIZE];
    union word16 p16;
} lut;

// Overlaid memory for individual states' use
static union {
    union word16 lutvec;

    struct {
        uint8_t rle1;
        uint8_t rle2;
    };
} ovl;

static uint8_t fifo_tail;
static uint8_t opcode;
static uint8_t counter;
static uint8_t byte;
static uint8_t tick_deadline;
static void (*state)(void);

// Shared temporaries, within a state
static __bit nibIndex;

/*
 * Decoder implementation.
 */

static void state_OPCODE(void) __naked;
static void state_ADDR_LOW(void) __naked;
static void state_ADDR_HIGH(void) __naked;
static void state_LUT1_COLOR1(void) __naked;
static void state_LUT1_COLOR2(void) __naked;
static void state_LUT16_VEC1(void) __naked;
static void state_LUT16_VEC2(void) __naked;
static void state_LUT16_COLOR1(void) __naked;
static void state_LUT16_COLOR2(void) __naked;
static void state_TILE_P1_R4(void) __naked;
static void state_TILE_P2_R4(void) __naked;
static void state_TILE_P4_R4(void) __naked;
static void state_TILE_P16_MASK(void) __naked;
static void state_TILE_P16_LOW(void) __naked;
static void state_TILE_P16_HIGH(void) __naked;

#define STATE_RETURN()  __asm ljmp state_return __endasm

void flash_init(void)
{
    /*
     * Reset the state machine, reset the LUT.
     */

    __idata uint8_t *l = (__idata uint8_t *) &lut;
    uint8_t i = sizeof lut;

    do {
        *(l++) = 0;
    } while (--i);

    fifo_tail = flash_fifo_head;
    state = state_OPCODE;
}

void flash_handle_fifo(void)
{
    // Nothing to do? Exit early.
    if (flash_fifo_head == fifo_tail)
        return;
        
    global_busy_flag = 1;

    /*
     * Out-of-band cue to reset the state machine.
     *
     * We don't have to check for this every byte, since latency in
     * initiating a reset isn't especially important.
     */
    
    if (flash_fifo_head == FLS_FIFO_RESET) {
        flash_fifo_head = 0;
        flash_init();

        __asm
            inc (_ack_data + RF_ACK_FLASH_FIFO)
            mov _ack_len, #RF_ACK_LEN_MAX
        __endasm ;

        return;
    }

    // Prep the flash hardware to start writing
    tick_deadline = sensor_tick_counter + FLASH_TIMESLICE_TICKS;
    flash_program_start();
    
#ifdef DEBUG_LOADSTREAM
    DEBUG_UART_INIT();
#endif

    // This is where STATE_RETURN() drops us after each state finishes.
    __asm
        state_return:
    __endasm ;

    // No more data? Timeslice is over? Clean up and get out.
    if (tick_deadline == sensor_tick_counter || flash_fifo_head == fifo_tail) {
        flash_program_end();
        return;
    }
    
    /*
     * Dequeue one byte from the FIFO.
     *
     * As soon as we increment flash_fifo_bytes, the master may
     * overwrite the location we just freed up in the FIFO buffer.
     */

    byte = flash_fifo[fifo_tail];       
    fifo_tail = (fifo_tail + 1) & (FLS_FIFO_SIZE - 1);

#ifdef DEBUG_LOADSTREAM
    DEBUG_UART(byte);
#endif

    __asm
        inc     (_ack_data + RF_ACK_FLASH_FIFO)
        mov     _ack_len, #RF_ACK_LEN_MAX
    __endasm ;

    __asm
        clr   a
        mov   dpl, _state
        mov   dph, (_state+1)
        jmp   @a+dptr
    __endasm ;
}

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
        flash_autoerase();
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
        flash_autoerase();
        counter = 64;
        ovl.rle1 = 0xFF;
        state = state_TILE_P1_R4;
        STATE_RETURN();

    case FLS_OP_TILE_P2_R4:
        flash_autoerase();
        counter = 64;
        ovl.rle1 = 0xFF;
        state = state_TILE_P2_R4;
        STATE_RETURN();

    case FLS_OP_TILE_P4_R4:
        flash_autoerase();
        counter = 64;
        ovl.rle1 = 0xFF;
        state = state_TILE_P4_R4;
        STATE_RETURN();
        
    case FLS_OP_TILE_P16:
        flash_autoerase();
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
    flash_addr_lat2 = byte & 0xFE;
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
                flash_autoerase();
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
                flash_autoerase();
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
               flash_autoerase();
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
            flash_autoerase();                  \
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
