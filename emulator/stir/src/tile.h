/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * STIR -- Sifteo Tiled Image Reducer
 * M. Elizabeth Scott <beth@sifteo.com>
 *
 * Copyright <c> 2011 Sifteo, Inc. All rights reserved.
 */

#ifndef _TILE_H
#define _TILE_H

#include <stdint.h>
#include <tr1/memory>
#include <color.h>

class Tile;
class TileStack;
typedef std::tr1::shared_ptr<Tile> TileRef;
typedef std::tr1::shared_ptr<TileStack> TileStackRef;


/*
 * TilePalette --
 *
 *    The color palette for one single tile. This is a small utility
 *    object that we use as part of the tile order optimization, in
 *    order to search for runs of tiles with common colors.
 *
 *    Tiles will only have indexed color palettes if we have
 *    LUT_MAX or fewer distinct colors. If numColors is greater
 *    than LUT_MAX, the colors[] array is not valid.
 */

struct TilePalette {
    TilePalette();

    static const unsigned LUT_MAX = 16;

    uint8_t numColors;
    RGB565 colors[LUT_MAX];

    enum ColorMode {
	CM_INVALID = 0,
	CM_LUT1,
	CM_LUT2,
	CM_LUT4,
	CM_LUT16,
	CM_TRUE,
    };

    ColorMode colorMode() const {
	if (numColors <= 1)  return CM_LUT1;
	if (numColors <= 2)  return CM_LUT2;
	if (numColors <= 4)  return CM_LUT4;
	if (numColors <= 16) return CM_LUT16;
	return CM_TRUE;
    }

    bool hasLUT() const {
	// Do we have a color LUT at all?
	return numColors <= LUT_MAX;
    }

    bool hasColor(RGB565 c) const {
	// Is a particular color in the LUT?
	if (hasLUT())
	    for (unsigned i = 0; i < numColors; i++)
		if (colors[i] == c)
		    return true;
	return false;
    }
};


/*
 * Tile --
 *
 *    One fixed-size image tile, in palettized RGB565 color. 
 *    Tile objects are immutable after they are initially created.
 */

class Tile {
 public:
    Tile(uint8_t *rgba, size_t stride);

    static const unsigned SIZE = 8;       // Number of pixels on a side
    static const unsigned PIXELS = 64;    // Total pixels in a tile

    // Firmware chroma-key color
    static const uint16_t CHROMA_KEY = 0x4FF5;

    void render(uint8_t *rgba, size_t stride) const;
   
    bool usingChromaKey() const {
	return mUsingChromaKey;
    }

    RGB565 pixel(unsigned i) const {
	return mPixels[i];
    }

    RGB565 pixel(unsigned x, unsigned y) const {
	return mPixels[x + y * SIZE];
    }

    RGB565 pixelWrap(unsigned x, unsigned y) const {
	return pixel(x & 7, y & 7);
    }

    // http://en.wikipedia.org/wiki/Sobel_operator
    double sobelGx(unsigned x, unsigned y) const {
	return ( - CIELab(pixelWrap(x-1, y-1)).L
		 + CIELab(pixelWrap(x+1, y-1)).L
		 -2* CIELab(pixelWrap(x-1, y)).L
		 +2* CIELab(pixelWrap(x+1, y)).L
		 - CIELab(pixelWrap(x-1, y+1)).L
		 + CIELab(pixelWrap(x+1, y+1)).L );
    }
    double sobelGy(unsigned x, unsigned y) const {
	return ( - CIELab(pixelWrap(x-1, y-1)).L
		 + CIELab(pixelWrap(x-1, y+1)).L
		 -2* CIELab(pixelWrap(x, y-1)).L
		 +2* CIELab(pixelWrap(x, y+1)).L
		 - CIELab(pixelWrap(x+1, y-1)).L
		 + CIELab(pixelWrap(x+1, y+1)).L );
    }

    const TilePalette &palette() {
	// Lazily build the palette info
	if (!mPalette.numColors)
	    constructPalette();
	return mPalette;
    }	

    double errorMetric(const Tile &other) const;
    double meanSquaredError(const Tile &other, int scale=1) const;
    double sobelError(const Tile &other) const;

    TileRef reduce(ColorReducer &reducer, double maxMSE) const;

 private:
    Tile();
    Tile(bool usingChromaKey);
    void constructPalette(void);

    friend class TileStack;

    RGB565 mPixels[PIXELS];
    bool mUsingChromaKey;
    TilePalette mPalette;
};


/*
 * TileStack --
 *
 *    A stack of similar tiles, represented at any given time by a tile
 *    created via a per-pixel median operation on every tile in the set.
 *
 *    When the optimizer finds a tile that's similar to this set, it
 *    can add it to the set in order to statistically incorporate that
 *    tile's pixels into the median image we'll eventually generate
 *    for that set of tiles.
 */

class TileStack {
 public:
    void add(TileRef t);
    TileRef median();

 private:
    static const unsigned MAX_SIZE = 128;

    friend class TilePool;

    std::vector<TileRef> tiles;
    TileRef cache;
    TileRef optimized;
};


/*
 * TilePool --
 *
 *    An independent pool of tiles, supporting lossless or lossy optimization.
 */

class TilePool {
 public:
    TilePool(double _maxMSE)
        : maxMSE(_maxMSE), totalTiles(0) {}

    TileStackRef add(TileRef t);
    TileStackRef closest(TileRef t, double &mse);
    void optimize();

    void render(uint8_t *rgba, size_t stride, unsigned width);

    unsigned size() {
	return sets.size();
    }

 private:
    typedef std::list<TileStackRef> Collection;
    Collection sets;

    unsigned totalTiles;
    double maxMSE;

    void optimizePalette();

    void optimizeOrder();
    unsigned orderingCost();
};


/*
 * TileGrid --
 *
 *    An image, converted into a matrix of TileStack references.
 */

class TileGrid {
 public:
    TileGrid(TilePool *pool);

    void load(uint8_t *rgba, size_t stride, unsigned width, unsigned height);
    bool load(const char *pngFilename);
    
    void render(uint8_t *rgba, size_t stride);

    unsigned width() {
	return mWidth;
    }

    unsigned height() {
	return mHeight;
    }

    TileStackRef tile(unsigned x, unsigned y) {
	return tiles[x + y * mWidth];
    }

 private:
    TilePool *mPool;
    unsigned mWidth, mHeight;
    std::vector<TileStackRef> tiles;
};

#endif
