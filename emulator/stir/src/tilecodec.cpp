/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * STIR -- Sifteo Tiled Image Reducer
 * M. Elizabeth Scott <beth@sifteo.com>
 *
 * Copyright <c> 2011 Sifteo, Inc. All rights reserved.
 */

#include "tilecodec.h"


TileCodecLUT::TileCodecLUT()
{
    for (int i = 0; i < LUT_MAX; i++)
	mru.push_back(i);
}

unsigned TileCodecLUT::encode(const TilePalette &pal)
{
    uint16_t newColors;
    return encode(pal, newColors);
}

unsigned TileCodecLUT::encode(const TilePalette &pal, uint16_t &newColors)
{
    /*
     * Modify the current LUT state in order to accomodate the given
     * tile palette, and measure the associated cost (in bytes).
     */

    const unsigned runBreakCost = 1;   // Breaking a run of tiles, need a new opcode/runlength byte
    const unsigned lutLoadCost = 3;    // Opcode/index byte, plus 16-bit color

    unsigned cost = 0;
    TilePalette::ColorMode mode = pal.colorMode();

    newColors = 0;

    // Account for each color in this tile
    if (pal.hasLUT()) {
	std::vector<RGB565> missing;

	/*
	 * Reverse-iterate over the tile's colors, so that the most
	 * popular colors (at the beginning of the palette) will end
	 * up at the beginning of the MRU list afterwards.
	 */

	for (int c = pal.numColors - 1; c >= 0; c--) {
	    RGB565 color = pal.colors[c];
	    int index = findColor(color);

	    if (index < 0) {
		// Don't have this color yet. Add it to a temporary list of colors that we need
		missing.push_back(color);

	    } else {
		// We already have this color in the LUT! Bump it to the front of the MRU list.
		mru.remove(index);
		mru.push_front(index);
	    }
	}

	/*
	 * After bumping any colors that we do need, add colors we
	 * don't yet have. We replace starting with the least recently
	 * used LUT entries and the least popular missing colors. The
	 * most popular missing colors will end up at the front of the
	 * MRU list.
	 *
	 * Because of the reversal above, the most popular missing
	 * colors are at the end of the vector. So, we iterate in
	 * reverse once more.
	 */

	for (std::vector<RGB565>::reverse_iterator i = missing.rbegin();
	     i != missing.rend(); i++) {

	    int index = mru.back();
	    mru.pop_back();
	    colors[index] = *i;
	    mru.push_front(index);
	    
	    newColors |= 1 << index;
	    cost += lutLoadCost;
	}
    }

    // We have to break a run if we're switching modes OR reloading a LUT entry.
    if (mode != lastMode || cost != 0)
	cost += runBreakCost;
    lastMode = mode;

    return cost;
}

void TileCodec::encode(const TileRef tile, std::vector<uint8_t>& out)
{
}

void TileCodec::flush(std::vector<uint8_t>& out)
{
}

