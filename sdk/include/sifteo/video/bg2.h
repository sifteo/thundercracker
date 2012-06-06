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
#include <sifteo/video/color.h>

namespace Sifteo {

/**
 * @addtogroup video
 * @{
 */

/**
 * @brief A VRAM accessor for drawing graphics in the BG2 mode.
 *
 * This is a special-purpose mode for doing scaling, rotation, and
 * other effects using an affine transform matrix.
 *
 * Conceptually, this mode is a 256x256-pixel canvas, repeating forever
 * in both axes, in which the top-left quadrant is a 16x16-tile drawable,
 * and the other three quadrants are painted with a solid "border" color.
 *
 * This arrangement allows you to treat the 16x16 area as "centered" on the
 * screen, with the border color poking through when that image is scaled
 * down or rotated.
 *
 * Note that the border color is not part of the Colormap, it is a separate
 * piece of state that can be set through the BG2Drawable.
 */
struct BG2Drawable {
    _SYSAttachedVideoBuffer sys;

    /**
     * @brief Return the width, in tiles, of this mode
     */
    static unsigned tileWidth() {
        return _SYS_VRAM_BG2_WIDTH;
    }

    /**
     * @brief Return the height, in tiles, of this mode
     */
    static unsigned tileHeight() {
        return _SYS_VRAM_BG2_WIDTH;
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
     * @brief Calculate the video buffer address of a particular tile.
     *
     * All coordinates must be in range. This function performs no clipping.
     */
    uint16_t tileAddr(UInt2 pos) {
        return pos.x + pos.y * tileWidth();
    }

    /**
     * @brief Erase mode-specific VRAM, filling the BG2 buffer with the specified
     * absolute tile index value.
     *
     * Does not modify the affine transform or the border color.
     */
    void erase(uint16_t index = 0) {
        _SYS_vbuf_fill(&sys.vbuf, 0, _SYS_TILE77(index), sizeInWords());
    }

    /**
     * @brief Erase mode-specific VRAM, filling the BG2 buffer with the first tile
     * from the specified PinnedAssetImage.
     */
    void erase(const PinnedAssetImage &image) {
        erase(image.tile(sys.cube, 0));
    }
    
    /**
     * @brief Set the border color, given an arbitrary 16-bit RGB565 color.
     *
     * This color is displayed 'outside' the 16x16-tile drawable region,
     * in the other three quadrants of the virtual 256x256-pixel plane.
     */
    void setBorder(RGB565 color) {
        _SYS_vbuf_poke(&sys.vbuf, offsetof(_SYSVideoRAM, bg2_border) / 2,
            color.value);
    }

    /**
     * @brief Get the last border color set by setBorder().
     */
    RGB565 getBorder() const {
        RGB565 result = { _SYS_vbuf_peek(&sys.vbuf, offsetof(_SYSVideoRAM, bg2_border)) };
        return result;
    }

    /**
     * @brief Set the current affine transform matrix.
     */
    void setMatrix(const AffineMatrix &m) {
        _SYSAffine a = {
            // Round to fixed-point
            256.0f * m.cx + 0.5f,
            256.0f * m.cy + 0.5f,
            256.0f * m.xx + 0.5f,
            256.0f * m.xy + 0.5f,
            256.0f * m.yx + 0.5f,
            256.0f * m.yy + 0.5f,
        };
        _SYS_vbuf_write(&sys.vbuf, offsetof(_SYSVideoRAM, bg2_affine)/2,
                        (const uint16_t *)&a, 6);
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
     * @brief Plot a horizontal span of tiles, by absolue tile index,
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
     * @brief Draw a full AssetImage frame, with its top-left corner at the
     * specified location.
     *
     * Locations are specified in tile units, relative to the top-left of the 18x18 grid.
     *
     * All coordinates must be in range. This function performs no clipping.
     */
    void image(UInt2 pos, const AssetImage &image, unsigned frame = 0)
    {
        _SYS_image_BG2Draw(&sys, image, tileAddr(pos), frame);
    }

    /**
     * @brief Draw part of an AssetImage frame, with its top-left corner at the
     * specified location.
     *
     * Locations are specified in tile units, relative to the top-left of the 18x18 grid.
     *
     * All coordinates must be in range. This function performs no clipping.
     */
    void image(UInt2 destXY, UInt2 size, const AssetImage &image, UInt2 srcXY, unsigned frame = 0)
    {
        _SYS_image_BG2DrawRect(&sys, image, tileAddr(destXY),
            frame, (_SYSInt2*) &srcXY, (_SYSInt2*) &size);
    }

    /**
     * @brief Draw text, using an AssetImage as a fixed width font.
     *
     * Each character is represented by a consecutive 'frame' in the image. Characters not
     * present in the font will be skipped.
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
                _SYS_image_BG2Draw(&sys, font, addr, c - firstChar);
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
