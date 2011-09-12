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

namespace Stir {

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
    void bumpMRU(unsigned mruIndex, unsigned lutIndex) {
	for (;mruIndex < LUT_MAX - 1; mruIndex++)
	    mru[mruIndex] = mru[mruIndex + 1];
	mru[mruIndex] = lutIndex;
    }

    TilePalette::ColorMode lastMode;
    uint8_t mru[LUT_MAX];   // Newest entries at the end, oldest at the front.
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
    void encodeRun(std::vector<uint8_t>& out, bool terminal=false);
};


/*
 * FlashAddress --
 *
 *    Addresses in our cube's flash memory have some special
 *    properties and representations. All tiles are aligned on a
 *    tile-size multiple, and we also have large flash blocks (64K)
 *    which set our erase granularity.
 *
 *    The device firmware doesn't use linear addresses, instead it
 *    uses addresses that have been broken into three left-justified
 *    7-bit chunks. Due to the hardware conventions, these chunks are
 *    named "low", "lat1", and "lat2". The upper chunks are programmed
 *    into the two hardware latches.
 */

struct FlashAddress {
    uint32_t linear;

    static const unsigned FLASH_SIZE = 2 * 1024 * 1024;
    static const unsigned BLOCK_SIZE = 64 * 1024;
    static const unsigned TILE_SIZE = 128;
    static const unsigned TILES_PER_BLOCK = BLOCK_SIZE / TILE_SIZE;
    
    FlashAddress(uint32_t addr)
        : linear(addr) {}

    FlashAddress(uint8_t lat2, uint8_t lat1, uint8_t low = 0)
        : linear( ((lat2 >> 1) << 14) |
		  ((lat1 >> 1) << 7 ) |
		  ((low  >> 1)      ) ) {} 

    uint8_t low() const {
	return linear << 1;
    }

    uint8_t lat1() const {
	return (linear >> 7) << 1;
    }

    uint8_t lat2() const {
	return (linear >> 14) << 1;
    }

    static unsigned tilesToBlocks(unsigned tiles) {
	return (tiles + TILES_PER_BLOCK - 1) / TILES_PER_BLOCK;
    }
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
    TileCodec(std::vector<uint8_t>& buffer);

    void address(FlashAddress addr);
    void erase(unsigned numBlocks);
    void encode(const TileRef tile, bool autoErase=false);
    void flush();

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
	OP_TILE_P16_RM	= 0xc0, // Tile with 16-bit pixels and 8-bit repetition mask (arg = count-1)
	OP_SPECIAL	= 0xe0, // Special symbols (below)

	OP_ADDRESS	= 0xe1, // Followed by a 2-byte (lat1:lat2) tile address
	OP_ERASE        = 0xf5, // Followed by count-1 of 64K blocks, and a 1-byte checksum
    };

    std::vector<uint8_t>& out;
    std::vector<uint8_t> dataBuf;

    bool opIsBuffered;
    uint8_t opcodeBuf;
    unsigned tileCount;
    TileCodecLUT lut;
    RLECodec4 rle;
    uint32_t p16run;
    FlashAddress currentAddress;

    // Stats
    struct {
	unsigned opcodes;
	unsigned tiles;
	unsigned dataBytes;
    } stats[TilePalette::CM_COUNT];
    unsigned statBucket;

    void newStatsTile(unsigned bucket);
    void encodeOp(uint8_t op);
    void encodeLUT(uint16_t newColors);
    void encodeWord(uint16_t w);
    void encodeTileRLE4(const TileRef tile, unsigned bits);
    void encodeTileMasked16(const TileRef tile);
};

};  // namespace Stir

#endif
