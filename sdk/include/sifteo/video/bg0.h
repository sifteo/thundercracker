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

namespace Sifteo {

/**
 * @addtogroup video
 * @{
 */

/**
 * @brief A VRAM accessor for drawing graphics in the BG0 mode.
 *
 * We have an 18x18 tile grid, which wraps around in both axes.
 */
struct BG0Drawable {
    _SYSAttachedVideoBuffer sys;

    /**
     * @brief Return the width, in tiles, of this mode
     */
    static unsigned tileWidth() {
        return _SYS_VRAM_BG0_WIDTH;
    }

    /**
     * @brief Return the height, in tiles, of this mode
     */
    static unsigned tileHeight() {
        return _SYS_VRAM_BG0_WIDTH;
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
     * @brief Returns the size of this drawable's tile data, in bytes
     */
    static unsigned sizeInBytes() {
        return tileWidth() * tileHeight() * 2;
    }

    /**
     * @brief Returns the size of this drawable's tile data, in 16-bit words
     */
    static unsigned sizeInWords() {
        return tileWidth() * tileHeight();
    }

    /**
     * @brief Erase mode-specific VRAM, filling the BG0 buffer with the specified
     * absolute tile index value and resetting the panning registers.
     */
    void erase(uint16_t index = 0) {
        _SYS_vbuf_fill(&sys.vbuf, 0, _SYS_TILE77(index), sizeInWords());
        setPanning(vec(0,0));
    }

    /**
     * @brief Erase mode-specific VRAM, filling the BG0 buffer with the first tile
     * from the specified PinnedAssetImage and resetting the panning registers.
     */
    void erase(const PinnedAssetImage &image) {
        erase(image.tile(sys.cube, 0));
    }

    /**
     * @brief Change the hardware pixel-panning origin for this mode.
     *
     * The supplied vector is interpreted as the location on the tile buffer, in pixels,
     * where the origin of the LCD will begin.
     *
     * BG0 is an 18x18 buffer that wraps around in both directions.
     */ 
    void setPanning(Int2 pixels) {
        _SYS_vbuf_poke(&sys.vbuf, offsetof(_SYSVideoRAM, bg0_x) / 2,
            umod(pixels.x, pixelWidth()) |
            (umod(pixels.y, pixelHeight()) << 8));
    }

    /**
     * @brief Retrieve the last value set by setPanning(), modulo the layer size
     * in pixels.
     */
    Int2 getPanning() const {
        unsigned word = _SYS_vbuf_peek(&sys.vbuf, offsetof(_SYSVideoRAM, bg0_x) / 2);
        return vec<int>(word & 0xFF, word >> 8);
    }

    /**
     * @brief Calculate the video buffer address of a particular tile.
     *
     * All coordinates must be in range. This function performs no clipping.
     */
    uint16_t tileAddr(UInt2 pos) {
        return pos.x + pos.y * tileWidth();
    }

	/**
	 * @brief Retrieve the absolute tile index currently set at the 
	 * given address.  The inverse of plot().
	 */
	uint16_t tile(UInt2 pos) {
        ASSERT(pos.x < tileWidth() && pos.y < tileHeight());
		return _SYS_INVERSE_TILE77(_SYS_vbuf_peek(&sys.vbuf, tileAddr(pos)));
	}
	
    /**
     * @brief Plot a single tile, by absolute tile index,
     * at location 'pos' in tile units.
     *
     * All coordinates must be in range. This function performs no clipping.
     */
    void plot(UInt2 pos, uint16_t tileIndex) {
        ASSERT(pos.x < tileWidth() && pos.y < tileHeight());
        _SYS_vbuf_poke(&sys.vbuf, tileAddr(pos), _SYS_TILE77(tileIndex));
    }

    /**
     * @brief Plot a horizontal span of tiles, by absolute tile index,
     * given the position of the leftmost tile and the number of tiles to plot.
     *
     * All coordinates must be in range. This function performs no clipping.
     */
    void span(UInt2 pos, unsigned width, unsigned tileIndex)
    {
        ASSERT(pos.x <= tileWidth() && width <= tileWidth() &&
            (pos.x + width) <= tileWidth() && pos.y < tileHeight());
        _SYS_vbuf_fill(&sys.vbuf, tileAddr(pos), _SYS_TILE77(tileIndex), width);
    }

    /**
     * @brief Plot a horizontal span of tiles, using the first tile of a pinned asset.
     *
     * All coordinates must be in range. This function performs no clipping.
     */
    void span(UInt2 pos, unsigned width, const PinnedAssetImage &image)
    {
        span(pos, width, image.tile(sys.cube,0));
    }

    /**
     * @brief Fill a rectangle of identical tiles, specified as a top-left corner
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
     * @brief Fill a rectangle of identical tiles, using the first tile of a
     * pinned asset.
     *
     * All coordinates must be in range. This function performs no clipping.
     */
    void fill(UInt2 topLeft, UInt2 size, const PinnedAssetImage &image)
    {
        fill(topLeft, size, image.tile(sys.cube, 0));
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
    void image(UInt2 pos, const AssetImage &image, unsigned frame = 0)
    {
        _SYS_image_BG0Draw(&sys, image, tileAddr(pos), frame);
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
    void image(UInt2 destXY, UInt2 size, const AssetImage &image, UInt2 srcXY, unsigned frame = 0)
    {
        _SYS_image_BG0DrawRect(&sys, image, tileAddr(destXY),
            frame, (_SYSInt2*) &srcXY, (_SYSInt2*) &size);
    }

    /**
     * @brief Draw text, using an AssetImage as a fixed width font.
     *
     * Each character is represented by a consecutive 'frame' in the image.
     * Characters not present in the font will be skipped.
     */
    void text(Int2 topLeft, const AssetImage &font, const char *str, char firstChar = ' ')
    {
        unsigned addr = tileAddr(topLeft);
        unsigned lineAddr = addr;
        char c;

        while ((c = *str)) {
            if (c == '\n') {
                addr = (lineAddr += font.tileHeight() * tileWidth());
            } else {
                _SYS_image_BG0Draw(&sys, font, addr, c - firstChar);
                addr += font.tileWidth();
            }
            str++;
        }
    }

    /**
     * @brief Return the VideoBuffer associated with this drawable.
     */
    _SYSVideoBuffer &videoBuffer() {
        return sys.vbuf;
    }

    /**
     * @brief Return the CubeID associated with this drawable.
     */
    CubeID cube() const {
        return sys.cube;
    }
};

/**
 * @} end addtogroup video
 */

};  // namespace Sifteo
