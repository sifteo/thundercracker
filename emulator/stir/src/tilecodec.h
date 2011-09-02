/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * STIR -- Sifteo Tiled Image Reducer
 * M. Elizabeth Scott <beth@sifteo.com>
 *
 * Copyright <c> 2011 Sifteo, Inc. All rights reserved.
 */

#ifndef _TILECODEC_H
#define _TILECODEC_H

#include "tile.h"
#include "logger.h"


/*
 * TileCodecLUT --
 *
 *    This is similar to TilePalette, but here instead of tracking the
 *    colors used in a particular tile, we'll track the state of the
 *    color LUT in the tile encoder/decoder.
 *
 *    This is used for actually encoding tiles, as well as for
 *    estimating the cost of encoding a particular tile using the
 *    current encoder state.
 */

struct TileCodecLUT {
    // Current state of the color table
    static const unsigned LUT_MAX = 16;
    RGB565 colors[LUT_MAX];

    TileCodecLUT();

    unsigned encode(const TilePalette &pal, uint16_t &newColors);
    unsigned encode(const TilePalette &pal);

    int findColor(RGB565 c, int maxIndex = LUT_MAX - 1) const {
	// Is a particular color in the LUT? Return the index.
	for (unsigned i = 0; i <= maxIndex; i++)
	    if (colors[i] == c)
		return i;
	return -1;
    }

private:
    std::list<int> mru;
    TilePalette::ColorMode lastMode;
};


/*
 * RLECodec4 --
 *
 *    A nybble-wise RLE codec. Nybbles are handy since the 8051 can
 *    quickly do 4-bit rotations, and 4 bits is a handy size for run
 *    length counts. The encoding is simple- every time two identical
 *    nybbles are repeated, a third nybble follows with a count of
 *    additional repeats.
 *
 *    Nybbles are stored in little-endian (least significant nybble first).
 */

class RLECodec4 {
 public:
    RLECodec4();

    void encode(uint8_t nybble, std::vector<uint8_t>& out);
    void flush(std::vector<uint8_t>& out);

 private:
    static const unsigned MAX_RUN = 15;
    
    uint8_t runNybble, bufferedNybble;
    bool isNybbleBuffered;
    int runCount;

    void encodeNybble(uint8_t value, std::vector<uint8_t>& out);
    void encodeRun(std::vector<uint8_t>& out);
};


/*
 * TileCodec --
 *
 *    A stateful compressor for streams of Tile data.  The encoded
 *    format is a "load stream", a sequence of opcodes that can be
 *    sent over the radio to the cube MCU in order to reproduce the
 *    original tile data in flash memory.
 */

class TileCodec {
 public:
    TileCodec();

    void encode(const TileRef tile, std::vector<uint8_t>& out);
    void flush(std::vector<uint8_t>& out);

    void dumpStatistics(Logger &log);

 private:

    // Low 5 bits is an argument, upper 3 bits are an opcode:
    enum Opcode {
	OP_MASK		= 0xe0,
	ARG_MASK	= 0x1f,

	OP_LUT1		= 0x00,	// Single color 16-bit LUT entry (argument is index)
	OP_LUT16	= 0x20,	// Up to 16 LUT entries follow a 16-bit vector of indices
	OP_TILE_P0	= 0x40, // One trivial solid-color tile (arg = color index)
	OP_TILE_P1_R4	= 0x60,	// Tiles with 1-bit pixels and 4-bit RLE encoding (arg = count-1)
	OP_TILE_P2_R4	= 0x80,	// Tiles with 2-bit pixels and 4-bit RLE encoding (arg = count-1)
	OP_TILE_P4_R4	= 0xa0,	// Tiles with 4-bit pixels and 4-bit RLE encoding (arg = count-1)
	OP_TILE_P16	= 0xc0, // Tile with uncompressed 16-bit pixels (arg = count-1)
	OP_SPECIAL	= 0xe0, // Special symbol
    };

    bool opIsBuffered;
    uint8_t opcodeBuf;
    std::vector<uint8_t> dataBuf;
    unsigned tileCount;
    TileCodecLUT lut;

    // Stats
    struct {
	unsigned opcodes;
	unsigned tiles;
	unsigned dataBytes;
    } stats[TilePalette::CM_COUNT];
    unsigned statBucket;

    void newStatsTile(unsigned bucket);
    void encodeOp(uint8_t op, std::vector<uint8_t>& out);
    void encodeLUT(uint16_t newColors, std::vector<uint8_t>& out);
    void encodeWord(uint16_t w);
    void encodeTileRLE(const TileRef tile, unsigned bits);
    void encodeTileUncompressed(const TileRef tile);
};


#endif
