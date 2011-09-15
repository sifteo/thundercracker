/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * Thundercracker common protocol definitions
 *
 * M. Elizabeth Scott <beth@sifteo.com>
 * Copyright <c> 2011 Sifteo, Inc. All rights reserved.
 */

#ifndef _TC_PROTOCOL_H
#define _TC_PROTOCOL_H

#include <stdint.h>

/*
 * Radio protocol opcodes.
 *
 * Our basic opcode format is a 2-bit command and a 6-bit length, with
 * length always encoded as the actual length - 1. This lets us fit
 * the most common commands into a single byte, and the 6-bit length
 * allows us to amortize the opcode size to less than a byte per
 * packet.
 *
 * Opcodes may persist past the end of a radio packet if and only if
 * that packet is the maximum length. At the end of any non-maximal
 * length packet, the radio state machine is reset. If you need to
 * explicitly reset the state machine, you can send a zero-length
 * packet.
 *
 * XXX: Figure out how best to allocate this opcode space once we get
 *      a sample of some real-world radio traffic for a game.
 */

#define RF_OP_MASK		0xc0
#define RF_ARG_MASK		0x3f

// Major opcodes
#define RF_OP_SPECIAL		0x00    // Multiplexor for special opcodes
#define RF_OP_VRAM_SKIP		0x40	// Seek forward arg+1 bytes
#define RF_OP_VRAM_DATA		0x80    // Write arg+1 bytes to VRAM
#define RF_OP_FLASH_QUEUE	0xc0    // Write arg+1 bytes to flash loadstream FIFO

// Special minor opcodes
#define RF_OP_NOP		0x00	// Reserved for explicit no-op
#define RF_OP_FLASH_RESET       0x01    // Reset the flash decoder state machine

/*
 * ACK packet format
 */

#define RF_ACK_LENGTH  		12

// Offset defines (for use in inline assembly)
#define RF_ACK_ACCEL_COUNTS	1
#define RF_ACK_ACCEL_TOTALS	3

typedef union {
    uint8_t bytes[RF_ACK_LENGTH];
    struct {
	/*
	 * Our standard response packet format. This currently all
	 * comes to a total of 15 bytes. We probably also want a
	 * variable-length "tail" on this packet, to allow us to
	 * transmit specific data that the master requested, like HWID
	 * or firmware version. But this is the high-frequency
	 * telemetry data that we ALWAYS send at the full packet rate.
	 */

	/*
	 * For synchronizing LCD refreshes, the master can keep track
	 * of how many repaints the cube has performed. Ideally these
	 * repaints would be in turn sync'ed with the LCDC's hardware
	 * refresh timer. If we're tight on space, we don't need a
	 * full byte for this. Even a one-bit toggle woudl work,
	 * though we might want two bits to allow deeper queues.
	 */
	uint8_t frame_count;

        // Averaged accel data
	uint8_t accel_count[2];
	uint8_t accel_total_bytes[4];  // Two unaligned 16-bit values

	/*
	 * Need ~5 bits per sensor (5 other cubes * 4 sides + 1 idle =
	 * 21 states) So, there are plenty of bits free in here to
	 * encode things like button state.
	 */
	uint8_t neighbor[4];

	/*
	 * Number of bytes processed by the flash decoder so
	 * far. Increments and wraps around, never decrements or
	 * resets.
	 */
	uint8_t flash_fifo_bytes;
    };
} RF_ACKType;

/*
 * Flash Loadstream codec
 */

#define FLS_FIFO_SIZE           64	// Size of buffer between radio and flash decoder
#define FLS_FIFO_RESET          0xFF	// Reserved FIFO index used to signal RF_OP_FLASH_RESET

#define FLS_LUT_SIZE		16	// Size of persistent color LUT used by RLE encodings

#define FLS_OP_MASK	 	0xe0	// Upper 3 bits are an opcode
#define FLS_ARG_MASK		0x1f	// Lower 5 bits are an argument (usually a repeat count)

#define FLS_OP_LUT1		0x00	// Single color 16-bit LUT entry (argument is index)
#define FLS_OP_LUT16		0x20	// Up to 16 LUT entries follow a 16-bit vector of indices
#define FLS_OP_TILE_P0		0x40	// One trivial solid-color tile (arg = color index)
#define FLS_OP_TILE_P1_R4	0x60	// Tiles with 1-bit pixels and 4-bit RLE encoding (arg = count-1)
#define FLS_OP_TILE_P2_R4	0x80	// Tiles with 2-bit pixels and 4-bit RLE encoding (arg = count-1)
#define FLS_OP_TILE_P4_R4	0xa0	// Tiles with 4-bit pixels and 4-bit RLE encoding (arg = count-1)
#define FLS_OP_TILE_P16		0xc0	// Tile with 16-bit pixels and 8-bit repetition mask (arg = count-1)
#define FLS_OP_SPECIAL		0xe0	// Special symbols (below)

#define FLS_OP_ADDRESS		0xe1	// Followed by a 2-byte (lat1:lat2) tile address
#define FLS_OP_ERASE		0xf5	// Followed by count-1 of 64K blocks, and a 1-byte checksum


#endif
