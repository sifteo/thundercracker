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
#include "color.h"

/*
 * Tile --
 *
 *    One fixed-size image tile, in palettized RGB565 color.
 */

class Tile {
 public:
    Tile(uint8_t *rgba, uint32_t stride);

    static const SIZE = 8;       // Number of pixels on a side
    static const PIXELS = 64;    // Total pixels in a tile

 private:
    RGB565 pixels[PIXELS];
};

#endif
