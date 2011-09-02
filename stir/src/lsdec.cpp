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

#define FLASH_WRITE_8(b) {			\
	flash.push_back((uint8_t) (b));		\
    }

#define FLASH_WRITE_16(b) {			\
	flash.push_back((uint8_t) (b));		\
	flash.push_back((uint8_t) ((b) >> 8));	\
    }


/*
 * Decoder state machine, written with an eye toward efficiency on an
 * 8051 with SDCC. This needs to preferably run from the radio ISR!
 */

static void handleByte(uint8_t byte, std::vector<uint8_t>& flash)
{
    static uint16_t lut[LUT_SIZE];
    static uint16_t lutVector;
    static uint8_t opcode;
    static uint8_t state;
    static uint8_t partial;
    static uint8_t counter;
    static uint8_t prevNybble;
    static uint8_t prevPrevNybble;

    enum {
	S_OPCODE = 0,
	S_LUT1_COLOR1,
	S_LUT1_COLOR2,
	S_LUT16_VEC1,
	S_LUT16_VEC2,
	S_LUT16_COLOR1,
	S_LUT16_COLOR2,
	S_TILE_RAW,
	S_TILE_RLE,
    };

    switch (state) {

    case S_OPCODE: {
	opcode = byte;
	switch (opcode & OP_MASK) {

	case OP_LUT1:
	    state = S_LUT1_COLOR1;
	    break;

	case OP_LUT16:
	    state = S_LUT16_VEC1;
	    break;

	case OP_TILE_P0: {
	    // Trivial solid-color tile, no repeats
	    uint8_t i;
	    uint16_t color = lut[byte & 0x0F];
	    for (i = 0; i < 64; i++)
		FLASH_WRITE_16(color);
	    break;
	}

	case OP_TILE_P1_R4:
	case OP_TILE_P2_R4:
	case OP_TILE_P4_R4:
	    counter = 64;
	    prevNybble = 0xFF;
	    state = S_TILE_RLE;
	    break;

	case OP_TILE_P16:
	    counter = 128;
	    state = S_TILE_RAW;
	    break;

	case OP_SPECIAL:
	    break;
	}
	break;
    }

    case S_LUT1_COLOR1: {
	partial = byte;
	state = S_LUT1_COLOR2;
	break;
    }

    case S_LUT1_COLOR2: {
	lut[opcode & 0x0F] = partial | (byte << 8);
	state = S_OPCODE;
	break;
    }

    case S_LUT16_VEC1: {
	partial = byte;
	state = S_LUT16_VEC2;
	break;
    }

    case S_LUT16_VEC2: {
	lutVector = partial | (byte << 8);
	counter = 0;
	state = S_LUT16_COLOR1;
	break;
    }

    case S_LUT16_COLOR1: {
	partial = byte;
	state = S_LUT16_COLOR2;
	break;
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
	break;
    }	

    case S_TILE_RAW: {
	FLASH_WRITE_8(byte);

	if (!--counter)
	    if (opcode & ARG_MASK) {
		opcode--;
		counter = 128;
	    } else {
		state = S_OPCODE;
	    }
	break;
    }

    case S_TILE_RLE: {
	uint8_t nybbleI = 2;

	// Loop over the two nybbles in our input byte, processing both identically.
	do {
	    uint8_t nybble = byte & 0x0F;
	    uint8_t runLength;

	    if (prevNybble == prevPrevNybble) {
		// This is a run, and "nybble" is the run length.
		runLength = nybble;

		// Fill the whole byte, so we can do right-rotates below to our heart's content.
		nybble = prevNybble | (prevNybble << 4);

		// Disarm the run detector with a byte that can never occur in 'nybble'.
		prevNybble = 0xF0;

	    } else {
		// Not a run yet, just a literal nybble
		runLength = 1;
		prevPrevNybble = prevNybble;
		prevNybble = nybble;
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
		    break;
		}

	    byte = (byte >> 4) | (byte << 4);
	} while (--nybbleI);
	
	break;
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
		RGB565 color((uint16_t)( flash[address] | (flash[address+1] << 8) ));

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
    
