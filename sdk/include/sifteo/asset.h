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
#include <sifteo/limits.h>      // For CUBE_ALLOCATION
#include <sifteo/math.h>        // For vector types
#include <sifteo/macros.h>      // For ASSERT

namespace Sifteo {


/**
 * An asset group. At build time, STIR creates a statically
 * initialized instance of the AssetGroup class for every asset group
 * in the game.
 *
 * At runtime, AssetGroup objects track the load state of a particular
 * group across all cubes.
 */

struct AssetGroup {
    _SYSAssetGroup sys;
    _SYSAssetGroupCube cubes[CUBE_ALLOCATION];

    /// Implicit conversion to system object
    operator const _SYSAssetGroup& () const { return sys; }
    operator _SYSAssetGroup& () { return sys; }

    /**
     * Return the base address of this asset group, as loaded onto the
     * specified cube. The result is only valid if the asset group is
     * already installed on that cube.
     */
    uint16_t baseAddress(_SYSCubeID cube) const {
        ASSERT(cube < CUBE_ALLOCATION);
        ASSERT((0x80000000 >> cube) & sys.doneCubes);
        return cubes[cube].baseAddr;
    }

    /**
     * Is this asset group still being downloaded?
     */
    bool isLoading() const {
        return (sys.reqCubes & ~sys.doneCubes) != 0;
    }

    /**
     * Wait until this asset group is available on all cubes that it
     * was requested on via Cube::loadAssets(). Assets load
     * asynchronously, but it's sometimes necessary to block until
     * loading is done.
     */
    void wait() const {
        while (isLoading())
            _SYS_yield();
    }
};


/**
 * Any kind of asset image, as defined in your STIR script.
 *
 * AssetImage acts as the base class for all tiled assets. It does not
 * imply any particular data representation: an AssetImage may be pinned,
 * flat, or compressed.
 *
 * We don't use actual C++ subclassing here, since we need to be able to
 * statically initialize all kinds of AssetImages. So, each subclass is POD,
 * and we supply implicit conversions from those classes back to AssetImage.
 *
 * STIR will generate all assets as AssetImage instances unless they
 * were specifically marked as pinned or flat assets.
 *
 * In a plain AssetImage, it's possible to access the asset's geometry
 * and to draw it with system support. The underlying data, however, should
 * be treated as opaque when working with plain AssetImages.
 */

struct AssetImage {
    _SYSAssetImage sys;

    // Common accessors for all AssetImage flavors
    AssetGroup &assetGroup() const { return *reinterpret_cast<AssetGroup*>(sys.pAssetGroup); }
    int tileWidth() const { return sys.width; }
    int tileHeight() const { return sys.height; }
    Int2 tileSize() const { return Vec2<int>(sys.width, sys.height); }
    int pixelWidth() const { return sys.width << 3; }
    int pixelHeight() const { return sys.height << 3; }
    Int2 pixelSize() const { return Vec2<int>(sys.width << 3, sys.height << 3); }
    int numFrames() const { return sys.frames; }
    int numTilesPerFrame() const { return tileWidth() * tileHeight(); }
    int numTiles() const { return numFrames() * numTilesPerFrame(); }
    
    /// Implicit conversion to system object
    operator const _SYSAssetImage& () const { return sys; }
    operator _SYSAssetImage& () { return sys; }
};


/**
 * An asset image in which all tiles are stored sequentially in memory.
 * This doesn't store any separate tilemap information, just the index for
 * the first tile in the sequence.
 *
 * Generate a PinnedAssetImage by passing the "pinned=1" option to image{}
 * in your STIR script.
 *
 * Pinned assets are required for sprites, since the hardware requires all
 * tiles in a sprite to be sequential.
 *
 * Pinned assets may also be efficient to use in other cases when there's
 * very little potential for tile deduplication.
 */
 
struct PinnedAssetImage {
    _SYSAssetImage sys;

    // Common accessors for all AssetImage flavors
    AssetGroup &assetGroup() const { return *reinterpret_cast<AssetGroup*>(sys.pAssetGroup); }
    int tileWidth() const { return sys.width; }
    int tileHeight() const { return sys.height; }
    Int2 tileSize() const { return Vec2<int>(sys.width, sys.height); }
    int pixelWidth() const { return sys.width << 3; }
    int pixelHeight() const { return sys.height << 3; }
    Int2 pixelSize() const { return Vec2<int>(sys.width << 3, sys.height << 3); }
    int numFrames() const { return sys.frames; }
    int numTilesPerFrame() const { return tileWidth() * tileHeight(); }
    int numTiles() const { return numFrames() * numTilesPerFrame(); }

    /// Implicit conversion to AssetImage base class
    operator const AssetImage& () const { return *reinterpret_cast<const AssetImage*>(this); }
    operator AssetImage& () { return *reinterpret_cast<AssetImage*>(this); }

    /**
     * Returns the index of the tile at linear position 'i' in the image.
     *
     * The returned index is unrelocated; it is relative to the base address
     * of the image's AssetGroup.
     */
    uint16_t tile(unsigned i) {
        ASSERT(i < numTiles());
        return sys.data + i;
    };

    /**
     * Return the index of the tile at the specified (x, y) tile coordinates,
     * and optionally on the specified frame number.
     *
     * The returned index is unrelocated; it is relative to the base address
     * of the image's AssetGroup.
     */
    uint16_t tile(Int2 pos, unsigned frame = 0) {
        ASSERT(pos.x < tileWidth() && pos.y < tileHeight() && frame < numFrames());
        return sys.data + pos.x + pos.y * tileHeight() + frame * numTilesPerFrame();
    }

    /**
     * Returns the index of the tile at linear position 'i' in the image.
     *
     * The returned index is relocated to an absolute address for the
     * specified cube. This image's assets must be installed on that cube.
     */
    uint16_t tile(_SYSCubeID cube, unsigned i) {
        ASSERT(i < numTiles());
        return assetGroup().baseAddress(cube) + sys.data + i;
    };

    /**
     * Return the index of the tile at the specified (x, y) tile coordinates,
     * and optionally on the specified frame number.
     *
     * The returned index is relocated to an absolute address for the
     * specified cube. This image's assets must be installed on that cube.
     */
    uint16_t tile(_SYSCubeID cube, Int2 pos, unsigned frame = 0) {
        ASSERT(pos.x < tileWidth() && pos.y < tileHeight() && frame < numFrames());
        return assetGroup().baseAddress(cube) + sys.data
            + pos.x + pos.y * tileHeight() + frame * numTilesPerFrame();
    }
};


/**
 * An asset image in which all tile indices are stored in a flat array,
 * without any additional compression. Flat assets are usually less efficient
 * than compressed AssetImages, but they allow you to have cheap random
 * access to any tile in the image.
 *
 * Generate a FlatAssetImage by passing the "flat=1" option to image{}
 * in your STIR script.
 */
 
struct FlatAssetImage {
    _SYSAssetImage sys;

    // Common accessors for all AssetImage flavors
    AssetGroup &assetGroup() const { return *reinterpret_cast<AssetGroup*>(sys.pAssetGroup); }
    int tileWidth() const { return sys.width; }
    int tileHeight() const { return sys.height; }
    Int2 tileSize() const { return Vec2<int>(sys.width, sys.height); }
    int pixelWidth() const { return sys.width << 3; }
    int pixelHeight() const { return sys.height << 3; }
    Int2 pixelSize() const { return Vec2<int>(sys.width << 3, sys.height << 3); }
    int numFrames() const { return sys.frames; }
    int numTilesPerFrame() const { return tileWidth() * tileHeight(); }
    int numTiles() const { return numFrames() * numTilesPerFrame(); }

    /// Implicit conversion to AssetImage base class
    operator const AssetImage& () const { return *reinterpret_cast<const AssetImage*>(this); }
    operator AssetImage& () { return *reinterpret_cast<AssetImage*>(this); }

    /**
     * Flat asset images represent their tile data as an array of uint16_t
     * un-relocated tile indices. This returns a pointer to that array.
     */
    const uint16_t *tileArray() const {
        return reinterpret_cast<const uint16_t *>(sys.data);
    }

    /**
     * Returns the index of the tile at linear position 'i' in the image.
     *
     * The returned index is unrelocated; it is relative to the base address
     * of the image's AssetGroup.
     */
    uint16_t tile(unsigned i) {
        ASSERT(i < numTiles());
        return tileArray()[i];
    };

    /**
     * Return the index of the tile at the specified (x, y) tile coordinates,
     * and optionally on the specified frame number.
     *
     * The returned index is unrelocated; it is relative to the base address
     * of the image's AssetGroup.
     */
    uint16_t tile(Int2 pos, unsigned frame = 0) {
        ASSERT(pos.x < tileWidth() && pos.y < tileHeight() && frame < numFrames());
        return tileArray()[pos.x + pos.y * tileHeight() + frame * numTilesPerFrame()];
    }

    /**
     * Returns the index of the tile at linear position 'i' in the image.
     *
     * The returned index is relocated to an absolute address for the
     * specified cube. This image's assets must be installed on that cube.
     */
    uint16_t tile(_SYSCubeID cube, unsigned i) {
        ASSERT(i < numTiles());
        return assetGroup().baseAddress(cube) + tileArray()[i];
    };

    /**
     * Return the index of the tile at the specified (x, y) tile coordinates,
     * and optionally on the specified frame number.
     *
     * The returned index is relocated to an absolute address for the
     * specified cube. This image's assets must be installed on that cube.
     */
    uint16_t tile(_SYSCubeID cube, Int2 pos, unsigned frame = 0) {
        ASSERT(pos.x < tileWidth() && pos.y < tileHeight() && frame < numFrames());
        return assetGroup().baseAddress(cube) + tileArray()[
            pos.x + pos.y * tileHeight() + frame * numTilesPerFrame()];
    }
};


/**
 * An audio asset, using any supported compression codec.
 */

struct AssetAudio {
    _SYSAudioModule sys;
};


};  // namespace Sifteo

#endif
