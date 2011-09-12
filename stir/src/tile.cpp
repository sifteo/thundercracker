/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * STIR -- Sifteo Tiled Image Reducer
 * M. Elizabeth Scott <beth@sifteo.com>
 *
 * Copyright <c> 2011 Sifteo, Inc. All rights reserved.
 */

#include <stdio.h>
#include <float.h>
#include <algorithm>
#include <set>
#include <map>

#include "tile.h"
#include "tilecodec.h"
#include "lodepng.h"

namespace Stir {


Tile::Tile(bool usingChromaKey)
    : mUsingChromaKey(usingChromaKey), mHasSobel(false), mHasDec4(false)
    {}

Tile::Tile(uint8_t *rgba, size_t stride)
    : mUsingChromaKey(false), mHasSobel(false), mHasDec4(false)
{
    /*
     * Load the tile image from a full-color RGBA source bitmap.
     */

    const uint8_t alphaThreshold = 0x80;
    uint8_t *row, *pixel;
    unsigned x, y;

    // First pass.. are there any transparent pixels?
    mUsingChromaKey = false;
    for (row = rgba, y = SIZE; y; --y, row += stride)
	for (pixel = row, x = SIZE; x; --x, pixel += 4)
	    if (pixel[3] < alphaThreshold)
		mUsingChromaKey = true;
    

    // Second pass.. convert to RGB565, possibly with colorkey.
    RGB565 *dest = mPixels;
    for (row = rgba, y = SIZE; y; --y, row += stride)
	for (pixel = row, x = SIZE; x; --x, pixel += 4) {
	    RGB565 color = RGB565(pixel);

	    if (!mUsingChromaKey) {
		// No transparency in the image, we're allowed to use any color.
		*dest = color;
	    }
	    else if (pixel[3] < alphaThreshold) {
		// Pixel is actually transparent
		*dest = CHROMA_KEY;
	    }
	    else if ((color.value & 0xFF) == (CHROMA_KEY & 0xFF)) {
		/*
		 * Pixel isn't transparent, but it would look like
		 * the chromakey to our firmware's 8-bit comparison.
		 * Modify the color slightly.
		 */
		*dest = color.wiggle();
	    }
	    else {
		// Opaque pixel
		*dest = color;
	    }

	    dest++;
	}	    
}

void Tile::constructPalette(void)
{
    /*
     * Create a TilePalette for this particular tile. If the tile has few enough colors
     * that we can make a LUT, the palette will include a populated LUT. Additionally,
     * as a heuristic to help us provide color matches across transitions from higher
     * tile color depth to lower color depth, we try to order the most popular colors
     * at the begining of the LUT.
     */

    // Tracks unique colors, and their frequencies
    std::map<RGB565, unsigned> colors;

    for (unsigned i = 0; i < PIXELS; i++)
	colors[mPixels[i]] = colors[mPixels[i]] + 1;

    mPalette.numColors = colors.size();

    if (mPalette.hasLUT()) {
	// Sort the colors using an inverse mapping.

	std::multimap<unsigned, RGB565> lutSorter;
	for (std::map<RGB565, unsigned>::iterator i = colors.begin(); i != colors.end(); i++)
	    lutSorter.insert(std::pair<unsigned, RGB565>(i->second, i->first));

	unsigned index = 0;
	for (std::multimap<unsigned, RGB565>::reverse_iterator i = lutSorter.rbegin();
	     i != lutSorter.rend(); i++)
	    mPalette.colors[index++] = i->second;
    }
}

void Tile::constructSobel()
{
    /*
     * Build a Sobel gradient approximation matrix for this tile.
     *
     * See: http://en.wikipedia.org/wiki/Sobel_operator
     */

    mHasSobel = true;
    mSobelTotal = 0;

    unsigned i = 0;
    for (unsigned y = 0; y < SIZE; y++)
	for (unsigned x = 0; x < SIZE; x++, i++) {

	    // Luminance of eight neighbor pixels
	    float l00 = CIELab(pixelWrap(x-1, y-1)).L;
	    float l10 = CIELab(pixelWrap(x  , y-1)).L;
	    float l20 = CIELab(pixelWrap(x+1, y-1)).L;
	    float l01 = CIELab(pixelWrap(x-1, y  )).L;
	    float l21 = CIELab(pixelWrap(x+1, y  )).L;
	    float l02 = CIELab(pixelWrap(x-1, y+1)).L;
	    float l12 = CIELab(pixelWrap(x  , y+1)).L;
	    float l22 = CIELab(pixelWrap(x+1, y+1)).L;

	    mSobelGx[i] = -l00 +l20 -l01 -l01 +l21 +l21 -l02 +l22;
	    mSobelGy[i] = -l00 +l02 -l10 -l10 +l12 +l12 -l20 +l22;

	    mSobelTotal += mSobelGx[i] * mSobelGx[i];
	    mSobelTotal += mSobelGy[i] * mSobelGy[i];
	}

#ifdef DEBUG_SOBEL
    for (i = 0; i < PIXELS; i++) {
	int x = std::max(0, std::min(255, (int)(128 + mSobelGx[i])));
	int y = std::max(0, std::min(255, (int)(128 + mSobelGy[i])));
	mPixels[i] = RGB565(x, y, (x+y)/2);
    }
#endif
}    

void Tile::constructDec4()
{
    /*
     * Build a decimated 2x2 pixel representation of this tile, using
     * averaged CIELab colors.
     */

    const unsigned scale = SIZE / 2;
    unsigned i = 0;

    mHasDec4 = true;

    for (unsigned y1 = 0; y1 < SIZE; y1 += scale)
	for (unsigned x1 = 0; x1 < SIZE; x1 += scale) {
	    CIELab acc;

	    // Y/X pixels
	    for (unsigned y2 = y1; y2 < y1 + scale; y2++)
		for (unsigned x2 = x1; x2 < x1 + scale; x2++)
		    acc += pixel(x2, y2);
	    
	    acc /= scale * scale;
	    mDec4[i++] = acc;
	}

#ifdef DEBUG_DEC4
    for (unsigned y = 0; y < SIZE; y++)
	for (unsigned x = 0; x < SIZE; x++)
	    mPixels[x + (y * SIZE)] = mDec4[x/scale + y/scale * 2].rgb();
#endif
}

double Tile::errorMetric(Tile &other, double limit)
{
    /*
     * This is a rather ad-hoc attempt at a perceptually-optimized
     * error metric. It is a form of multi-scale MSE metric, plus we
     * attempt to take into account the geometric structure of a tile
     * by comparing a luminance edge detection map.
     *
     * If we exceed 'limit', the test can exit early. This lets us
     * calculate the easy metrics first, and skip the rest if we're
     * already over.
     */

    double error = 0;

    error += 0.450 * coarseMSE(other);
    if (error > limit)
	return DBL_MAX;

    error += 0.025 * fineMSE(other);
    if (error > limit)
	return DBL_MAX;

    error += 5.00 * sobelError(other);
    return error;
}

double Tile::fineMSE(Tile &other)
{
    /*
     * A normal pixel-wise mean squared error metric.
     */

    double error = 0;

    for (unsigned i = 0; i < PIXELS; i++)
	error += CIELab(mPixels[i]).meanSquaredError(CIELab(other.mPixels[i]));

    return error / PIXELS;
}

double Tile::coarseMSE(Tile &other)
{
    /*
     * A reduced scale MSE metric using the 2x2 pixel decimated version of our tile.
     */

    double error = 0;

    if (!mHasDec4)
	constructDec4();
    if (!other.mHasDec4)
	other.constructDec4();

    for (unsigned i = 0; i < 4; i++)
	error += mDec4[i].meanSquaredError(other.mDec4[i]);

    return error / 4;
}

double Tile::sobelError(Tile &other)
{
    /*
     * An error metric based solely on detecting structural luminance
     * differences using the Sobel operator.
     */

    double error = 0;

    if (!mHasSobel)
	constructSobel();
    if (!other.mHasSobel)
	other.constructSobel();

    for (unsigned i = 0; i < PIXELS; i++) {
	double gx = mSobelGx[i] - other.mSobelGx[i];
	double gy = mSobelGy[i] - other.mSobelGy[i];

	error += gx * gx + gy * gy;
    }
    
    // Contrast difference over total contrast
    return error / (1 + mSobelTotal + other.mSobelTotal);
}

void Tile::render(uint8_t *rgba, size_t stride) const
{
    // Draw this tile to an RGBA framebuffer, for proofing purposes

    unsigned x, y;
    uint8_t *row, *dest;

    for (y = 0, row = rgba; y < SIZE; y++, row += stride)
	for (x = 0, dest = row; x < SIZE; x++, dest += 4) {
	    RGB565 color = pixel(x, y);
	    dest[0] = color.red();
	    dest[1] = color.green();
	    dest[2] = color.blue();
	    dest[3] = 0xFF;
	}
}

TileRef Tile::reduce(ColorReducer &reducer, double maxMSE) const
{
    /*
     * Reduce a tile's color palette, using a completed optimized
     * ColorReducer instance.  Uses maxMSE to provide hysteresis for
     * the color selections, emphasizing color runs when possible.
     */

    TileRef result = TileRef(new Tile(mUsingChromaKey));
    RGB565 run;

    // Hysteresis amount
    maxMSE *= 0.9;

    for (unsigned i = 0; i < PIXELS; i++) {
	RGB565 color = reducer.nearest(mPixels[i]);
	double error = CIELab(color).meanSquaredError(CIELab(run));
	if (error > maxMSE)
	    run = color;
	result->mPixels[i] = run;
    }

    return result;
}

TilePalette::TilePalette()
    : numColors(0)
    {}

const char *TilePalette::colorModeName(ColorMode m)
{
    switch (m) {
    case CM_LUT1:	return "CM_LUT1";
    case CM_LUT2:	return "CM_LUT2";
    case CM_LUT4:	return "CM_LUT4";
    case CM_LUT16:	return "CM_LUT16";
    case CM_TRUE:	return "CM_TRUE";
    default:		return "<invalid>";
    }
}

void TileStack::add(TileRef t)
{
    tiles.push_back(t);
    cache = TileRef();
}

TileRef TileStack::median()
{
    /*
     * Create a new tile based on the per-pixel median of every tile in the set.
     */

    if (cache)
	return cache;

    Tile *t = new Tile();
    std::vector<RGB565> colors(tiles.size());

    // The median algorithm repeats independently for every pixel in the tile.
    for (unsigned i = 0; i < Tile::PIXELS; i++) {

	// Collect possible colors for this pixel
	for (unsigned j = 0; j < tiles.size(); j++)
	    colors[j] = tiles[j]->pixel(i);

	// Sort along the major axis
	int major = CIELab::findMajorAxis(&colors[0], colors.size());
	std::sort(colors.begin(), colors.end(),
		  CIELab::sortAxis(major));

	// Pick the median color
	t->mPixels[i] = colors[colors.size() >> 1];
    }

    cache = TileRef(t);

    /*
     * Some individual tiles will receive a huge number of references.
     * This is a bit of a hack to prevent a single tile stack from growing
     * boundlessly. If we just calculated a median for a stack over a preset
     * maximum size, we replace the entire stack with copies of the median
     * image, in order to give that median significant (but not absolute)
     * statistical weight.
     *
     * The problem with this is that a tile's median is no longer
     * computed globally across the entire asset group, but is instead
     * more of a sliding window. But for the kinds of heavily repeated
     * tiles that this algorithm will apply to, maybe this isn't an
     * issue?
     */

    if (tiles.size() > MAX_SIZE) {
	tiles = std::vector<TileRef>(MAX_SIZE / 2, cache);
    }

    return cache;
}

TileStack* TilePool::closest(TileRef t, double &mse)
{
    /*
     * Search for the closest tile set for the provided tile image.
     * Returns the tile stack, if any was found, and the MSE
     * between the provided tile and that tile's current median.
     */

    double distance = DBL_MAX;
    TileStack *closest = NULL;

    for (std::list<TileStack>::iterator i = stackList.begin(); i != stackList.end(); i++) {
	double err = i->median()->errorMetric(*t, distance);
	if (err < distance) {
	    distance = err;
	    closest = &*i;
	}
    }

    mse = distance;
    return closest;
}

TileGrid::TileGrid(TilePool *pool)
    : mPool(pool), mWidth(0), mHeight(0)
    {}

void TileGrid::load(uint8_t *rgba, size_t stride, unsigned width, unsigned height)
{
    mWidth = width / Tile::SIZE;
    mHeight = height / Tile::SIZE;
    tiles.resize(mWidth * mHeight);

    for (unsigned y = 0; y < mHeight; y++)
	for (unsigned x = 0; x < mWidth; x++) {
	    TileRef t = TileRef(new Tile(rgba + (x * Tile::SIZE * 4) +
					 (y * Tile::SIZE * stride), stride));
	    tiles[x + y * mWidth] = mPool->add(t);
	}
}

bool TileGrid::load(const char *pngFilename)
{
    std::vector<uint8_t> image;
    std::vector<uint8_t> png;
    LodePNG::Decoder decoder;
 
    LodePNG::loadFile(png, pngFilename);
    if (png.empty())
	return false;

    decoder.decode(image, png);
    if (image.empty())
	return false;

    load(&image[0], decoder.getWidth() * 4, decoder.getWidth(), decoder.getHeight());

    return true;
}

void TileGrid::render(uint8_t *rgba, size_t stride)
{
    // Draw this image to an RGBA framebuffer, for proofing purposes
    
    for (unsigned y = 0; y < mHeight; y++)
	for (unsigned x = 0; x < mWidth; x++) {
	    TileRef t = mPool->tile(mPool->index(tile(x, y)));
	    t->render(rgba + (x * Tile::SIZE * 4) + (y * Tile::SIZE * stride), stride);
	}
}

void TileGrid::encodeMap(std::vector<uint8_t> &out, uint32_t baseAddress)
{
    for (unsigned y = 0; y < mHeight; y++)
	for (unsigned x = 0; x < mWidth; x++) {
	    TilePool::Index index = mPool->index(tile(x, y));

	    // Split 7:7:7 address calculations
	    uint32_t address = baseAddress + (index << 7);
	    uint8_t high = (address >> 14) << 1;
	    uint8_t low = (address >> 7) << 1;
	    
	    out.push_back(low);
	    out.push_back(high);
	}
}

void TilePool::optimize(Logger &log)
{
    /*
     * Global optimizations to apply after filling a tile pool.
     */

    if (maxMSE > 0)
	optimizePalette(log);

    optimizeTiles(log);
    optimizeOrder(log);
}

void TilePool::optimizePalette(Logger &log)
{
    /*
     * Palette optimization, for use after add() but before optimizeTiles().
     *
     * This operates on all tile images in 'tiles', replacing each
     * with a reduced-color version created using a global color
     * palette optimization process.
     */

    ColorReducer reducer;

    // First, add ALL tile data to the reducer's pool    
    for (std::vector<TileRef>::iterator i = tiles.begin(); i != tiles.end(); i++)
	for (unsigned j = 0; j < Tile::PIXELS; j++)
	    reducer.add((*i)->pixel(j));

    // Ask the reducer to do its own (slow!) global optimization
    reducer.reduce(maxMSE, log);

    // Now reduce each tile, using the agreed-upon color palette
    for (std::vector<TileRef>::iterator i = tiles.begin(); i != tiles.end(); i++)
	*i = (*i)->reduce(reducer, maxMSE);
}

void TilePool::optimizeTiles(Logger &log)
{
    /*
     * The tile optimizer is a greedy algorithm that works in two
     * passes: First, we collect tiles into stacks, using an MSE
     * comparison based on the stack's *current* median at the
     * time. This produces a good estimate of the stacks we'll need,
     * but there is no guarantee that the stack we place a tile in
     * will be the most optimal stack for that tile, or that each
     * stack median turns out to be unique.
     *
     * After this collection pass, we run a second pass which does not
     * modify the stored medians, but just chooses the best stack for
     * each input tile. In this stage, it is possible that not every
     * input stack will be used.
     *
     * Collect individual tiles, from "tiles", into TileStacks, stored
     * in 'stackList' and 'stackIndex'.
     */

    log.taskBegin("Gathering tiles");
    stackList.clear();
    optimizeTilesPass(true, log);
    log.taskEnd();

    log.taskBegin("Optimizing tiles");
    stackIndex.clear();
    optimizeTilesPass(false, log);
    log.taskEnd();
}
    
void TilePool::optimizeTilesPass(bool gather, Logger &log)
{
    // A single pass from the two-pass optimizeTiles() algorithm

    std::set<TileStack *> activeStacks;

    for (std::vector<TileRef>::iterator t = tiles.begin(); t != tiles.end();) {
	double mse;
	TileStack *c = closest(*t, mse);
	
	if (gather) {
	    // Create or augment a TileStack

	    if (!c || mse > maxMSE) {
		stackList.push_back(TileStack());
		c = &stackList.back();
	    }
	    c->add(*t);

	} else {
	    // Remember this stack, we've selected it for good.

	    stackIndex.push_back(c);	
	    activeStacks.insert(c);
	}

	t++;
	if (t == tiles.end() || !(stackIndex.size() % 128)) {
	    unsigned stacks = gather ? stackList.size() : activeStacks.size();
	    log.taskProgress("%u stacks (%.03f%% compression)", stacks,
			     100.0 - stacks * 100.0 / tiles.size());
	}
    }

    if (!gather) {
	// Permanently delete unused stacks

	std::list<TileStack>::iterator i = stackList.begin();

	while (i != stackList.end()) {
	    std::list<TileStack>::iterator j = i;
	    i++;

	    if (!activeStacks.count(&*j))
		stackList.erase(j);
	}
    }
}

void TilePool::optimizeOrder(Logger &log)
{
    /*
     * Optimize the order of our tiles within the pool. It turns out
     * that this is a very computationally difficult problem- it's
     * actually an asymmetric travelling salesman problem!
     *
     * It isn't even remotely computationally feasible to come up with
     * a globally optimal solution, so we just use a greedy heuristic
     * which tries to pick the best next tile. This uses
     * TileCodecLUT::encode() as a cost metric.
     *
     * This is where we assign an index to each stack, and build the
     * stackArray.
     */

    std::list<TileStack> newOrder;
    TileCodecLUT codec;
    unsigned totalCost = 0;

    log.taskBegin("Optimizing tile order");

    stackArray.clear();

    while (!stackList.empty()) {

	unsigned bestCost = (unsigned) -1;
	std::list<TileStack>::iterator bestIter;

	// Use a forked copy of the codec to ask what the cost would be for each possible choice
	for (std::list<TileStack>::iterator i = stackList.begin(); i != stackList.end(); i++) {
	    TileCodecLUT codecFork = codec;
	    unsigned cost = codecFork.encode(i->median()->palette());
	    if (cost < bestCost) {
		bestCost = cost;
		bestIter = i;
	    }
	}

	// Apply the new codec state permanently
	totalCost += codec.encode(bestIter->median()->palette());

	// Pick the best tile, and assign it a permanent index
	bestIter->index = stackArray.size();
	stackArray.push_back(&*bestIter);
	newOrder.splice(newOrder.end(), stackList, bestIter);

	if (!(stackList.size() % 128))
	    log.taskProgress("%d tiles (cost %d)",
			     (int) newOrder.size(), totalCost);
    }

    stackList.swap(newOrder);

    log.taskEnd();
}

void TilePool::render(uint8_t *rgba, size_t stride, unsigned width)
{
    // Draw all tiles in this pool to an RGBA framebuffer, for proofing purposes
    
    unsigned x = 0, y = 0;
    for (std::list<TileStack>::iterator i = stackList.begin(); i != stackList.end(); i++) {
	TileRef t = i->median();
	t->render(rgba + (x * Tile::SIZE * 4) + (y * Tile::SIZE * stride), stride);

	x++;
	if (x == width) {
	    x = 0;
	    y++;
	}
    }
}

void TilePool::encode(std::vector<uint8_t>& out, Logger *log)
{
    TileCodec codec(out);

    // XXX: Each pool should have a configurable base address
    FlashAddress addr = 0;
    unsigned blocks = FlashAddress::tilesToBlocks(stackList.size());

    if (log) {
	log->taskBegin("Encoding tiles");
	log->taskProgress("Starting at 0x%06x, erasing %d blocks",
			  addr.linear, blocks);
    }

    codec.address(addr);
    for (std::list<TileStack>::iterator i = stackList.begin(); i != stackList.end(); i++)
	codec.encode(i->median(), true);
    codec.flush();

    if (log) {
	log->taskEnd();
	codec.dumpStatistics(*log);
    }
}

};  // namespace Stir
