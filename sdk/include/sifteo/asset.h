/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * This file is part of the public interface to the Sifteo SDK.
 * Copyright <c> 2011 Sifteo, Inc. All rights reserved.
 */

#ifndef _SIFTEO_ASSET_H
#define _SIFTEO_ASSET_H

#include <stdint.h>
#include <sifteo/abi.h>
#include <sifteo/limits.h>

namespace Sifteo {

//typedef _SYSAssetGroupID AssetGroupID;

/**
 * An asset group. At build time, STIR creates a statically
 * initialized instance of the AssetGroup class for every asset group
 * in the game.
 *
 * At runtime, AssetGroup objects track the load state of a particular
 * group across all cubes.
 */

class AssetGroup {
 public:

    /**
     * Wait until this asset group is available on all cubes that it
     * was requested on via Cube::loadAssets(). Assets load
     * asynchronously, but it's sometimes necessary to block until
     * loading is done.
     */

    void wait() {
        while (sys.reqCubes & ~sys.doneCubes)
            _SYS_yield();
    }

    _SYSAssetGroup sys;
    _SYSAssetGroupCube cubes[CUBE_ALLOCATION];
};

class AssetGroupID {
 public:

    /**
     * Wait until this asset group is available on all cubes that it
     * was requested on via Cube::loadAssets(). Assets load
     * asynchronously, but it's sometimes necessary to block until
     * loading is done.
     */

    void wait() {
        while (sys.reqCubes & ~sys.doneCubes)
            _SYS_yield();
    }

    _SYSAssetGroupID sys;
    _SYSAssetGroupCube cubes[CUBE_ALLOCATION];
};

/**
 * A plain, tile-mapped asset image.
 *
 * XXX: Find a better binary representation of these, probably with some
 *      random-access-friendly form of dictionary compression. Right now
 *      this is very much just a placeholder!
 */

class AssetImage {
 public:
    unsigned width;
    unsigned height;
    unsigned frames;

    const uint16_t *tiles;
};


/**
 * An asset image in which all tiles are stored sequentially in memory.
 * No map is necessary, just an index for the first tile in the sequence.
 */
 
class PinnedAssetImage {
 public:
    unsigned width;
    unsigned height;
    unsigned frames;

    uint16_t index;
};
    
    
};  // namespace Sifteo

#endif
