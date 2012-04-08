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
 * A BG1 tile mask. In other words, this is a 16x16-bit two-dimensional
 * vector.
 *
 * It is optimized for compile-time arithmetic, so that it's easy and
 * efficient to build up static masks which correspond to a particular
 * screen layout or asset layout. But these vectors can also be used to
 * easily implement some fairly complex logic at runtime. They support
 * boolean operations, which can be used for geometric union, intersection,
 * and subtraction operations.
 */

struct BG1Mask {
    // Matches the memory layout of BG1, but uses faster 32-bit arithmetic.
    uint32_t row10;
    uint32_t row32;
    uint32_t row54;
    uint32_t row76;
    uint32_t row98;
    uint32_t rowBA;
    uint32_t rowDC;
    uint32_t rowFE;

    /**
     * Create an empty mask. All bits are zero.
     */
    static BG1Mask empty() {
        BG1Mask result = { 0, 0, 0, 0, 0, 0, 0, 0 };
        return result;
    }

    /**
     * Create a mask with a filled rectangle it.
     *
     * This should be used only with values that are constant
     * at compile-time. For dynamic masks, it is significantly
     * better to plot the rectangle dynamically using fill().
     * 
     * All coordinates must be in range. This function performs no clipping.
     */
    static BG1Mask filled(UInt2 topLeft, UInt2 size) {
        unsigned xBits = ((1 << size.x) - 1) << topLeft.x;
        unsigned yBits = ((1 << size.y) - 1) << topLeft.y;

        unsigned row0 = yBits & 0x0001 ? xBits : 0;
        unsigned row1 = yBits & 0x0002 ? xBits : 0;
        unsigned row2 = yBits & 0x0004 ? xBits : 0;
        unsigned row3 = yBits & 0x0008 ? xBits : 0;
        unsigned row4 = yBits & 0x0010 ? xBits : 0;
        unsigned row5 = yBits & 0x0020 ? xBits : 0;
        unsigned row6 = yBits & 0x0040 ? xBits : 0;
        unsigned row7 = yBits & 0x0080 ? xBits : 0;
        unsigned row8 = yBits & 0x0100 ? xBits : 0;
        unsigned row9 = yBits & 0x0200 ? xBits : 0;
        unsigned rowA = yBits & 0x0400 ? xBits : 0;
        unsigned rowB = yBits & 0x0800 ? xBits : 0;
        unsigned rowC = yBits & 0x1000 ? xBits : 0;
        unsigned rowD = yBits & 0x2000 ? xBits : 0;
        unsigned rowE = yBits & 0x4000 ? xBits : 0;
        unsigned rowF = yBits & 0x8000 ? xBits : 0;

        BG1Mask result = {
            (row1 << 16) | row0,
            (row3 << 16) | row2,
            (row5 << 16) | row4,
            (row7 << 16) | row6,
            (row9 << 16) | row8,
            (rowB << 16) | rowA,
            (rowD << 16) | rowC,
            (rowF << 16) | rowE,
        };
        return result;
    }

    /**
     * Erase a TileMask, setting all bits to zero.
     */
    void clear() {
        _SYS_memset32(&row10, 0, 8);
    }

    /**
     * Get a pointer to the rows in this mask, as 16-bit integers.
     */
    uint16_t *rows() {
        return (uint16_t*) this;
    }
    const uint16_t *rows() const {
        return (const uint16_t*) this;
    }

    /**
     * Mark one tile in the bitmap, given as (x,y) coordinates.
     * All coordinates must be in range. This function performs no clipping.
     */
    void plot(unsigned x, unsigned y) {
        ASSERT(x < 16 && y < 16);
        rows()[y] |= 1 << x;
    }

    /**
     * Mark one tile in the bitmap, given as a vector.
     * All coordinates must be in range. This function performs no clipping.
     */
    void plot(UInt2 pos) {
        plot(pos.x, pos.y);
    }

    /**
     * Mark a rectangular region of the bitmap.
     * All coordinates must be in range. This function performs no clipping.
     */
    void fill(UInt2 topLeft, UInt2 size) {
        for (unsigned y = 0; y < size.y; y++)
            for (unsigned x = 0; x < size.x; x++)
                plot(topLeft.x + x, topLeft.y + y);
    }

    BG1Mask & operator|= (BG1Mask other) {
        row10 |= other.row10;
        row32 |= other.row32;
        row54 |= other.row54;
        row76 |= other.row76;
        row98 |= other.row98;
        rowBA |= other.rowBA;
        rowDC |= other.rowDC;
        rowFE |= other.rowFE;
        return *this;
    }

    BG1Mask & operator&= (BG1Mask other) {
        row10 &= other.row10;
        row32 &= other.row32;
        row54 &= other.row54;
        row76 &= other.row76;
        row98 &= other.row98;
        rowBA &= other.rowBA;
        rowDC &= other.rowDC;
        rowFE &= other.rowFE;
        return *this;
    }

    BG1Mask & operator^= (BG1Mask other) {
        row10 ^= other.row10;
        row32 ^= other.row32;
        row54 ^= other.row54;
        row76 ^= other.row76;
        row98 ^= other.row98;
        rowBA ^= other.rowBA;
        rowDC ^= other.rowDC;
        rowFE ^= other.rowFE;
        return *this;
    }

    BG1Mask operator| (BG1Mask other) const {
         BG1Mask result = {
             row10 | other.row10,
             row32 | other.row32,
             row54 | other.row54,
             row76 | other.row76,
             row98 | other.row98,
             rowBA | other.rowBA,
             rowDC | other.rowDC,
             rowFE | other.rowFE,
        };
        return result;
    }

    BG1Mask operator& (BG1Mask other) const {
         BG1Mask result = {
             row10 & other.row10,
             row32 & other.row32,
             row54 & other.row54,
             row76 & other.row76,
             row98 & other.row98,
             rowBA & other.rowBA,
             rowDC & other.rowDC,
             rowFE & other.rowFE,
        };
        return result;
    }

    BG1Mask operator^ (BG1Mask other) const {
         BG1Mask result = {
             row10 ^ other.row10,
             row32 ^ other.row32,
             row54 ^ other.row54,
             row76 ^ other.row76,
             row98 ^ other.row98,
             rowBA ^ other.rowBA,
             rowDC ^ other.rowDC,
             rowFE ^ other.rowFE,
        };
        return result;
    }

    BG1Mask operator~ () const {
         BG1Mask result = {
             ~row10,
             ~row32,
             ~row54,
             ~row76,
             ~row98,
             ~rowBA,
             ~rowDC,
             ~rowFE,
        };
        return result;       
    }
};


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
     * Change the tile allocation bitmap.
     *
     * Changes to the bitmap will affect the way the tile array is
     * interpreted, causing already-drawn tiles to appear to shift.
     * Normally you should only change the allocation map when the
     * BG1 mode isn't currently active, or when you've ensured
     * that the cube isn't currently rendering asynchronously.
     *
     * Because of this, we automatically do a System::finish() by default,
     * so we can ensure nobody is still using the old mask. This can be
     * suppressed if you really know what you're doing, by setting
     * finish=false.
     */
    void setMask(const BG1Mask &mask, bool finish=true) {
        if (finish)
            _SYS_finish();
        _SYS_vbuf_write(&sys.vbuf, offsetof(_SYSVideoRAM, bg1_bitmap)/2,
            mask.rows(), 16);
    }

    /**
     * Change the hardware pixel-panning origin for this mode. The supplied
     * vector is interpreted as the location on the tile buffer, in pixels,
     * where the origin of the LCD will begin.
     *
     * BG1 is an 16x16 buffer that does not wrap. The panning value can
     * be negative, but only panning of up to +/- 128 is supported. Larger
     * values will cause wraparound.
     */ 
    void setPanning(Int2 pixels) {
        unsigned x = pixels.x & 0xFF;
        unsigned y = pixels.y & 0xFF;
        _SYS_vbuf_poke(&sys.vbuf, offsetof(_SYSVideoRAM, bg1_x) / 2, x | (y << 8));
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
