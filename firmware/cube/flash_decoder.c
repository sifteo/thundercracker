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

uint8_t __idata flash_fifo[FLASH_FIFO_MASK + 1];
uint8_t flash_fifo_head;


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
static uint8_t state;
static uint8_t counter;

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
    state = S_OPCODE;
}

void flash_handle_fifo(void)
{
    while (flash_fifo_head != fifo_tail) {
	/*
	 * Dequeue one byte from the FIFO.
	 *
	 * As soon as we increment flash_fifo_bytes, the master may
	 * overwrite the location we just freed up in the FIFO buffer.
	 */

	uint8_t byte = flash_fifo[fifo_tail];	
	fifo_tail = (fifo_tail + 1) & FLASH_FIFO_MASK;
	ack_data.flash_fifo_bytes++;

	/*
	 * We have a state machine that processes exactly one byte per invocation.
	 */

	switch (state) {

	case S_OPCODE: {
	    opcode = byte;
	    switch (opcode & OP_MASK) {

	    case OP_LUT1:
		state = S_LUT1_COLOR1;
		goto byte_done;

	    case OP_LUT16:
		state = S_LUT16_VEC1;
		goto byte_done;

	    case OP_TILE_P0: {
		// Trivial solid-color tile, no repeats
		uint8_t i = 64;
		byte &= 0x0F;
		do {
		    flash_program(lut.low[byte]);
		    flash_program(lut.high[byte]);
		} while (--i);
		goto byte_done;
	    }

	    case OP_TILE_P1_R4:
	    case OP_TILE_P2_R4:
	    case OP_TILE_P4_R4:
		counter = 64;
		ovl.rle1 = 0xFF;
		state = S_TILE_RLE4;
		goto byte_done;

	    case OP_TILE_P16:
		counter = 8;
		state = S_TILE_P16_MASK;
		goto byte_done;

	    case OP_SPECIAL:
		switch (opcode) {

		case OP_ADDRESS:
		    state = S_ADDR_LOW;
		    goto byte_done;

		case OP_ERASE:
		    state = S_ERASE_COUNT;
		    goto byte_done;

		default:
		    goto byte_done;
		}
	    }
	}

	case S_ADDR_LOW: {
	    flash_addr_low = 0;
	    flash_addr_lat1 = byte;
	    state = S_ADDR_HIGH;
	    goto byte_done;
	}

	case S_ADDR_HIGH: {
	    flash_addr_lat2 = byte;
	    state = S_OPCODE;
	    goto byte_done;
	}

	case S_ERASE_COUNT: {
	    counter = byte;
	    state = S_ERASE_CHECK;
	    goto byte_done;
	}

	case S_ERASE_CHECK: {
	    uint8_t check = 0xFF ^ (-counter -flash_addr_lat1 -flash_addr_lat2);
	    if (check == byte)
		flash_erase(counter);
	    state = S_OPCODE;
	    goto byte_done;
	}

	case S_LUT1_COLOR1: {
	    lut.low[opcode & 0x0F] = byte;
	    state = S_LUT1_COLOR2;
	    goto byte_done;
	}

	case S_LUT1_COLOR2: {
	    lut.high[opcode & 0x0F] = byte;
	    state = S_OPCODE;
	    goto byte_done;
	}

	case S_LUT16_VEC1: {
	    ovl.lutvec.low = byte;
	    state = S_LUT16_VEC2;
	    goto byte_done;
	}

	case S_LUT16_VEC2: {
	    ovl.lutvec.high = byte;
	    counter = 0;
	    state = S_LUT16_COLOR1;
	    goto byte_done;
	}

	case S_LUT16_COLOR1: {
	    while (!(ovl.lutvec.low & 1)) {
		// Skipped LUT entry
		ovl.lutvec.word >>= 1;
		counter++;
	    }
	    lut.low[counter] = byte;
	    state = S_LUT16_COLOR2;
	    goto byte_done;
	}

	case S_LUT16_COLOR2: {
	    lut.high[counter] = byte;
	    ovl.lutvec.word >>= 1;
	    counter++;
	    state = ovl.lutvec.word ? S_LUT16_COLOR1 : S_OPCODE;
	    goto byte_done;
	}	

	case S_TILE_RLE4: {
	    uint8_t nybbleI = 2;

	    // Loop over the two nybbles in our input byte, processing both identically.
	    do {
		uint8_t nybble = byte & 0x0F;
		uint8_t runLength, i;

		if (ovl.rle1 == ovl.rle2) {
		    // This is a run, and "nybble" is the run length.
		    runLength = nybble;

		    // Fill the whole byte, so we can do right-rotates below to our heart's content.
		    nybble = ovl.rle1 | (ovl.rle1 << 4);

		    // Disarm the run detector with a byte that can never occur in 'nybble'.
		    ovl.rle1 = 0xF0;

		} else {
		    // Not a run yet, just a literal nybble
		    runLength = 1;
		    ovl.rle2 = ovl.rle1;
		    ovl.rle1 = nybble;
		}

		// Unpack each nybble at the current bit depth
		while (runLength) {
		    runLength--;

		    switch (opcode & OP_MASK) {

		    case OP_TILE_P1_R4:
			i = 4;
			do {
			    flash_program(lut.low[nybble & 1]);
			    flash_program(lut.high[nybble & 1]);
			    nybble = rr(nybble);
			    counter--;
			} while (--i);
			break;

		    case OP_TILE_P2_R4:
			i = 2;
			do {
			    flash_program(lut.low[nybble & 3]);
			    flash_program(lut.high[nybble & 3]);
			    nybble = rr(nybble);
			    nybble = rr(nybble);
			    counter--;
			} while (--i);
			break;

		    case OP_TILE_P4_R4:
			flash_program(lut.low[nybble & 0xF]);
			flash_program(lut.high[nybble & 0xF]);
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
			goto byte_done;
		    }

		byte = (byte >> 4) | (byte << 4);
	    } while (--nybbleI);
	
	    goto byte_done;
	}

	case S_TILE_P16_MASK: {
	    ovl.rle1 = byte;  // Mask byte for the next 8 pixels
	    ovl.rle2 = 8;     // Remaining pixels in mask

	    p16_emit_runs:
	    do {
		if (ovl.rle1 & 1) {
		    state = S_TILE_P16_LOW;
		    goto byte_done;
		}
		flash_program(lut.p16_low);
		flash_program(lut.p16_high);
		ovl.rle1 = rr(ovl.rle1);
	    } while (--ovl.rle2);

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

	    goto byte_done;
	}

	case S_TILE_P16_LOW: {
	    flash_program(byte);
	    lut.p16_low = byte;
	    state = S_TILE_P16_HIGH;
	    goto byte_done;
	}

	case S_TILE_P16_HIGH: {
	    lut.p16_high = byte;
	    flash_program(byte);

	    if (--ovl.rle2) {
		// Still more pixels in this mask.
		ovl.rle1 = rr(ovl.rle1);
		goto p16_emit_runs;
	    } else {
		// End of the mask byte or the whole tile
		goto p16_next_mask;
	    }
	}
	}

    byte_done: ;
    }
}
