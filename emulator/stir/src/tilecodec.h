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

    int findColor(RGB565 c) const {
	// Is a particular color in the LUT? Return the index.
	for (unsigned i = 0; i < LUT_MAX; i++)
	    if (colors[i] == c)
		return i;
	return -1;
    }

private:
    std::list<int> mru;
    TilePalette::ColorMode lastMode;
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
    /// XXX: Use the actual opcode during order optimization, since some tiles will want RLE and some won't.
    void encode(const TileRef tile, std::vector<uint8_t>& out);
    void flush(std::vector<uint8_t>& out);

 private:
    enum Opcode {
	OP_SPECIAL,          // Uses 'repeat' as a sub-opcode.
	OP_TILE_LUT2,        // 2-color (1-bit) indexed tiles
	OP_TILE_LUT4,        // 4-color (2-bit) indexed tiles
	OP_TILE_LUT16,       // 16-color (4-bit) indexed tiles
    };

    uint8_t opcode;
    uint8_t repeat;

};


#endif
