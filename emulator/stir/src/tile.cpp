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
#include "tile.h"
#include "lodepng.h"

Tile::Tile(uint8_t *rgba, size_t stride)
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

double Tile::meanSquaredError(Tile &other)
{
    /*
     * Measure the Mean Squared Error between every corresponding
     * pixel in the two tiles.
     */

    double error = 0;
    for (unsigned i = 0; i < PIXELS; i++)
	error += CIELab(mPixels[i]).meanSquaredError(CIELab(other.mPixels[i]));

    return error / PIXELS;
}

void Tile::render(uint8_t *rgba, size_t stride)
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

TileStack *TilePool::add(TileRef t)
{
    /*
     * Add a new tile to the pool, and try to optimize it if we can.
     *
     * Returns a TileStack reference which can be used now or
     * later (within the lifetime of the TilePool) to retrieve the
     * latest median for all tiles we've consolidated with the
     * provided one, and to refer to this particular unique set of
     * tiles.
     */

    double mse;
    TileStack *c = closest(t, mse);

    if (c == NULL || mse > maxMSE) {
	// Too far away. Start a new set.
	sets.push_front(TileStack());
	c = &*sets.begin();
	fprintf(stderr, "Optimizing tiles... %d sets\n", (int) sets.size());
    }

    c->add(t);
    return c;
}

TileStack *TilePool::closest(TileRef t, double &mse)
{
    /*
     * Search for the closest tile set for the provided tile image.
     * Returns the tile set itsef, if any was found, and the MSE
     * between the provided tile and that tile's current median.
     */

    double distance = DBL_MAX;
    TileStack *closest = NULL;

    for (std::list<TileStack>::iterator i = sets.begin(); i != sets.end(); i++) {
	double err = i->median()->meanSquaredError(*t);
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
	    TileRef t = tile(x, y)->median();
	    t->render(rgba + (x * Tile::SIZE * 4) + (y * Tile::SIZE * stride), stride);
	}
}
