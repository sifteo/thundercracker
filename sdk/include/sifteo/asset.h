/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * This file is part of the public interface to the Sifteo SDK.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#ifndef _SIFTEO_ASSET_H
#define _SIFTEO_ASSET_H

#ifdef NO_USERSPACE_HEADERS
#   error This is a userspace-only header, not allowed by the current build.
#endif

#include <sifteo/abi.h>
#include <sifteo/limits.h>

namespace Sifteo {


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
     * Is this asset group still being downloaded?
     */

    bool isLoading() {
        return (sys.reqCubes & ~sys.doneCubes) != 0;
    }

    /**
     * Wait until this asset group is available on all cubes that it
     * was requested on via Cube::loadAssets(). Assets load
     * asynchronously, but it's sometimes necessary to block until
     * loading is done.
     */

    void wait() {
        while (isLoading())
            _SYS_yield();
    }

    _SYSAssetGroup sys;
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

    AssetGroup *group;
    const uint16_t *tiles;
    
    unsigned pixelWidth() const {
        return width * 8;
    }
    
    unsigned pixelHeight() const {
        return height * 8;
    }
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

    AssetGroup *group;
    uint16_t index;

    unsigned pixelWidth() const {
        return width * 8;
    }
    
    unsigned pixelHeight() const {
        return height * 8;
    }
};


/**
 * An audio asset, using any supported compression codec.
 */

class AssetAudio {
 public:
    _SYSAudioModule sys;
};


};  // namespace Sifteo

#endif
