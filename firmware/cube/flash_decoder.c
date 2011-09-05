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

enum {
    S_OPCODE = 0,
    S_ADDR_LOW,
    S_ADDR_HIGH,
    S_ERASE_COUNT,
    S_ERASE_CHECK,
    S_LUT1_COLOR1,
    S_LUT1_COLOR2,
    S_LUT16_VEC1,
    S_LUT16_VEC2,
    S_LUT16_COLOR1,
    S_LUT16_COLOR2,
    S_TILE_RLE4,
    S_TILE_P16_MASK,
    S_TILE_P16_LOW,
    S_TILE_P16_HIGH,
};

static __idata uint8_t lut_low[LUT_SIZE];
static __idata uint8_t lut_high[LUT_SIZE];
static uint16_t lutVector;
static uint8_t p16run_low;
static uint8_t p16run_high;
static uint8_t opcode;
static uint8_t state;
static uint8_t partial;
static uint8_t counter;
static uint8_t rle1;
static uint8_t rle2;

/*
 * Byte decoder state machine
 */

static void flash_decode_byte(uint8_t byte)
{
    switch (state) {

    case S_OPCODE: {
	opcode = byte;
	switch (opcode & OP_MASK) {

	case OP_LUT1:
	    state = S_LUT1_COLOR1;
	    return;

	case OP_LUT16:
	    state = S_LUT16_VEC1;
	    return;

	case OP_TILE_P0: {
	    // Trivial solid-color tile, no repeats
	    uint8_t i = 64;
	    byte &= 0x0F;
	    do {
		flash_program(lut_low[byte]);
		flash_program(lut_high[byte]);
	    } while (--i);
	    return;
	}

	case OP_TILE_P1_R4:
	case OP_TILE_P2_R4:
	case OP_TILE_P4_R4:
	    counter = 64;
	    rle1 = 0xFF;
	    state = S_TILE_RLE4;
	    return;

	case OP_TILE_P16:
	    counter = 8;
	    state = S_TILE_P16_MASK;
	    return;

	case OP_SPECIAL:
	    switch (opcode) {

	    case OP_ADDRESS:
		state = S_ADDR_LOW;
		return;

	    case OP_ERASE:
		state = S_ERASE_COUNT;
		return;

	    default:
		return;
	    }
	}
    }

    case S_ADDR_LOW: {
	flash_addr_low = 0;
	flash_addr_lat1 = byte;
	state = S_ADDR_HIGH;
	return;
    }

    case S_ADDR_HIGH: {
	flash_addr_lat2 = byte;
	state = S_OPCODE;
	return;
    }

    case S_ERASE_COUNT: {
	partial = byte;
	state = S_ERASE_CHECK;
	return;
    }

    case S_ERASE_CHECK: {
	uint8_t check = 0xFF ^ (-partial -flash_addr_lat1 -flash_addr_lat2);
	if (check == byte)
	    flash_erase(partial);
	state = S_OPCODE;
	return;
    }

    case S_LUT1_COLOR1: {
	lut_low[opcode & 0x0F] = byte;
	state = S_LUT1_COLOR2;
	return;
    }

    case S_LUT1_COLOR2: {
	lut_high[opcode & 0x0F] = byte;
	state = S_OPCODE;
	return;
    }

    case S_LUT16_VEC1: {
	partial = byte;
	state = S_LUT16_VEC2;
	return;
    }

    case S_LUT16_VEC2: {
	lutVector = partial | (byte << 8);
	counter = 0;
	state = S_LUT16_COLOR1;
	return;
    }

    case S_LUT16_COLOR1: {
	while (!(lutVector & 1)) {
	    // Skipped LUT entry
	    lutVector >>= 1;
	    counter++;
	}
	lut_low[counter] = byte;
	state = S_LUT16_COLOR2;
	return;
    }

    case S_LUT16_COLOR2: {
	lut_high[counter] = byte;
	lutVector >>= 1;
	counter++;
	state = lutVector ? S_LUT16_COLOR1 : S_OPCODE;
	return;
    }	

    case S_TILE_RLE4: {
	uint8_t nybbleI = 2;

	// Loop over the two nybbles in our input byte, processing both identically.
	do {
	    uint8_t nybble = byte & 0x0F;
	    uint8_t runLength;

	    if (rle1 == rle2) {
		// This is a run, and "nybble" is the run length.
		runLength = nybble;

		// Fill the whole byte, so we can do right-rotates below to our heart's content.
		nybble = rle1 | (rle1 << 4);

		// Disarm the run detector with a byte that can never occur in 'nybble'.
		rle1 = 0xF0;

	    } else {
		// Not a run yet, just a literal nybble
		runLength = 1;
		rle2 = rle1;
		rle1 = nybble;
	    }

	    // Unpack each nybble at the current bit depth
	    while (runLength) {
		runLength--;
		switch (opcode & OP_MASK) {

		case OP_TILE_P1_R4:
		    flash_program(lut_low[nybble & 1]);
		    flash_program(lut_high[nybble & 1]);
		    nybble = (nybble >> 1) | (nybble << 7);

		    flash_program(lut_low[nybble & 1]);
		    flash_program(lut_high[nybble & 1]);
		    nybble = (nybble >> 1) | (nybble << 7);

		    flash_program(lut_low[nybble & 1]);
		    flash_program(lut_high[nybble & 1]);
		    nybble = (nybble >> 1) | (nybble << 7);

		    flash_program(lut_low[nybble & 1]);
		    flash_program(lut_high[nybble & 1]);
		    nybble = (nybble >> 1) | (nybble << 7);

		    counter -= 4;
		    break;

		case OP_TILE_P2_R4:
		    flash_program(lut_low[nybble & 3]);
		    flash_program(lut_high[nybble & 3]);
		    nybble = (nybble >> 2) | (nybble << 6);

		    flash_program(lut_low[nybble & 3]);
		    flash_program(lut_high[nybble & 3]);
		    nybble = (nybble >> 2) | (nybble << 6);

		    counter -= 2;
		    break;

		case OP_TILE_P4_R4:
		    flash_program(lut_low[nybble & 0xF]);
		    flash_program(lut_high[nybble & 0xF]);
		    counter--;
		    break;
	    
		}
	    }

	    // Finished with this tile? Next.
	    if ((int8_t)counter <= 0)
		if (opcode & ARG_MASK) {
		    opcode--;
		    counter += 64;
		} else {
		    state = S_OPCODE;
		    return;
		}

	    byte = (byte >> 4) | (byte << 4);
	} while (--nybbleI);
	
	return;
    }

    case S_TILE_P16_MASK: {
	rle1 = byte;  // Mask byte for the next 8 pixels
	rle2 = 8;     // Remaining pixels in mask

	p16_emit_runs:
	do {
	    if (rle1 & 1) {
		state = S_TILE_P16_LOW;
		return;
	    }
	    flash_program(p16run_low);
	    flash_program(p16run_high);
	    rle1 = (rle1 >> 1) | (rle1 << 7);
	} while (--rle2);

	p16_next_mask:
	if (--counter) {
	    state = S_TILE_P16_MASK;
	} else if (opcode & ARG_MASK) {
	    opcode--;
	    counter = 8;
	    state = S_TILE_P16_MASK;
	} else {
	    state = S_OPCODE;
	}

	return;
    }

    case S_TILE_P16_LOW: {
	flash_program(byte);
	p16run_low = byte;
	state = S_TILE_P16_HIGH;
	return;
    }

    case S_TILE_P16_HIGH: {
	p16run_high = byte;
	flash_program(byte);

	if (--rle2) {
	    // Still more pixels in this mask.
	    rle1 = (rle1 >> 1) | (rle1 << 7);
	    goto p16_emit_runs;
	} else {
	    // End of the mask byte or the whole tile
	    goto p16_next_mask;
	}
    }
    }
}
