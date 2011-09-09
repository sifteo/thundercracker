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
 * The FIFO is small, just large enough for two max-size radio
 * packets.  The only way we have to prevent the master from
 * overrunning this buffer is to provide flow control feedback, in the
 * form of a byte counter in the ack buffer. The master uses this to
 * keep a worst-case estiamate of the cube's current remaining buffer
 * space.
 */

#include "flash.h"
#include "hardware.h"
#include "radio.h"

volatile uint8_t __idata flash_fifo[FLASH_FIFO_MASK + 1];
volatile uint8_t flash_fifo_head;


/*
 * Loadstream codec constants
 */

#define LUT_SIZE	16
#define OP_MASK	 	0xe0
#define ARG_MASK 	0x1f
#define OP_LUT1		0x00
#define OP_LUT16	0x20
#define OP_TILE_P0	0x40
#define OP_TILE_P1_R4	0x60
#define OP_TILE_P2_R4	0x80
#define OP_TILE_P4_R4	0xa0
#define OP_TILE_P16	0xc0
#define OP_SPECIAL	0xe0
#define OP_ADDRESS	0xe1
#define OP_ERASE	0xf5

/*
 * Loadstream codec state
 */

// Color lookup table
static __idata struct {
    uint8_t low[LUT_SIZE];
    uint8_t high[LUT_SIZE];
    uint8_t p16_low;
    uint8_t p16_high;
} lut;

// Overlaid memory for individual states' use
static union {
    union {
	uint16_t word;
	struct {
	    uint8_t low, high;
	};
    } lutvec;

    struct {
	uint8_t rle1;
	uint8_t rle2;
    };
} ovl;

static uint8_t fifo_tail;
static uint8_t opcode;
static uint8_t counter;
static uint8_t byte;
static void (*state)(void);

/*
 * Decoder implementation.
 */

static void state_OPCODE(void);
static void state_ADDR_LOW(void);
static void state_ADDR_HIGH(void);
static void state_ERASE_COUNT(void);
static void state_ERASE_CHECK(void);
static void state_LUT1_COLOR1(void);
static void state_LUT1_COLOR2(void);
static void state_LUT16_VEC1(void);
static void state_LUT16_VEC2(void);
static void state_LUT16_COLOR1(void);
static void state_LUT16_COLOR2(void);
static void state_TILE_P1_R4(void);
static void state_TILE_P2_R4(void);
static void state_TILE_P4_R4(void);
static void state_TILE_P16_MASK(void);
static void state_TILE_P16_LOW(void);
static void state_TILE_P16_HIGH(void);

void flash_init(void)
{
    /*
     * Reset the state machine, reset the LUT.
     */

    __idata uint8_t *l = &lut.low[0];
    uint8_t i = sizeof lut;

    do {
	*(l++) = 0;
    } while (--i);

    fifo_tail = flash_fifo_head;
    state = state_OPCODE;
}

void flash_handle_fifo(void)
{
    /*
     * Out-of-band cue to reset the state machine.
     *
     * We don't have to check for this every byte, since latency in
     * initiating a reset isn't especially important.
     */
    
    if (flash_fifo_head == FLASH_HEAD_RESET) {
	flash_fifo_head = 0;
	flash_init();
	ack_data.flash_fifo_bytes++;
	return;
    }

    while (flash_fifo_head != fifo_tail) {
	/*
	 * Dequeue one byte from the FIFO.
	 *
	 * As soon as we increment flash_fifo_bytes, the master may
	 * overwrite the location we just freed up in the FIFO buffer.
	 */

	byte = flash_fifo[fifo_tail];	
	fifo_tail = (fifo_tail + 1) & FLASH_FIFO_MASK;
	ack_data.flash_fifo_bytes++;

	state();
    }
}

/*
 * State machine.
 *
 * This is a state machine, in which each state is implemented as a
 * function chunk that we jump to. The state transition functions
 * process exactly one byte per invocation.
 */

static void state_OPCODE(void)
{
    opcode = byte;
    switch (opcode & OP_MASK) {

    case OP_LUT1:
	state = state_LUT1_COLOR1;
	return;

    case OP_LUT16:
	state = state_LUT16_VEC1;
	return;

    case OP_TILE_P0: {
	// Trivial solid-color tile, no repeats
	uint8_t i = 64;
	byte &= 0x0F;
	do {
	    flash_program(lut.low[byte]);
	    flash_program(lut.high[byte]);
	} while (--i);
	return;
    }
	
    case OP_TILE_P1_R4:
	counter = 64;
	ovl.rle1 = 0xFF;
	state = state_TILE_P1_R4;
	return;

    case OP_TILE_P2_R4:
	counter = 64;
	ovl.rle1 = 0xFF;
	state = state_TILE_P2_R4;
	return;

    case OP_TILE_P4_R4:
	counter = 64;
	ovl.rle1 = 0xFF;
	state = state_TILE_P4_R4;
	return;
	
    case OP_TILE_P16:
	counter = 8;
	state = state_TILE_P16_MASK;
	return;

    case OP_SPECIAL:
	switch (opcode) {
	    
	case OP_ADDRESS:
	    state = state_ADDR_LOW;
	    return;

	case OP_ERASE:
	    state = state_ERASE_COUNT;
	    return;
	    
	default:
	    return;
	}
	
    default:
	// Undefined opcode
	return;
    }
}

static void state_ADDR_LOW(void)
{
    flash_addr_low = 0;
    flash_addr_lat1 = byte;
    state = state_ADDR_HIGH;
}

static void state_ADDR_HIGH(void)
{
    flash_addr_lat2 = byte;
    state = state_OPCODE;
}

static void state_ERASE_COUNT(void)
{
    counter = byte;
    state = state_ERASE_CHECK;
}

static void state_ERASE_CHECK(void)
{
    uint8_t check = 0xFF ^ (-counter -flash_addr_lat1 -flash_addr_lat2);
    if (check == byte)
	flash_erase(counter);
    state = state_OPCODE;
}

static void state_LUT1_COLOR1(void)
{
    lut.low[opcode & 0x0F] = byte;
    state = state_LUT1_COLOR2;
}

static void state_LUT1_COLOR2(void)
{
    lut.high[opcode & 0x0F] = byte;
    state = state_OPCODE;
}

static void state_LUT16_VEC1(void)
{
    ovl.lutvec.low = byte;
    state = state_LUT16_VEC2;
}

static void state_LUT16_VEC2(void)
{
    ovl.lutvec.high = byte;
    counter = 0;
    state = state_LUT16_COLOR1;
}

static void state_LUT16_COLOR1(void)
{
    while (!(ovl.lutvec.low & 1)) {
	// Skipped LUT entry
	ovl.lutvec.word >>= 1;
	counter++;
    }
    lut.low[counter] = byte;
    state = state_LUT16_COLOR2;
}

static void state_LUT16_COLOR2(void)
{
    lut.high[counter] = byte;
    ovl.lutvec.word >>= 1;
    counter++;
    state = ovl.lutvec.word ? state_LUT16_COLOR1 : state_OPCODE;
}	

#define RLE4_BEGIN()					\
    bit nibIndex = 1;					\
    uint8_t nybble = byte & 0x0F;			\
    uint8_t runLength;					\
    for (;;) {						\
	if (ovl.rle1 == ovl.rle2) {			\
	    runLength = nybble;				\
	    nybble = ovl.rle1 | swap(ovl.rle1);		\
	    ovl.rle1 = 0xF0;				\
	    if (!runLength)				\
		goto no_runs;				\
	} else {					\
	    runLength = 1;				\
	    ovl.rle2 = ovl.rle1;			\
	    ovl.rle1 = nybble;				\
	}						\

#define RLE4_END()					\
    no_runs:						\
	if (!counter || (counter & 0x80))		\
	    if (opcode & ARG_MASK) {			\
		opcode--;				\
		counter += 64;				\
	    } else {					\
		state = state_OPCODE;			\
		return;					\
	    }						\
	if (!nibIndex)					\
	    return;					\
	nibIndex = 0;					\
	nybble = byte >> 4;				\
    }

static void state_TILE_P1_R4(void)
{
    RLE4_BEGIN() {

	runLength = rl(runLength);
	runLength = rl(runLength);
	counter -= runLength;

	do {
	    flash_program(lut.low[nybble & 1]);
	    flash_program(lut.high[nybble & 1]);
	    nybble = rr(nybble);
	} while (--runLength);

    } RLE4_END();
}
		
static void state_TILE_P2_R4(void)
{
    RLE4_BEGIN() {

	runLength = rl(runLength);
	counter -= runLength;

	do {
	    flash_program(lut.low[nybble & 3]);
	    flash_program(lut.high[nybble & 3]);
	    nybble = rr(nybble);
	    nybble = rr(nybble);
	} while (--runLength);

    } RLE4_END();
}

static void state_TILE_P4_R4(void)
{
    RLE4_BEGIN() {
		
	counter -= runLength;

	do {
	    flash_program(lut.low[nybble & 0xF]);
	    flash_program(lut.high[nybble & 0xF]);
	} while (--runLength);

    } RLE4_END();
}

#define P16_EMIT_RUNS() {			\
	do {					\
	    if (ovl.rle1 & 1) {			\
		state = state_TILE_P16_LOW;	\
		return;				\
	    }					\
	    flash_program(lut.p16_low);		\
	    flash_program(lut.p16_high);	\
	    ovl.rle1 = rr(ovl.rle1);		\
	} while (--ovl.rle2);			\
    }

#define P16_NEXT_MASK() {			\
        if (--counter) {			\
	    state = state_TILE_P16_MASK;	\
	} else if (opcode & ARG_MASK) {		\
	    opcode--;				\
	    counter = 8;			\
	    state = state_TILE_P16_MASK;	\
	} else {				\
	    state = state_OPCODE;		\
	}					\
    }

static void state_TILE_P16_MASK(void)
{
    ovl.rle1 = byte;  // Mask byte for the next 8 pixels
    ovl.rle2 = 8;     // Remaining pixels in mask
    
    P16_EMIT_RUNS();
    P16_NEXT_MASK();
}

static void state_TILE_P16_LOW(void)
{
    flash_program(byte);
    lut.p16_low = byte;
    state = state_TILE_P16_HIGH;
}

static void state_TILE_P16_HIGH(void)
{
    lut.p16_high = byte;
    flash_program(byte);

    if (--ovl.rle2) {
	// Still more pixels in this mask.
	ovl.rle1 = rr(ovl.rle1);

	P16_EMIT_RUNS();
    }

    P16_NEXT_MASK();
}
