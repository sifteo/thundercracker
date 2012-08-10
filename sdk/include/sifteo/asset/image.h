/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * This file is part of the public interface to the Sifteo SDK.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#pragma once

/*
 * This header needs to work in both userspace and non-userspace
 * builds, though the latter have a greatly reduced feature set.
 */

#ifndef NOT_USERSPACE
#   include <sifteo/math.h>
#   include <sifteo/macros.h>
#   include <sifteo/asset/group.h>
#endif

#include <sifteo/abi.h>

namespace Sifteo {

/**
 * @addtogroup assets
 * @{
 */

/**
 * @brief Any kind of asset image, as defined in your `stir` script.
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

#ifndef NOT_USERSPACE   // Begin userspace-only members

    /// Access the AssetGroup instance associated with this AssetImage
    AssetGroup &assetGroup() const { return *reinterpret_cast<AssetGroup*>(sys.pAssetGroup); }

    /// The (width, height) vector of this image, in tiles
    Int2 tileSize() const { return vec<int>(sys.width, sys.height); }

    /// Half the size of this image, in tiles
    Int2 tileExtent() const { return tileSize() / 2; }

    /// The (width, height) vector of this image, in pixels
    Int2 pixelSize() const { return vec<int>(sys.width << 3, sys.height << 3); }

    /// Half the size of this image, in pixels
    Int2 pixelExtent() const { return pixelSize() / 2; }

#endif   // End userspace-only members

    /// The width of this image, in tiles
    int tileWidth() const { return sys.width; }

    /// The height of this image, in tiles
    int tileHeight() const { return sys.height; }

    /// The width of this image, in pixels
    int pixelWidth() const { return sys.width << 3; }

    /// The height of this image, in pixels
    int pixelHeight() const { return sys.height << 3; }

    /// Access the number of 'frames' in this image
    int numFrames() const { return sys.frames; }

    /// Compute the total number of tiles per frame (tileWidth * tileHeight)
    int numTilesPerFrame() const { return tileWidth() * tileHeight(); }

    /// Compute the total number of tiles in the image
    int numTiles() const { return numFrames() * numTilesPerFrame(); }
    
    /// Implicit conversion to system object
    operator const _SYSAssetImage& () const { return sys; }
    operator _SYSAssetImage& () { return sys; }
    operator const _SYSAssetImage* () const { return &sys; }
    operator _SYSAssetImage* () { return &sys; }
};


/**
 * @brief An AssetImage in which all tiles are stored sequentially in memory.
 *
 * This doesn't store any separate tilemap information, just the index for
 * the first tile in the sequence.
 *
 * Generate a PinnedAssetImage by passing the `pinned=1` option to image{}
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

#ifndef NOT_USERSPACE   // Begin userspace-only members

    /// Access the AssetGroup instance associated with this AssetImage
    AssetGroup &assetGroup() const { return *reinterpret_cast<AssetGroup*>(sys.pAssetGroup); }

    /// The (width, height) vector of this image, in tiles
    Int2 tileSize() const { return vec<int>(sys.width, sys.height); }

    /// Half the size of this image, in tiles
    Int2 tileExtent() const { return tileSize() / 2; }

    /// The (width, height) vector of this image, in pixels
    Int2 pixelSize() const { return vec<int>(sys.width << 3, sys.height << 3); }

    /// Half the size of this image, in pixels
    Int2 pixelExtent() const { return pixelSize() / 2; }

    /**
     * @brief Returns the index of the tile at linear position 'i' in the image.
     *
     * The returned index is unrelocated; it is relative to the base address
     * of the image's AssetGroup.
     */
    uint16_t tile(unsigned i) const {
        ASSERT(i < numTiles());
        return sys.pData + i;
    }

    /**
     * @brief Return the index of the tile at the specified (x, y) tile coordinates.
     *
     * The returned index is unrelocated; it is relative to the base address
     * of the image's AssetGroup.
     */
    uint16_t tile(Int2 pos, unsigned frame = 0) const {
        ASSERT(pos.x < tileWidth() && pos.y < tileHeight() && frame < numFrames());
        return sys.pData + pos.x + pos.y * tileHeight() + frame * numTilesPerFrame();
    }

    /**
     * @brief Returns the index of the tile at linear position 'i' in the image.
     *
     * The returned index is relocated to an absolute address for the
     * specified cube. This image's assets must be installed on that cube.
     */
    uint16_t tile(_SYSCubeID cube, unsigned i) const {
        ASSERT(i < numTiles());
        return assetGroup().baseAddress(cube) + sys.pData + i;
    }

    /**
     * @brief Return the index of the tile at the specified (x, y) tile coordinates.
     *
     * The returned index is relocated to an absolute address for the
     * specified cube. This image's assets must be installed on that cube.
     */
    uint16_t tile(_SYSCubeID cube, Int2 pos, unsigned frame = 0) const {
        ASSERT(pos.x < tileWidth() && pos.y < tileHeight() && frame < numFrames());
        return assetGroup().baseAddress(cube) + sys.pData
            + pos.x + pos.y * tileHeight() + frame * numTilesPerFrame();
    }

#endif   // End userspace-only members

    /// The width of this image, in tiles
    int tileWidth() const { return sys.width; }

    /// The height of this image, in tiles
    int tileHeight() const { return sys.height; }

    /// The width of this image, in pixels
    int pixelWidth() const { return sys.width << 3; }

    /// The height of this image, in pixels
    int pixelHeight() const { return sys.height << 3; }

    /// Access the number of 'frames' in this image
    int numFrames() const { return sys.frames; }

    /// Compute the total number of tiles per frame (tileWidth * tileHeight)
    int numTilesPerFrame() const { return tileWidth() * tileHeight(); }

    /// Compute the total number of tiles in the image
    int numTiles() const { return numFrames() * numTilesPerFrame(); }

    /// Implicit conversion to AssetImage base class
    operator const AssetImage& () const { return *reinterpret_cast<const AssetImage*>(this); }
    operator AssetImage& () { return *reinterpret_cast<AssetImage*>(this); }
    operator const AssetImage* () const { return reinterpret_cast<const AssetImage*>(this); }
    operator AssetImage* () { return reinterpret_cast<AssetImage*>(this); }

    /// Implicit conversion to system object
    operator const _SYSAssetImage& () const { return sys; }
    operator _SYSAssetImage& () { return sys; }
    operator const _SYSAssetImage* () const { return &sys; }
    operator _SYSAssetImage* () { return &sys; }
};


/**
 * @brief An AssetImage in which all tile indices are stored in a flat array,
 * without any additional compression.
 *
 * Flat assets are usually less efficient than compressed AssetImages,
 * but they allow you to have cheap random access to any tile in the image.
 *
 * Generate a FlatAssetImage by passing the `flat=1` option to image{}
 * in your STIR script.
 */
 
struct FlatAssetImage {
    _SYSAssetImage sys;

#ifndef NOT_USERSPACE   // Begin userspace-only members

    /// Access the AssetGroup instance associated with this AssetImage
    AssetGroup &assetGroup() const { return *reinterpret_cast<AssetGroup*>(sys.pAssetGroup); }

    /// The (width, height) vector of this image, in tiles
    Int2 tileSize() const { return vec<int>(sys.width, sys.height); }

    /// Half the size of this image, in tiles
    Int2 tileExtent() const { return tileSize() / 2; }

    /// The (width, height) vector of this image, in pixels
    Int2 pixelSize() const { return vec<int>(sys.width << 3, sys.height << 3); }

    /// Half the size of this image, in pixels
    Int2 pixelExtent() const { return pixelSize() / 2; }

    /**
     * @brief Get a pointer to the raw tile data.
     *
     * Flat asset images represent their tile data as an array of uint16_t
     * un-relocated tile indices. This returns a pointer to that array.
     */
    const uint16_t *tileArray() const {
        return reinterpret_cast<const uint16_t *>(sys.pData);
    }

    /**
     * @brief Returns the index of the tile at linear position 'i' in the image.
     *
     * The returned index is unrelocated; it is relative to the base address
     * of the image's AssetGroup.
     */
    uint16_t tile(unsigned i) const {
        ASSERT(i < numTiles());
        return tileArray()[i];
    }

    /**
     * @brief Return the index of the tile at the specified (x, y) tile coordinates.
     *
     * The returned index is unrelocated; it is relative to the base address
     * of the image's AssetGroup.
     */
    uint16_t tile(Int2 pos, unsigned frame = 0) const {
        ASSERT(pos.x < tileWidth() && pos.y < tileHeight() && frame < numFrames());
        return tileArray()[pos.x + pos.y * tileHeight() + frame * numTilesPerFrame()];
    }

    /**
     * @brief Returns the index of the tile at linear position 'i' in the image.
     *
     * The returned index is relocated to an absolute address for the
     * specified cube. This image's assets must be installed on that cube.
     */
    uint16_t tile(_SYSCubeID cube, unsigned i) const {
        ASSERT(i < numTiles());
        return assetGroup().baseAddress(cube) + tileArray()[i];
    }

    /**
     * @brief Return the index of the tile at the specified (x, y) tile coordinates.
     *
     * The returned index is relocated to an absolute address for the
     * specified cube. This image's assets must be installed on that cube.
     */
    uint16_t tile(_SYSCubeID cube, Int2 pos, unsigned frame = 0) const {
        ASSERT(pos.x < tileWidth() && pos.y < tileHeight() && frame < numFrames());
        return assetGroup().baseAddress(cube) + tileArray()[
            pos.x + pos.y * tileHeight() + frame * numTilesPerFrame()];
    }

#endif   // End userspace-only members

    /// The width of this image, in tiles
    int tileWidth() const { return sys.width; }

    /// The height of this image, in tiles
    int tileHeight() const { return sys.height; }

    /// The width of this image, in pixels
    int pixelWidth() const { return sys.width << 3; }

    /// The height of this image, in pixels
    int pixelHeight() const { return sys.height << 3; }

    /// Access the number of 'frames' in this image
    int numFrames() const { return sys.frames; }

    /// Compute the total number of tiles per frame (tileWidth * tileHeight)
    int numTilesPerFrame() const { return tileWidth() * tileHeight(); }

    /// Compute the total number of tiles in the image
    int numTiles() const { return numFrames() * numTilesPerFrame(); }

    /// Implicit conversion to AssetImage base class
    operator const AssetImage& () const { return *reinterpret_cast<const AssetImage*>(this); }
    operator AssetImage& () { return *reinterpret_cast<AssetImage*>(this); }
    operator const AssetImage* () const { return reinterpret_cast<const AssetImage*>(this); }
    operator AssetImage* () { return reinterpret_cast<AssetImage*>(this); }

    /// Implicit conversion to system object
    operator const _SYSAssetImage& () const { return sys; }
    operator _SYSAssetImage& () { return sys; }
    operator const _SYSAssetImage* () const { return &sys; }
    operator _SYSAssetImage* () { return &sys; }
};

/**
 * @} end addtogroup assets
 */

};  // namespace Sifteo
