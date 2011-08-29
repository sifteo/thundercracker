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


struct RGB565 {
    uint16_t value;

    RGB565(uint32_t rgb) {
	RGB565((uint8_t)rgba, (uint8_t)(rgba >> 8), (uint8_t)(rgba >> 16));
    }
    
    RGB565(uint8_t r, uint8_t g, uint8_t b) {
	/*
	 * Round to the nearest 5/6 bit color. Note that simple
	 * bit truncation does NOT produce the best result!
	 */
	uint16_t r5 = (uint16_t)r * 31 / 255;
	uint16_t g6 = (uint16_t)g * 63 / 255;
	uint16_t b5 = (uint16_t)b * 31 / 255;
	value = (r5 << 11) | (g6 << 5) | b5;
    }

    uint8_t red() const {
	/*
	 * A good approximation is (r5 << 3) | (r5 >> 2), but this
	 * is still not quite as accurate as the implementation here.
	 */
	uint16_t r5 = (value >> 11) & 0x1F;
	return r5 * 255 / 31;
    }

    uint8_t green() const {
	uint16_t g6 = (value >> 5) & 0x3F;
	return g6 * 255 / 63;
    }
    
    uint8_t blue() const {
	uint16_t b5 = value & 0x1F;
	return b5 * 255 / 31;
    }

    bool operator==(RGB565 &other) const {
	return value == other.value;
    }

    


};







/*
 * Tile --
 *
 *    One fixed-size image tile, in palettized RGB565 color.
 */

class Tile {
 public:
    Tile(uint32_t *rgba, uint32_t stride);

    static const SIZE = 8;       // Number of pixels on a side
    static const PIXELS = 64;    // Total pixels in a tile

 private:
    uint8_t mPixels[PIXELS];     // Palette indices for each pixel
    uint16_t mColors[PIXELS];    // Palette in RGB565 format, sorted by decreasing prevalence
    unsigned mColorCount;        // Number of colors in use
};

#endif
