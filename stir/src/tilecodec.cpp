/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * STIR -- Sifteo Tiled Image Reducer
 * M. Elizabeth Scott <beth@sifteo.com>
 *
 * Copyright <c> 2011 Sifteo, Inc. All rights reserved.
 */

#include <string.h>
#include <assert.h>

#include "tilecodec.h"


TileCodecLUT::TileCodecLUT()
{
    for (int i = 0; i < LUT_MAX; i++)
	mru[i] = i;
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
    unsigned maxLUTIndex = pal.maxLUTIndex();

    newColors = 0;

    // Account for each color in this tile
    if (pal.hasLUT()) {
	unsigned numMissing = 0;
	RGB565 missing[LUT_MAX];

	/*
	 * Reverse-iterate over the tile's colors, so that the most
	 * popular colors (at the beginning of the palette) will end
	 * up at the beginning of the MRU list afterwards.
	 */

	for (int c = pal.numColors - 1; c >= 0; c--) {
	    RGB565 color = pal.colors[c];
	    int index = findColor(color, maxLUTIndex);

	    if (index < 0) {
		// Don't have this color yet, or it isn't reachable.
		// Add it to a temporary list of colors that we need
		missing[numMissing++] = color;

	    } else {
		// We already have this color in the LUT! Bump it to the head of the MRU list.

		for (unsigned i = 0; i < LUT_MAX - 1; i++)
		    if (mru[i] == index) {
			bumpMRU(i, index);
			break;
		    }
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

	while (numMissing) {

	    // Find the oldest index that is reachable by this tile's LUT
	    unsigned index;
	    for (unsigned i = 0; i < LUT_MAX; i++) {
		index = mru[i];
		if (index <= maxLUTIndex) {
		    bumpMRU(i, index);
		    break;
		}
	    }

	    colors[index] = missing[--numMissing];
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

RLECodec4::RLECodec4()
    : runNybble(0), bufferedNybble(0), runCount(0), isNybbleBuffered(false)
    {}
    
void RLECodec4::encode(uint8_t nybble, std::vector<uint8_t>& out)
{
    if (nybble != runNybble || runCount == MAX_RUN)
	encodeRun(out);

    runNybble = nybble;
    runCount++;   
}

void RLECodec4::flush(std::vector<uint8_t>& out)
{
    encodeRun(out, true);
    if (isNybbleBuffered)
	encodeNybble(0, out);
}

void RLECodec4::encodeNybble(uint8_t value, std::vector<uint8_t>& out)
{
    if (isNybbleBuffered) {
	out.push_back(bufferedNybble | (value << 4));
	isNybbleBuffered = false;
    } else {
	bufferedNybble = value;
	isNybbleBuffered = true;
    }
}

void RLECodec4::encodeRun(std::vector<uint8_t>& out, bool terminal)
{
    if (runCount > 0) {
	encodeNybble(runNybble, out);

	if (runCount > 1) {
	    encodeNybble(runNybble, out);

	    if (runCount == 2 && !terminal)
		// Null runs can be omitted at the end of an encoding block
		encodeNybble(0, out);

	    else if (runCount > 2)
		encodeNybble(runCount - 2, out);
	}
    }

    runCount = 0;
}

TileCodec::TileCodec()
    : opIsBuffered(false), statBucket(TilePalette::CM_INVALID), p16run(0xFFFFFFFF)
{
    memset(&stats, 0, sizeof stats);
    tileCount = 0;
}

void TileCodec::encode(const TileRef tile, std::vector<uint8_t>& out)
{
    /*
     * First off, encode LUT changes.
     */

    const TilePalette &pal = tile->palette();
    uint16_t newColors;
    lut.encode(pal, newColors);
    if (newColors)
	encodeLUT(newColors, out);

    /*
     * Now encode the tile bitmap data. We do this in a
     * format-specific manner, then try to combine the result with our
     * current opcode if we can.
     */

    TilePalette::ColorMode colorMode = pal.colorMode();
    uint8_t tileOpcode;

    switch (colorMode) {

	/* Trivial one-color tile. Just emit a bare OP_TILE_P0 opcode. */
    case TilePalette::CM_LUT1:
	encodeOp(OP_TILE_P0 | lut.findColor(pal.colors[0]), out);
	newStatsTile(colorMode);
	return;

	/* Repeatable tile types */
    case TilePalette::CM_LUT2:	tileOpcode = OP_TILE_P1_R4;	break;
    case TilePalette::CM_LUT4:	tileOpcode = OP_TILE_P2_R4;	break;
    case TilePalette::CM_LUT16:	tileOpcode = OP_TILE_P4_R4;	break;
    default:			tileOpcode = OP_TILE_P16_RM;	break;
    }

    /*
     * Emit a new opcode only if we have to break this run. Otherwise,
     * extend our existing tile run.
     */

    if (!opIsBuffered
	|| tileOpcode != (opcodeBuf & OP_MASK)
	|| (opcodeBuf & ARG_MASK) == ARG_MASK)
	encodeOp(tileOpcode, out);
    else
	opcodeBuf++;

    /*
     * Format-specific tile encoders
     */

    newStatsTile(colorMode);

    switch (tileOpcode) {
    case OP_TILE_P1_R4:		encodeTileRLE4(tile, 1);	break;
    case OP_TILE_P2_R4:		encodeTileRLE4(tile, 2);	break;
    case OP_TILE_P4_R4:		encodeTileRLE4(tile, 4);	break;
    case OP_TILE_P16_RM:        encodeTileMasked16(tile);	break;
    }
}

void TileCodec::newStatsTile(unsigned bucket)
{
    // Collect stats per-colormode
    statBucket = bucket;
    tileCount++;
}

void TileCodec::flush(std::vector<uint8_t>& out)
{
    if (opIsBuffered) {
	if (statBucket != TilePalette::CM_INVALID) {
	    stats[statBucket].opcodes++;
	    stats[statBucket].dataBytes += dataBuf.size();
	    stats[statBucket].tiles += tileCount;
	    tileCount = 0;
	}

	rle.flush(dataBuf);

	out.push_back(opcodeBuf);
	out.insert(out.end(), dataBuf.begin(), dataBuf.end());
	dataBuf.clear();
	opIsBuffered = false;
    }
}

void TileCodec::encodeOp(uint8_t op, std::vector<uint8_t>& out)
{
    flush(out);

    opIsBuffered = true;
    opcodeBuf = op;
}

void TileCodec::encodeLUT(uint16_t newColors, std::vector<uint8_t>& out)
{
    if (newColors & (newColors - 1)) {
	/*
	 * More than one new color. Emit OP_LUT16.
	 */

	encodeOp(OP_LUT16, out);
	encodeWord(newColors);

	for (unsigned index = 0; index < 16; index++)
	    if (newColors & (1 << index))
		encodeWord(lut.colors[index].value);

    } else {
	/*
	 * Exactly one new color. Use OP_LUT1.
	 */

	for (unsigned index = 0; index < 16; index++)
	    if (newColors & (1 << index)) {
		encodeOp(OP_LUT1 | index, out);
		encodeWord(lut.colors[index].value);
		break;
	    }
    }
}

void TileCodec::encodeWord(uint16_t w)
{
    dataBuf.push_back((uint8_t) w);
    dataBuf.push_back((uint8_t) (w >> 8));
}

void TileCodec::encodeTileRLE4(const TileRef tile, unsigned bits)
{
    /*
     * Simple indexed-color tiles, compressed using our 4-bit RLE codec.
     */
    
    uint8_t nybble = 0;
    unsigned pixelIndex = 0;
    unsigned bitIndex = 0;
 
    while (pixelIndex < Tile::PIXELS) {
	uint8_t color = lut.findColor(tile->pixel(pixelIndex));
	assert(color < (1 << bits));
	nybble |= color << bitIndex;

	pixelIndex++;
	bitIndex += bits;

	if (bitIndex == 4) {
	    rle.encode(nybble, dataBuf);
	    nybble = 0;
	    bitIndex = 0;
	}
    }
}

void TileCodec::encodeTileMasked16(const TileRef tile)
{
    /*
     * This is a different form of RLE, which encodes repeats at a
     * pixel granularity, but with interleaved repeat information and
     * pixel data.
     *
     * We emit rows which begin with an 8-bit mask, containing between
     * 0 and 8 pixel values. A '1' bit in the mask corresponds to a
     * new pixel value, and a '0' is copied from the previous
     * value. Color runs may persist across the entire load stream.
     */

    for (unsigned y = 0; y < Tile::SIZE; y++) {
	uint8_t mask = 0;

	for (unsigned x = 0; x < Tile::SIZE; x++)
	    if (tile->pixel(x, y).value != p16run) {
		mask |= 1 << x;
		p16run = tile->pixel(x, y).value;
	    }

	dataBuf.push_back(mask);

	for (unsigned x = 0; x < Tile::SIZE; x++)
	    if (mask & (1 << x))
		encodeWord(tile->pixel(x, y).value);
    }
}

void TileCodec::dumpStatistics(Logger &log)
{
    log.infoBegin("Tile encoder statistics");

    for (int m = 0; m < TilePalette::CM_COUNT; m++) {
	unsigned compressedSize = stats[m].dataBytes + stats[m].opcodes;
	unsigned uncompressedSize = stats[m].tiles * (Tile::PIXELS * 2);
	double ratio = uncompressedSize ? 100.0 - compressedSize * 100.0 / uncompressedSize : 0;

	log.infoLine("%10s: % 4u ops, % 4u tiles, % 5u bytes, % 5.01f%% compression",
		     TilePalette::colorModeName((TilePalette::ColorMode) m),
		     stats[m].opcodes,
		     stats[m].tiles,
		     stats[m].dataBytes,
		     ratio);
    }

    log.infoEnd();
}

void TileCodec::address(uint32_t linearAddress, std::vector<uint8_t>& out)
{
    encodeOp(OP_ADDRESS, out);
    dataBuf.push_back((uint8_t) ((linearAddress >> 7) << 1));
    dataBuf.push_back((uint8_t) ((linearAddress >> 14) << 1));
    flush(out);
}

void TileCodec::end(std::vector<uint8_t>& out)
{
    encodeOp(OP_END, out);
    flush(out);
}
