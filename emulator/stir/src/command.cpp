/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * STIR -- Sifteo Tiled Image Reducer
 * M. Elizabeth Scott <beth@sifteo.com>
 *
 * Copyright <c> 2011 Sifteo, Inc. All rights reserved.
 */

/*
 * Command line front-end.
 */

#include <stdio.h>
#include <stdlib.h>
#include "tile.h"
#include "lodepng.h"

int main(int argc, char **argv) {

    if (argc != 4) {
	fprintf(stderr, "usage: %s in.png out.png mse\n", argv[0]);
	return 1;
    }

    CIELab::initialize();

    double maxMSE = atof(argv[3]);
    TilePool pool = TilePool(maxMSE);
    TileGrid tg = TileGrid(&pool);

    tg.load(argv[1]);
    pool.optimize();

    std::vector<uint8_t> image;
    image.resize(4 * Tile::PIXELS * tg.width() * tg.height());
    tg.render(&image[0], 4 * Tile::SIZE * tg.width());

    LodePNG::Encoder encoder;
    std::vector<uint8_t> pngOut;
    encoder.encode(pngOut, &image[0], tg.width() * Tile::SIZE, tg.height() * Tile::SIZE);
    LodePNG::saveFile(pngOut, argv[2]);

    return 0;
}
