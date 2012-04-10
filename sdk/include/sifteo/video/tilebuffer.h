/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * This file is part of the public interface to the Sifteo SDK.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#pragma once
#ifdef NO_USERSPACE_HEADERS
#   error This is a userspace-only header, not allowed by the current build.
#endif

#include <sifteo/abi.h>
#include <sifteo/macros.h>
#include <sifteo/math.h>
#include <sifteo/asset.h>
#include <sifteo/memory.h>
#include <sifteo/cube.h>

namespace Sifteo {


/**
 * A TileBuffer is a drawable that's backed by plain memory, instead of
 * by a VideoBuffer. You can draw on a TileBuffer in the same manner you'd
 * draw on BG0, for example.
 *
 * In memory, we store a flat array of 16-bit relocated tile indices.
 *
 * TileBuffers can be used as assets. In particular, they can be freely
 * converted to FlatAssetImage or AssetImage instances. This means you can
 * use them to do off-screen rendering, and then draw that rendering to
 * any other drawable.
 *
 * Note that TileBuffers are not POD types. The underlying AssetImage
 * header we build in RAM needs to be initialized by the constructor,
 * and you must attach the TileBuffer to a specific CubeID so we can
 * relocate assets.
 */

template <unsigned tTileWidth, unsigned tTileHeight, unsigned tFrames = 1>
struct TileBuffer {
    struct {
        _SYSAssetImage image;
        _SYSCubeID cube;
    } sys;
    uint16_t tiles[tTileWidth * tTileHeight];

    /// Implicit conversion to AssetImage base class
    operator const AssetImage& () const { return *reinterpret_cast<const AssetImage*>(&sys.image); }
    operator AssetImage& () { return *reinterpret_cast<AssetImage*>(&sys.image); }
    operator const AssetImage* () const { return reinterpret_cast<const AssetImage*>(&sys.image); }
    operator AssetImage* () { return reinterpret_cast<AssetImage*>(&sys.image); }

    /// Implicit conversion to FlatAssetImage
    operator const FlatAssetImage& () const { return *reinterpret_cast<const FlatAssetImage*>(&sys.image); }
    operator FlatAssetImage& () { return *reinterpret_cast<FlatAssetImage*>(&sys.image); }
    operator const FlatAssetImage* () const { return reinterpret_cast<const FlatAssetImage*>(&sys.image); }
    operator FlatAssetImage* () { return reinterpret_cast<FlatAssetImage*>(&sys.image); }

    /// Implicit conversion to system object
    operator const _SYSAssetImage& () const { return sys.image; }
    operator _SYSAssetImage& () { return sys.image; }
    operator const _SYSAssetImage* () const { return sys.image; }
    operator _SYSAssetImage* () { return sys.image; }

    /**
     * Return the CubeID associated with this drawable.
     */
    CubeID cube() const {
        return sys.cube;
    }

    /**
     * Change the CubeID associated with this drawable.
     *
     * Note that this buffer holds relocated tile indices, and
     * there is no way to efficiently retarget an already-rendered
     * TileBuffer from one cube to another. So, this will
     * effectively render any existing contents of this buffer
     * meaningless.
     */
    void setCube(CubeID cube) {
        sys.cube = cube;
    }

    /**
     * Initialize the TileBuffer's AssetImage header.
     *
     * You must pass in a CubeID which represents the cube that
     * this buffer will be drawn to. Each cube may have assets
     * loaded at different addresses, so we need to know this in
     * order to relocate tile indices when drawing to this buffer.
     *
     * This does *not* write anything to the tile buffer memory itself.
     * If you want to initialize the buffer to a known value, call erase().
     */
    TileBuffer(CubeID cube) {
        sys.cube = cube;

        bzero(sys.image);
        sys.image.width = tTileWidth;
        sys.image.height = tTileHeight;
        sys.image.frames = tFrames;
        sys.image.format = _SYS_AIF_FLAT;
        sys.image.pData = reinterpret_cast<uint32_t>(&tiles[0]);
    }

    /**
     * Return the width, in tiles, of this mode
     */
    static unsigned tileWidth() {
        return tTileWidth;
    }

    /**
     * Return the height, in tiles, of this mode
     */
    static unsigned tileHeight() {
        return tTileHeight;
    }

    /**
     * Return the size of this mode as a vector, in tiles.
     */
    static UInt2 tileSize() {
        return vec(tileWidth(), tileHeight());
    }

    /**
     * Return the width, in pixels, of this mode
     */
    static unsigned pixelWidth() {
        return tileWidth() * 8;
    }

    /**
     * Return the height, in pixel, of this mode
     */
    static unsigned pixelHeight() {
        return tileHeight() * 8;
    }

    /**
     * Return the size of this mode as a vector, in pixels.
     */
    static UInt2 pixelSize() {
        return vec(pixelWidth(), pixelHeight());
    }

    /**
     * Returns the size of this drawable's tile data, in bytes
     */
    static unsigned sizeInBytes() {
        return tileWidth() * tileHeight() * 2;
    }

    /**
     * Returns the size of this drawable's tile data, in 16-bit words
     */
    static unsigned sizeInWords() {
        return tileWidth() * tileHeight();
    }

    /**
     * Erase the buffer, filling it with the specified
     * absolute tile index value.
     */
    void erase(uint16_t index = 0) {
        memset16(&tiles[0], index, sizeInWords());
    }

    /**
     * Erase the buffer, filling it with the first tile
     * from the specified PinnedAssetImage.
     */
    void erase(const PinnedAssetImage &image) {
        erase(image.tile(sys.cube, 0));
    }

    /**
     * Plot a single tile, by absolute tile index,
     * at location 'pos' in tile units.
     *
     * All coordinates must be in range. This function performs no clipping.
     */
    void plot(UInt2 pos, uint16_t tileIndex) {
        ASSERT(pos.x < tileWidth() && pos.y < tileHeight());
        tiles[pos.x + pos.y * tileWidth()] = tileIndex;
    }

    /**
     * Plot a horizontal span of tiles, by absolute tile index,
     * given the position of the leftmost tile and the number of tiles to plot.
     *
     * All coordinates must be in range. This function performs no clipping.
     */
    void span(UInt2 pos, unsigned width, unsigned tileIndex)
    {
        ASSERT(pos.x <= tileWidth() && width <= tileWidth() &&
            (pos.x + width) <= tileWidth() && pos.y < tileHeight());
        memset16(&tiles[pos.x + pos.y * tileWidth()], tileIndex, width);
    }

    /**
     * Plot a horizontal span of tiles, using the first tile of a pinned asset.
     * All coordinates must be in range. This function performs no clipping.
     */
    void span(UInt2 pos, unsigned width, const PinnedAssetImage &image)
    {
        span(pos, width, image.tile(sys.cube));
    }

    /**
     * Fill a rectangle of identical tiles, specified as a top-left corner
     * location and a size.
     *
     * All coordinates must be in range. This function performs no clipping.
     */
    void fill(UInt2 topLeft, UInt2 size, unsigned tileIndex)
    {
        while (size.y) {
            span(topLeft, size.x, tileIndex);
            size.y--;
            topLeft.y++;
        }
    }

    /**
     * Fill a rectangle of identical tiles, using the first tile of a
     * pinned asset.
     *
     * All coordinates must be in range. This function performs no clipping.
     */
    void fill(UInt2 topLeft, UInt2 size, const PinnedAssetImage &image)
    {
        fill(topLeft, size, image.tile(sys.cube));
    }

    /**
     * Draw a full AssetImage frame, with its top-left corner at the
     * specified location. Locations are specified in tile units, relative
     * to the top-left of the 18x18 grid.
     *
     * All coordinates must be in range. This function performs no clipping.
     */
    void image(UInt2 pos, const AssetImage &image, unsigned frame = 0)
    {
        _SYS_image_memDraw(&tiles[pos.x + pos.y * tileWidth()],
            image, tileWidth(), frame);
    }

    /**
     * Draw part of an AssetImage frame, with its top-left corner at the
     * specified location. Locations are specified in tile units, relative
     * to the top-left of the 18x18 grid.
     *
     * All coordinates must be in range. This function performs no clipping.
     */
    void image(UInt2 destXY, UInt2 size, const AssetImage &image, UInt2 srcXY, unsigned frame = 0)
    {
        _SYS_image_memDrawRect(&tiles[destXY.x + destXY.y * tileWidth()],
            image, tileWidth(), frame, (_SYSInt2*) &srcXY, (_SYSInt2*) &size);
    }

    /**
     * Draw text, using an AssetImage as a fixed width font. Each character
     * is represented by a consecutive 'frame' in the image. Characters not
     * present in the font will be skipped.
     */
    void text(Int2 topLeft, const AssetImage &font, const char *str, char firstChar = ' ')
    {
        uint16_t *addr = &tiles[topLeft.x + topLeft.y * tileWidth()];
        uint16_t *lineAddr = addr;
        char c;

        while ((c = *str)) {
            if (c == '\n') {
                addr = (lineAddr += tileWidth());
            } else {
                _SYS_image_memDraw(addr, font, tileWidth(), c - firstChar);
                addr += font.tileWidth();
            }
            str++;
        }
    }
};


};  // namespace Sifteo
