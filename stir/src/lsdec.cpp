/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * Simple loadstream interpreter. This is a prototype for the
 * interpreter we'll use in the cube's 8051 firmware.
 *
 * M. Elizabeth Scott <beth@sifteo.com>
 *
 * Copyright <c> 2011 Sifteo, Inc. All rights reserved.
 */

#include <stdio.h>
#include <stdlib.h>
#include "color.h"
#include "lodepng.h"

// Codec constants
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

uint32_t flash_addr;

#define FLASH_WRITE_8(b) {			\
	flash.push_back((uint8_t) (b));		\
	flash_addr++;				\
    }

#define FLASH_WRITE_16(b) {			\
	flash.push_back((uint8_t) (b));		\
	flash.push_back((uint8_t) ((b) >> 8));	\
	flash_addr += 2;	       		\
    }


/*
 * Decoder state machine, written with an eye toward efficiency on an
 * 8051 with SDCC. This needs to preferably run from the radio ISR!
 */

static void handleByte(uint8_t byte, std::vector<uint8_t>& flash)
{
    static uint16_t lut[LUT_SIZE];
    static uint16_t p16run;
    static uint16_t lutVector;
    static uint8_t opcode;
    static uint8_t state;
    static uint8_t partial;
    static uint8_t counter;
    static uint8_t rle1;
    static uint8_t rle2;

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
	    uint8_t i;
	    uint16_t color = lut[byte & 0x0F];
	    for (i = 0; i < 64; i++)
		FLASH_WRITE_16(color);
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
	partial = byte;
	state = S_ADDR_HIGH;
	return;
    }

    case S_ADDR_HIGH: {
	flash_addr = ((byte >> 1) << 7) | ((partial >> 1) << 14);
	printf("Load address: 0x%06x\n", flash_addr);

	state = S_OPCODE;
	return;
    }

    case S_ERASE_COUNT: {
	partial = byte;
	state = S_ERASE_CHECK;
	return;
    }

    case S_ERASE_CHECK: {
	printf("Erase: %d blocks, chk %02x (expect %02x)\n", partial + 1, byte,
	       0xFF ^ (uint8_t)(-partial -(uint8_t)((flash_addr >> 14) << 1)
				-(uint8_t)((flash_addr >> 7) << 1)));
	state = S_OPCODE;
	return;
    }

    case S_LUT1_COLOR1: {
	partial = byte;
	state = S_LUT1_COLOR2;
	return;
    }

    case S_LUT1_COLOR2: {
	lut[opcode & 0x0F] = partial | (byte << 8);
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
	partial = byte;
	state = S_LUT16_COLOR2;
	return;
    }

    case S_LUT16_COLOR2: {
	while (!(lutVector & 1)) {
	    // Skipped LUT entry
	    lutVector >>= 1;
	    counter++;
	}
	lut[counter] = partial | (byte << 8);
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
		    FLASH_WRITE_16(lut[nybble & 1]);
		    nybble = (nybble >> 1) | (nybble << 7);
		    FLASH_WRITE_16(lut[nybble & 1]);
		    nybble = (nybble >> 1) | (nybble << 7);
		    FLASH_WRITE_16(lut[nybble & 1]);
		    nybble = (nybble >> 1) | (nybble << 7);
		    FLASH_WRITE_16(lut[nybble & 1]);
		    nybble = (nybble >> 1) | (nybble << 7);
		    counter -= 4;
		    break;

		case OP_TILE_P2_R4:
		    FLASH_WRITE_16(lut[nybble & 3]);
		    nybble = (nybble >> 2) | (nybble << 6);
		    FLASH_WRITE_16(lut[nybble & 3]);
		    nybble = (nybble >> 2) | (nybble << 6);
		    counter -= 2;
		    break;

		case OP_TILE_P4_R4:
		    FLASH_WRITE_16(lut[nybble & 0xF]);
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
	    FLASH_WRITE_16(p16run);
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
	FLASH_WRITE_8(byte);
	partial = byte;
	state = S_TILE_P16_HIGH;
	return;
    }

    case S_TILE_P16_HIGH: {
	p16run = partial | (byte << 8);
	FLASH_WRITE_8(byte);

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


/*
 * Test harness. This code isn't intended to simulate the actual firmware :)
 */

int main(int argc, char **argv)
{
    if (argc != 3 && argc != 5) {
	fprintf(stderr, "usage: %s loadstream.bin tiles.png [tilemap.bin imageWidth]\n", argv[0]);
	return 1;
    }

    // Read the whole loadstream
    std::vector<uint8_t> loadstream;
    LodePNG::loadFile(loadstream, argv[1]);

    // Simulate the firmware's loadstream decoding process.
    std::vector<uint8_t> flash;
    for (std::vector<uint8_t>::iterator i = loadstream.begin(); i != loadstream.end(); i++)
	handleByte(*i, flash);
 
    unsigned pixels = flash.size() / 2;
    unsigned tiles = pixels / 64;
    unsigned tileMapWidth;
    std::vector<uint8_t> tileMap;

    if (argc == 5) {
	/*
	 * Load a real tile map from disk
	 */
	
	LodePNG::loadFile(tileMap, argv[3]);
	tileMapWidth = atoi(argv[4]) / 8;

    } else {
	/*
	 * Generate an all-inclusive tilemap, to show all tiles.
	 */

	tileMapWidth = 16;
	for (unsigned index = 0; index < tiles; index++) {
	    tileMap.push_back(index << 1);
	    tileMap.push_back((index >> 6) & 0xFE);
	}
    }

    /*
     * Using the tile map, render a fully decoded RGBA image.
     */

    unsigned tileMapHeight = (tileMap.size()/2 + tileMapWidth - 1) / tileMapWidth;
    unsigned pixelsWide = tileMapWidth << 3;
    unsigned pixelsHigh = tileMapHeight << 3;
    std::vector<uint8_t> rgba;

    for (unsigned y = 0; y < pixelsHigh; y++)
	for (unsigned x = 0; x < pixelsWide; x++) {
	    unsigned tileX = x & 7;
	    unsigned tileY = y & 7;
	    unsigned mapIndex = ((x >> 3) + (y >> 3) * tileMapWidth) << 1;

	    if (mapIndex + 1 < tileMap.size()) {
		uint8_t low = tileMap[mapIndex];
		uint8_t high = tileMap[mapIndex + 1];
		unsigned address = (high << 13) + (low << 6) + (tileY << 4) + (tileX << 1);
		Stir::RGB565 color((uint16_t)( flash[address] | (flash[address+1] << 8) ));

		rgba.push_back(color.red());
		rgba.push_back(color.green());
		rgba.push_back(color.blue());
		rgba.push_back(0xFF);

	    } else {
		rgba.push_back(0x00);
		rgba.push_back(0x00);
		rgba.push_back(0x00);
		rgba.push_back(0x00);
	    }
	}

    // Write out as a PNG image
    LodePNG::Encoder encoder;
    std::vector<uint8_t> pngOut;
    encoder.encode(pngOut, &rgba[0], pixelsWide, pixelsHigh);
    LodePNG::saveFile(pngOut, argv[2]);

    return 0;
}
    
