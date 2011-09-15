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
#define RF_OP_VRAM_ADDRESS	0x00	// Change the VRAM write pointer
#define RF_OP_VRAM_DIRECT	0x01	// Change write mode: Direct
#define RF_OP_VRAM_VERTICAL	0x02	// Change write mode: Top-to-bottom
#define RF_OP_FLASH_RESET       0x03    // Reset the flash decoder state machine

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
