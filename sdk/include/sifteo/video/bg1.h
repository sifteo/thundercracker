/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * This file is part of the public interface to the Sifteo SDK.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#ifndef _SIFTEO_VIDEO_BG1_H
#define _SIFTEO_VIDEO_BG1_H

#ifdef NO_USERSPACE_HEADERS
#   error This is a userspace-only header, not allowed by the current build.
#endif

#include <sifteo/abi.h>
#include <sifteo/macros.h>
#include <sifteo/math.h>
#include <sifteo/asset.h>

namespace Sifteo {


/**
 * This is a VRAM accessor for drawing graphics in the BG1 mode.
 *
 * BG1 is an overlay layer, existing above BG0 and all Sprites.
 * It consists of a virtual 16x16-tile grid which does not wrap.
 * Tiles beyond the edges of this grid are treated as transparent.
 *
 * Because of video memory size limitations, the hardware cannot
 * keep use every tile in this 16x16 grid simultaneously. Up to
 * 144 tiles are available, which is only enough to cover 56%
 * of the full screen.
 *
 * This layer is represented by a packed array of 144 tile indices,
 * plus a separate 16x16-bit bitmap which tracks which tiles in the
 * virtual grid have been allocated. Allocated tiles are mapped to
 * locations in the 144-tile array, by walking top-to-bottom
 * left-to-right and allocating a tile any time there's a '1' bit
 * in the allocation bitmap. Unallocated tiles are rendered as
 * fully transparent.
 */
struct BG1Drawable {
    _SYSAttachedVideoBuffer sys;

    /**
     * Return the width, in tiles, of this mode
     */
    static unsigned tileWidth() {
        return _SYS_VRAM_BG1_WIDTH;
    }

    /**
     * Return the height, in tiles, of this mode
     */
    static unsigned tileHeight() {
        return _SYS_VRAM_BG1_WIDTH;
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
     * Returns the maximum number of allcoated tiles.
     */
    static unsigned numTiles() {
        return _SYS_VRAM_BG1_TILES;
    }

    /**
     * Erase mode-specific VRAM, filling the BG1 buffer with the specified
     * absolute tile index value, clearing the allocation mask,
     * and resetting the panning registers.
     */
    void erase(uint16_t index = 0) {
        eraseMask();
        _SYS_vbuf_fill(&sys.vbuf, _SYS_VA_BG1_TILES / 2,
            _SYS_TILE77(index), numTiles());
        setPanning(vec(0,0));
    }

    /**
     * Erase mode-specific VRAM, filling the BG1 buffer with the first tile
     * from the specified PinnedAssetImage, clearing the allocation mask,
     * and resetting the panning registers.
     */
    void erase(const PinnedAssetImage &image) {
        erase(image.tile(sys.cube, 0));
    }

    /**
     * Erase just the allocation mask. All tiles will now be unallocated,
     * and BG1 will be fully transparent.
     */
    void eraseMask() {
        _SYS_vbuf_fill(&sys.vbuf, _SYS_VA_BG1_BITMAP, 0, _SYS_VRAM_BG1_WIDTH);
    }

    /**
     * Change the hardware pixel-panning origin for this mode. The supplied
     * vector is interpreted as the location on the tile buffer, in pixels,
     * where the origin of the LCD will begin.
     *
     * BG1 is an 16x16 buffer that does not wrap. The panning value can
     * be negative, but only panning of up to +/- 128 is supported.
     */ 
    void setPanning(Int2 pixels) {
        ASSERT(pixels.x < 128 && pixels.x > -128 && pixels.y < 128 && pixels.y > -128);
        _SYS_vbuf_poke(&sys.vbuf, offsetof(_SYSVideoRAM, bg1_x) / 2,
            (int8_t)pixels.x | ((uint16_t)(int8_t)pixels.y << 8));
    }

    /**
     * Plot a single tile, by absolute tile index,
     * at a specific location in the 144-tile array.
     */
    void plot(unsigned locationIndex, uint16_t tileIndex) {
        ASSERT(locationIndex < numTiles());
        _SYS_vbuf_poke(&sys.vbuf, _SYS_VA_BG1_TILES / 2 + locationIndex,
            _SYS_TILE77(tileIndex));
    }

    /**
     * Plot a horizontal span of tiles in consecutively allocated
     * locations in the BG1 tile array. These tiles may not necessarily
     * be consecutive on-screen.
     */
    void span(unsigned locationIndex, unsigned count, unsigned tileIndex)
    {
        ASSERT(locationIndex <= numTiles() && count <= numTiles() &&
            (locationIndex + count) < numTiles());
        _SYS_vbuf_fill(&sys.vbuf, _SYS_VA_BG1_TILES / 2 + locationIndex,
            _SYS_TILE77(tileIndex), count);
    }

    /**
     * Draw a full AssetImage frame, with its top-left corner at the
     * specified location. Locations are specified in tile units, relative
     * to the top-left of the 18x18 grid.
     *
     * Tiles are located in the BG1 array based on the current mask.
     * Any tiles not allocated by the mask are not drawn. (There will be a
     * "hole" in your asset there.)
     */
    void image(UInt2 pos, const AssetImage &image, unsigned frame = 0)
    {
        _SYS_image_BG1Draw(&sys, image, (_SYSInt2*) &pos, frame);
    }

    /**
     * Draw part of an AssetImage frame, with its top-left corner at the
     * specified location. Locations are specified in tile units, relative
     * to the top-left of the 18x18 grid.
     *
     * Tiles are located in the BG1 array based on the current mask.
     * Any tiles not allocated by the mask are not drawn. (There will be a
     * "hole" in your asset there.)
     */
    void image(UInt2 destXY, UInt2 size, const AssetImage &image, UInt2 srcXY, unsigned frame = 0)
    {
        _SYS_image_BG1DrawRect(&sys, image, (_SYSInt2*) &destXY,
            frame, (_SYSInt2*) &srcXY, (_SYSInt2*) &size);
    }

    /**
     * Draw text, using an AssetImage as a fixed width font. Each character
     * is represented by a consecutive 'frame' in the image. Characters not
     * present in the font will be skipped.
     */
    void text(Int2 topLeft, const AssetImage &font, const char *str, char firstChar = ' ')
    {
        Int2 pos = topLeft;
        char c;

        while ((c = *str)) {
            if (c == '\n') {
                pos.x = topLeft.x;
                pos.y++;
            } else {
                _SYS_image_BG1Draw(&sys, font, (_SYSInt2*) &pos, c - firstChar);
                pos.x++;
            }
            str++;
        }
    }

    /**
     * Return the VideoBuffer associated with this drawable.
     */
    _SYSVideoBuffer &videoBuffer() {
        return sys.vbuf;
    }

    /**
     * Return the CubeID associated with this drawable.
     */
    CubeID cube() const {
        return sys.cube;
    }
};


};  // namespace Sifteo

#endif
