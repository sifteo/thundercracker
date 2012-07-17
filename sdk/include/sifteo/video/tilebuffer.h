/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * This file is part of the public interface to the Sifteo SDK.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#pragma once
#ifdef NOT_USERSPACE
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
 * @addtogroup video
 * @{
 */


/**
 * @brief A drawable that's backed by plain memory, instead of
 * by a VideoBuffer.
 *
 * Template parameters specify the width and height of the buffer,
 * in tiles, and optionally the number of frames.
 *
 * You can draw on a TileBuffer in the same manner you'd
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

template <unsigned tW, unsigned tH, unsigned tF = 1>
struct TileBuffer {
    struct {
        _SYSAssetImage image;
        _SYSCubeID cube;
    } sys;
    uint16_t tiles[tW * tH * tF];

    // Implicit conversion to AssetImage base class
    operator const AssetImage& () const { return *reinterpret_cast<const AssetImage*>(&sys.image); }
    operator AssetImage& () { return *reinterpret_cast<AssetImage*>(&sys.image); }
    operator const AssetImage* () const { return reinterpret_cast<const AssetImage*>(&sys.image); }
    operator AssetImage* () { return reinterpret_cast<AssetImage*>(&sys.image); }

    // Implicit conversion to FlatAssetImage
    operator const FlatAssetImage& () const { return *reinterpret_cast<const FlatAssetImage*>(&sys.image); }
    operator FlatAssetImage& () { return *reinterpret_cast<FlatAssetImage*>(&sys.image); }
    operator const FlatAssetImage* () const { return reinterpret_cast<const FlatAssetImage*>(&sys.image); }
    operator FlatAssetImage* () { return reinterpret_cast<FlatAssetImage*>(&sys.image); }

    // Implicit conversion to system object
    operator const _SYSAssetImage& () const { return sys.image; }
    operator _SYSAssetImage& () { return sys.image; }
    operator const _SYSAssetImage* () const { return sys.image; }
    operator _SYSAssetImage* () { return sys.image; }

    /**
     * @brief Return the CubeID associated with this drawable.
     */
    CubeID cube() const {
        ASSERT(sys.cube != _SYS_CUBE_ID_INVALID);
        return sys.cube;
    }

    /**
     * @brief Change the CubeID associated with this drawable.
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
     * @brief Initialize the TileBuffer's AssetImage header.
     */
    void init() {
        sys.cube = _SYS_CUBE_ID_INVALID;
        bzero(sys.image);
        sys.image.width = tW;
        sys.image.height = tH;
        sys.image.frames = tF;
        sys.image.format = _SYS_AIF_FLAT;
        sys.image.pData = reinterpret_cast<uint32_t>(&tiles[0]);
    }

    /**
     * @brief Initialize a TileBuffer
     *
     * You must call setCube() on this instance before using it, due to the
     * potentially different memory layout on each individual cube.
     */
    TileBuffer() {
        init();
    }

    /**
     * @brief Initialize a TileBuffer for use.
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
        init();

        sys.cube = cube;
    }

    /**
     * @brief Return the width, in tiles, of this mode
     */
    static unsigned tileWidth() {
        return tW;
    }

    /**
     * @brief Return the height, in tiles, of this mode
     */
    static unsigned tileHeight() {
        return tH;
    }

    /**
     * @brief Return the size of this mode as a vector, in tiles.
     */
    static UInt2 tileSize() {
        return vec(tileWidth(), tileHeight());
    }

    /**
     * @brief Return the width, in pixels, of this mode
     */
    static unsigned pixelWidth() {
        return tileWidth() * 8;
    }

    /**
     * @brief Return the height, in pixel, of this mode
     */
    static unsigned pixelHeight() {
        return tileHeight() * 8;
    }

    /**
     * @brief Return the size of this mode as a vector, in pixels.
     */
    static UInt2 pixelSize() {
        return vec(pixelWidth(), pixelHeight());
    }

    /**
     * @brief Return the number of frames in this image.
     */
    static int numFrames() {
        return tF;
    }

    /**
     * @brief Return the number of tiles in each frame of the image.
     */
    static int numTilesPerFrame() {
        return tileWidth() * tileHeight();
    }

    /**
     * @brief Return the total number of tiles in every frame of the image.
     */
    static int numTiles() {
        return numFrames() * numTilesPerFrame();
    }

    /**
     * @brief Returns the size of this drawable's tile data, in bytes
     */
    static unsigned sizeInBytes() {
        return numTiles() * 2;
    }

    /**
     * @brief Returns the size of this drawable's tile data, in 16-bit words
     */
    static unsigned sizeInWords() {
        return numTiles();
    }

    /**
     * @brief Erase the buffer, filling it with the specified
     * absolute tile index value.
     */
    void erase(uint16_t index = 0) {
        ASSERT(sys.cube != _SYS_CUBE_ID_INVALID);
        memset16(&tiles[0], index, sizeInWords());
    }

    /**
     * @brief Erase the buffer, filling it with the first tile
     * from the specified PinnedAssetImage.
     */
    void erase(const PinnedAssetImage &image) {
        erase(image.tile(sys.cube, 0));
    }

    /**
     * @brief Calculate the video buffer address of a particular tile.
     *
     * All coordinates must be in range. This function performs no clipping.
     */
    uint16_t tileAddr(UInt2 pos, unsigned frame = 0) {
        ASSERT(sys.cube != _SYS_CUBE_ID_INVALID);
        return pos.x + (pos.y + frame * tileHeight()) * tileWidth();
    }

    /**
     * @brief Plot a single tile, by absolute tile index,
     * at linear position 'i'.
     *
     * All coordinates must be in range. This function performs no clipping.
     */
    void plot(unsigned i, uint16_t tileIndex) {
        ASSERT(sys.cube != _SYS_CUBE_ID_INVALID);
        ASSERT(i < numTiles());
        tiles[i] = tileIndex;
    }

    /**
     * @brief Plot a single tile, by absolute tile index,
     * at location 'pos' in tile units, on the given frame.
     *
     * All coordinates must be in range. This function performs no clipping.
     */
    void plot(UInt2 pos, uint16_t tileIndex, unsigned frame = 0) {
        ASSERT(sys.cube != _SYS_CUBE_ID_INVALID);
        ASSERT(pos.x < tileWidth() && pos.y < tileHeight() && frame < numFrames());
        tiles[tileAddr(pos, frame)] = tileIndex;
    }

    /**
     * @brief Returns the index of the tile at linear position 'i' in the image.
     */
    uint16_t tile(unsigned i) const {
        ASSERT(sys.cube != _SYS_CUBE_ID_INVALID);
        ASSERT(i < numTiles());
        return tiles[i];
    }

    /**
     * @brief Return the index of the tile at the specified (x, y) tile coordinates,
     * and optionally on the specified frame number.
     */
    uint16_t tile(Int2 pos, unsigned frame = 0) const {
        ASSERT(sys.cube != _SYS_CUBE_ID_INVALID);
        ASSERT(pos.x < tileWidth() && pos.y < tileHeight() && frame < numFrames());
        return tiles[tileAddr(pos, frame)];
    }

    /**
     * @brief Plot a horizontal span of tiles, by absolute tile index,
     * given the position of the leftmost tile and the number of tiles to plot.
     *
     * All coordinates must be in range. This function performs no clipping.
     */
    void span(UInt2 pos, unsigned width, unsigned tileIndex, unsigned frame = 0)
    {
        ASSERT(sys.cube != _SYS_CUBE_ID_INVALID);
        ASSERT(pos.x <= tileWidth() && width <= tileWidth() &&
            (pos.x + width) <= tileWidth() && pos.y < tileHeight());
        memset16(&tiles[tileAddr(pos, frame)], tileIndex, width);
    }

    /**
     * @brief Plot a horizontal span of tiles, using the first tile of a pinned asset.
     *
     * All coordinates must be in range. This function performs no clipping.
     */
    void span(UInt2 pos, unsigned width, const PinnedAssetImage &image, unsigned frame = 0)
    {
        span(pos, width, image.tile(sys.cube, 0), frame);
    }

    /**
     * @brief Fill a rectangle of identical tiles, specified as a top-left corner
     * location and a size.
     *
     * All coordinates must be in range. This function performs no clipping.
     */
    void fill(UInt2 topLeft, UInt2 size, unsigned tileIndex, unsigned frame = 0)
    {
        while (size.y) {
            span(topLeft, size.x, tileIndex, frame);
            size.y--;
            topLeft.y++;
        }
    }

    /**
     * @brief Fill a rectangle of identical tiles, using the first tile of a
     * pinned asset.
     *
     * All coordinates must be in range. This function performs no clipping.
     */
    void fill(UInt2 topLeft, UInt2 size, const PinnedAssetImage &image, unsigned frame = 0)
    {
        fill(topLeft, size, image.tile(sys.cube, 0), frame);
    }

    /**
     * @brief Draw a full AssetImage frame, with its top-left corner at the
     * specified location.
     *
     * Locations are specified in tile units, relative
     * to the top-left of the 18x18 grid.
     *
     * All coordinates must be in range. This function performs no clipping.
     */
    void image(UInt2 pos, const AssetImage &image, unsigned srcFrame = 0, unsigned destFrame = 0)
    {
        ASSERT(sys.cube != _SYS_CUBE_ID_INVALID);
        ASSERT(pos.x < tileWidth() && pos.x + image.tileWidth() <= tileWidth() &&
               pos.y < tileHeight() && pos.y + image.tileHeight() <= tileHeight() &&
               destFrame < numFrames());
        _SYS_image_memDraw(&tiles[tileAddr(pos, destFrame)], sys.cube, image, tileWidth(), srcFrame);
    }

    /**
     * @brief Draw part of an AssetImage frame, with its top-left corner at the
     * specified location.
     *
     * Locations are specified in tile units, relative
     * to the top-left of the 18x18 grid.
     *
     * All coordinates must be in range. This function performs no clipping.
     */
    void image(UInt2 destXY, UInt2 size, const AssetImage &image, UInt2 srcXY,
        unsigned srcFrame = 0, unsigned destFrame = 0)
    {
        ASSERT(sys.cube != _SYS_CUBE_ID_INVALID);
        ASSERT(destXY.x < tileWidth() && destXY.x + size.x <= tileWidth() &&
               destXY.y < tileHeight() && destXY.y + size.y <= tileHeight() &&
               destFrame < numFrames());
        _SYS_image_memDrawRect(&tiles[tileAddr(destXY, destFrame)], sys.cube,
            image, tileWidth(), srcFrame, (_SYSInt2*) &srcXY, (_SYSInt2*) &size);
    }

    /**
     * @brief Draw text, using an AssetImage as a fixed width font.
     *
     * Each character is represented by a consecutive 'frame' in the image.
     * Characters not present in the font will be skipped.
     */
    void text(Int2 topLeft, const AssetImage &font, const char *str,
        unsigned destFrame = 0, char firstChar = ' ')
    {
        ASSERT(sys.cube != _SYS_CUBE_ID_INVALID);
        uint16_t *addr = &tiles[tileAddr(topLeft, destFrame)];
        uint16_t *lineAddr = addr;
        char c;

        while ((c = *str)) {
            if (c == '\n') {
                addr = (lineAddr += tileWidth());
            } else {
                ASSERT(font.tileWidth() + (font.tileHeight() - 1) * tileWidth() + addr
                    <= numTiles() + tiles);
                _SYS_image_memDraw(addr, sys.cube, font, tileWidth(), c - firstChar);
                addr += font.tileWidth();
            }
            str++;
        }
    }
};


/**
 * @brief A drawable that's backed by plain memory, usable with
 * multiple cubes.
 *
 * Template parameters specify the width and height of the buffer,
 * in tiles, and optionally the number of frames.
 *
 * This is a variation on TileBuffer, but instead of storing per-cube
 * (already relocated) tile indices, this stores per-asset-group (relocatable)
 * indices.
 *
 * A RelocatableTileBuffer can only be used with assets from a single
 * group, and that group must be set during initialization. But, in return,
 * the RelocatableTileBuffer may be drawn to any cube that has that group
 * installed.
 *
 * In memory, we store a flat array of 16-bit un-relocated tile indices.
 *
 * RelocatableTileBuffers can be used as assets. In particular, they can be
 * freely converted to FlatAssetImage or AssetImage instances. This means you can
 * use them to do off-screen rendering, and then draw that rendering to
 * any other drawable.
 *
 * Note that RelocatableTileBuffers are not POD types. The underlying AssetImage
 * header we build in RAM needs to be initialized by the constructor.
 */

template <unsigned tW, unsigned tH, unsigned tF = 1>
struct RelocatableTileBuffer {
    _SYSAssetImage sys;
    uint16_t tiles[tW * tH * tF];

    /// Implicit conversion to AssetImage base class
    operator const AssetImage& () const { return *reinterpret_cast<const AssetImage*>(&sys); }
    operator AssetImage& () { return *reinterpret_cast<AssetImage*>(&sys); }
    operator const AssetImage* () const { return reinterpret_cast<const AssetImage*>(&sys); }
    operator AssetImage* () { return reinterpret_cast<AssetImage*>(&sys); }

    /// Implicit conversion to FlatAssetImage
    operator const FlatAssetImage& () const { return *reinterpret_cast<const FlatAssetImage*>(&sys); }
    operator FlatAssetImage& () { return *reinterpret_cast<FlatAssetImage*>(&sys); }
    operator const FlatAssetImage* () const { return reinterpret_cast<const FlatAssetImage*>(&sys); }
    operator FlatAssetImage* () { return reinterpret_cast<FlatAssetImage*>(&sys); }

    /// Implicit conversion to system object
    operator const _SYSAssetImage& () const { return sys; }
    operator _SYSAssetImage& () { return sys; }
    operator const _SYSAssetImage* () const { return sys; }
    operator _SYSAssetImage* () { return sys; }

    /**
     * @brief Initialize the RelocatableTileBuffer's AssetImage header.
     *
     * This resets the AssetGroup associated with the buffer, so it may again be used
     * with assets from any group. The contents of the buffer are not cleared.
     */
    void init() {
        bzero(sys);
        sys.width = tW;
        sys.height = tH;
        sys.frames = tF;
        sys.format = _SYS_AIF_FLAT;
        sys.pData = reinterpret_cast<uint32_t>(&tiles[0]);
    }

    /**
     * @brief Initialize a RelocatableTileBuffer
     */
    RelocatableTileBuffer() {
        init();
    }

    /**
     * @brief Return the width, in tiles, of this mode
     */
    static unsigned tileWidth() {
        return tW;
    }

    /**
     * @brief Return the height, in tiles, of this mode
     */
    static unsigned tileHeight() {
        return tH;
    }

    /**
     * @brief Return the size of this mode as a vector, in tiles.
     */
    static UInt2 tileSize() {
        return vec(tileWidth(), tileHeight());
    }

    /**
     * @brief Return the width, in pixels, of this mode
     */
    static unsigned pixelWidth() {
        return tileWidth() * 8;
    }

    /**
     * @brief Return the height, in pixel, of this mode
     */
    static unsigned pixelHeight() {
        return tileHeight() * 8;
    }

    /**
     * @brief Return the size of this mode as a vector, in pixels.
     */
    static UInt2 pixelSize() {
        return vec(pixelWidth(), pixelHeight());
    }

    /**
     * @brief Return the number of frames in this image.
     */
    static int numFrames() {
        return tF;
    }

    /**
     * @brief Return the number of tiles in each frame of the image.
     */
    static int numTilesPerFrame() {
        return tileWidth() * tileHeight();
    }

    /**
     * @brief Return the total number of tiles in every frame of the image.
     */
    static int numTiles() {
        return numFrames() * numTilesPerFrame();
    }

    /**
     * @brief Returns the size of this drawable's tile data, in bytes
     */
    static unsigned sizeInBytes() {
        return numTiles() * 2;
    }

    /**
     * @brief Returns the size of this drawable's tile data, in 16-bit words
     */
    static unsigned sizeInWords() {
        return numTiles();
    }

    /**
     * @brief Set the AssetGroup associated with this buffer.
     *
     * If the same asset group is already associated, this is a no-op.
     */
    void setAssetGroup(AssetGroup &group) {
        uint32_t pAssetGroup = reinterpret_cast<uint32_t>(&group.sys);
        ASSERT(sys.pAssetGroup == 0 || sys.pAssetGroup == pAssetGroup);
        sys.pAssetGroup = pAssetGroup;
    }

    /**
     * @brief Erase the buffer, filling it with the specified
     * relative tile index value.
     */
    void erase(uint16_t index = 0) {
        memset16(&tiles[0], index, sizeInWords());
    }

    /**
     * @brief Erase the buffer, filling it with the first tile
     * from the specified PinnedAssetImage.
     */
    void erase(const PinnedAssetImage &image) {
        setAssetGroup(image.assetGroup());
        erase(image.tile(0));
    }

    /**
     * @brief Calculate the video buffer address of a particular tile.
     *
     * All coordinates must be in range. This function performs no clipping.
     */
    uint16_t tileAddr(UInt2 pos, unsigned frame = 0) {
        return pos.x + (pos.y + frame * tileHeight()) * tileWidth();
    }

    /**
     * @brief Plot a single tile, by relative tile index,
     * at linear position 'i'.
     *
     * All coordinates must be in range. This function performs no clipping.
     */
    void plot(unsigned i, uint16_t tileIndex) {
        ASSERT(i < numTiles());
        tiles[i] = tileIndex;
    }

    /**
     * @brief Plot a single tile, by relative tile index,
     * at location 'pos' in tile units, on the given frame.
     *
     * All coordinates must be in range. This function performs no clipping.
     */
    void plot(UInt2 pos, uint16_t tileIndex, unsigned frame = 0) {
        ASSERT(pos.x < tileWidth() && pos.y < tileHeight() && frame < numFrames());
        tiles[tileAddr(pos, frame)] = tileIndex;
    }

    /**
     * @brief Returns the relative index of the tile at linear position 'i' in the image.
     */
    uint16_t tile(unsigned i) const {
        ASSERT(i < numTiles());
        return tiles[i];
    }

    /**
     * @brief Return the relative index of the tile at the specified (x, y) tile coordinates,
     * and optionally on the specified frame number.
     */
    uint16_t tile(Int2 pos, unsigned frame = 0) const {
        ASSERT(pos.x < tileWidth() && pos.y < tileHeight() && frame < numFrames());
        return tiles[tileAddr(pos, frame)];
    }

    /**
     * @brief Plot a horizontal span of tiles, by relative tile index,
     * given the position of the leftmost tile and the number of tiles to plot.
     *
     * All coordinates must be in range. This function performs no clipping.
     */
    void span(UInt2 pos, unsigned width, unsigned tileIndex, unsigned frame = 0)
    {
        ASSERT(pos.x <= tileWidth() && width <= tileWidth() &&
            (pos.x + width) <= tileWidth() && pos.y < tileHeight());
        memset16(&tiles[tileAddr(pos, frame)], tileIndex, width);
    }

    /**
     * @brief Plot a horizontal span of tiles, using the first tile of a pinned asset.
     *
     * All coordinates must be in range. This function performs no clipping.
     */
    void span(UInt2 pos, unsigned width, const PinnedAssetImage &image, unsigned frame = 0)
    {
        setAssetGroup(image.assetGroup());
        span(pos, width, image.tile(0), frame);
    }

    /**
     * @brief Fill a rectangle of identical tiles, specified as a top-left corner
     * location and a size.
     *
     * All coordinates must be in range. This function performs no clipping.
     */
    void fill(UInt2 topLeft, UInt2 size, unsigned tileIndex, unsigned frame = 0)
    {
        while (size.y) {
            span(topLeft, size.x, tileIndex, frame);
            size.y--;
            topLeft.y++;
        }
    }

    /**
     * @brief Fill a rectangle of identical tiles, using the first tile of a
     * pinned asset.
     *
     * All coordinates must be in range. This function performs no clipping.
     */
    void fill(UInt2 topLeft, UInt2 size, const PinnedAssetImage &image, unsigned frame = 0)
    {
        setAssetGroup(image.assetGroup());
        fill(topLeft, size, image.tile(0), frame);
    }

    /**
     * @brief Draw a full AssetImage frame, with its top-left corner at the
     * specified location.
     *
     * Locations are specified in tile units, relative
     * to the top-left of the 18x18 grid.
     *
     * All coordinates must be in range. This function performs no clipping.
     */
    void image(UInt2 pos, const AssetImage &image, unsigned srcFrame = 0, unsigned destFrame = 0)
    {
        setAssetGroup(image.assetGroup());
        ASSERT(pos.x < tileWidth() && pos.x + image.tileWidth() <= tileWidth() &&
               pos.y < tileHeight() && pos.y + image.tileHeight() <= tileHeight() &&
               destFrame < numFrames());
        _SYS_image_memDraw(&tiles[tileAddr(pos, destFrame)],
            _SYS_CUBE_ID_INVALID, image, tileWidth(), srcFrame);
    }

    /**
     * @brief Draw part of an AssetImage frame, with its top-left corner at the
     * specified location.
     *
     * Locations are specified in tile units, relative
     * to the top-left of the 18x18 grid.
     *
     * All coordinates must be in range. This function performs no clipping.
     */
    void image(UInt2 destXY, UInt2 size, const AssetImage &image, UInt2 srcXY,
        unsigned srcFrame = 0, unsigned destFrame = 0)
    {
        setAssetGroup(image.assetGroup());
        ASSERT(destXY.x < tileWidth() && destXY.x + size.x <= tileWidth() &&
               destXY.y < tileHeight() && destXY.y + size.y <= tileHeight() &&
               destFrame < numFrames());
        _SYS_image_memDrawRect(&tiles[tileAddr(destXY, destFrame)],
            _SYS_CUBE_ID_INVALID, image, tileWidth(), srcFrame,
            (_SYSInt2*) &srcXY, (_SYSInt2*) &size);
    }

    /**
     * @brief Draw text, using an AssetImage as a fixed width font.
     *
     * Each character is represented by a consecutive 'frame' in the image.
     * Characters not present in the font will be skipped.
     */
    void text(Int2 topLeft, const AssetImage &font, const char *str,
        unsigned destFrame = 0, char firstChar = ' ')
    {
        setAssetGroup(font.assetGroup());
        uint16_t *addr = &tiles[tileAddr(topLeft, destFrame)];
        uint16_t *lineAddr = addr;
        char c;

        while ((c = *str)) {
            if (c == '\n') {
                addr = (lineAddr += tileWidth());
            } else {
                ASSERT(font.tileWidth() + (font.tileHeight() - 1) * tileWidth() + addr
                    <= numTiles() + tiles);
                _SYS_image_memDraw(addr, _SYS_CUBE_ID_INVALID, font, tileWidth(), c - firstChar);
                addr += font.tileWidth();
            }
            str++;
        }
    }
};


/**
 * @} end addtogroup video
 */

};  // namespace Sifteo
