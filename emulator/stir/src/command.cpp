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
#include <sys/time.h>

#include "lodepng.h"
#include "tile.h"

int main(int argc, char **argv) {
    ConsoleLogger log;

    if (argc != 4) {
	fprintf(stderr, "usage: %s in.png out.png mse\n", argv[0]);
	return 1;
    }

    struct timeval startTime;
    gettimeofday(&startTime, NULL);

    CIELab::initialize();

    double maxMSE = atof(argv[3]);
    TilePool pool = TilePool(maxMSE);
    TileGrid tg = TileGrid(&pool);

    tg.load(argv[1]);
    pool.optimize(log);

    std::vector<uint8_t> image;
    unsigned width = Tile::SIZE * tg.width() * 2;
    unsigned height = Tile::SIZE * tg.height();
    size_t pitch = width * 4;
    image.resize(pitch * height);

    std::vector<uint8_t> loadstream;
    pool.encode(loadstream, &log);
    LodePNG::saveFile(loadstream, "loadstream.bin");

    std::vector<uint8_t> map;
    tg.encodeMap(map);
    LodePNG::saveFile(map, "tilemap.bin");

    tg.render(&image[pitch/2], pitch);
    pool.render(&image[0], pitch, tg.width());

    LodePNG::Encoder encoder;
    std::vector<uint8_t> pngOut;
    encoder.encode(pngOut, &image[0], width, height);
    LodePNG::saveFile(pngOut, argv[2]);

    std::vector<uint8_t> zLoadstream;
    std::vector<uint8_t> zMap;
    LodePNG::compress(zLoadstream, loadstream);
    LodePNG::compress(zMap, map);

    log.infoBegin("Asset statistics");
    log.infoLine("%30s: %d", "Total tiles", pool.size());
    log.infoLine("%30s: %.02f kB", "Installed size in NOR", pool.size() * 128 / 1024.0, pool.size());
    log.infoLine("%30s: %.02f kB", "Loadstream size", loadstream.size() / 1024.0);
    log.infoLine("%30s: %.02f kB", "Compressed loadstream", zLoadstream.size() / 1024.0);
    log.infoLine("%30s: %.02f s", "Load time estimate", loadstream.size() / 40000.0);
    log.infoLine("%30s: %.02f kB", "Uncompressed map", map.size() / 1024.0);
    log.infoLine("%30s: %.02f kB", "Compressed map", zMap.size() / 1024.0);
    log.infoLine("%30s: %.02f kB", "Compressed total", (zLoadstream.size() + zMap.size()) / 1024.0);
    log.infoEnd();

    struct timeval endTime;
    gettimeofday(&endTime, NULL);
    fprintf(stderr, "\nDone in %.02f seconds.\n", 
	    (endTime.tv_sec - startTime.tv_sec) +
	    (endTime.tv_usec - startTime.tv_usec) * 1e-6);

    return 0;
}
