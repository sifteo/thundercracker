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

namespace Sifteo {

/**
 * @addtogroup video
 * @{
 */

/**
 * @brief A VRAM accessor for drawing graphics in the BG0_ROM mode.
 *
 * We have an 18x18 tile grid, just like the normal BG0 mode, but
 * the tile data in our case comes from a built-in image ROM in
 * the cube firmware.
 *
 * This mode can be used to draw specific built-in graphics, or it
 * can be used to draw debug text using the cube's built-in ROM font.
 */

struct BG0ROMDrawable {
    _SYSAttachedVideoBuffer sys;

    /**
     * @brief Palette IDs, XOR'ed with the tile IDs below.
     */
    enum Palette {
        BLACK_ON_WHITE      = 0 << 10,
        BLUE_ON_WHITE       = 1 << 10,
        ORANGE_ON_WHITE     = 2 << 10,
        YELLOW_ON_BLUE      = 3 << 10,
        RED_ON_WHITE        = 4 << 10,
        GRAY_ON_WHITE       = 5 << 10,
        WHITE_ON_BLACK      = 6 << 10,
        WHITE_ON_BLUE       = 7 << 10,
        WHITE_ON_TEAL       = 8 << 10,
        BLACK_ON_YELLOW     = 9 << 10,
        DKGRAY_ON_LTGRAY    = 10 << 10,
        GREEN_ON_WHITE      = 11 << 10,
        WHITE_ON_GREEN      = 12 << 10,
        PURPLE_ON_WHITE     = 13 << 10,
        LTBLUE_ON_DKBLUE    = 14 << 10,
        GOLD_ON_WHITE       = 15 << 10,

        // Aliases for white background
        BLACK   = BLACK_ON_WHITE,
        BLUE    = BLUE_ON_WHITE,
        ORANGE  = ORANGE_ON_WHITE,
        RED     = RED_ON_WHITE,
        GRAY    = GRAY_ON_WHITE,
        GREEN   = GREEN_ON_WHITE,
        PURPLE  = PURPLE_ON_WHITE,
        GOLD    = GOLD_ON_WHITE,
    };

    /**
     * @brief Well-known tile numbers.
     */
    enum Tiles {
        FONT_SPACE  = 0,        ///< First character in the font, ASCII space
        SOLID_BG    = 0,        ///< Solid background-colored tile (space)
        SOLID_FG    = 104,      ///< Solid foreground-colored tile
        V_BARGRAPH  = 224,      ///< First tile in the vertical bargraph series
        H_BARGRAPH  = 231,      ///< First tile in the horizontal bargraph series
    };

    /**
     * @brief Color modes, XOR'ed with the tile IDs below.
     */
    enum ColorMode {
        TWO_COLOR   = 0 << 9,
        FOUR_COLOR  = 1 << 9,
    };

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
     * @brief Calculate the video buffer address of a particular tile.
     *
     * All coordinates must be in range. This function performs no clipping.
     */
    uint16_t tileAddr(UInt2 pos) {
        return pos.x + pos.y * tileWidth();
    }

    /**
     * @brief Erase mode-specific VRAM, filling the BG0 buffer with the specified
     * value and resetting the panning registers.
     */
    void erase(uint16_t index = 0) {
        _SYS_vbuf_fill(&sys.vbuf, 0, _SYS_TILE77(index), sizeInWords());
        setPanning(vec(0,0));
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
     * @brief Calculate the tile index of one character in the ROM font.
     */
    static uint16_t charTile(char c, enum Palette palette = BLACK) {
        return palette ^ (c - ' ' + FONT_SPACE);
    }

    /**
     * @brief Plot a single tile, at location 'pos', in tile units.
     *
     * All coordinates must be in range. This function performs no clipping.
     */
    void plot(UInt2 pos, uint16_t tileIndex) {
        ASSERT(pos.x < tileWidth() && pos.y < tileHeight());
        _SYS_vbuf_poke(&sys.vbuf, tileAddr(pos), _SYS_TILE77(tileIndex));
    }

    /**
     * @brief Plot a horizontal span of tiles, given the position of the
     * leftmost tile, and the number of tiles to plot.
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
     * @brief Draw a horizontal bargraph, with its top-left corner position
     * specified in tiles, and its width in pixels.
     *
     * The progress bar is drawn using tiles from the ROM. Fully empty tiles
     * are not drawn. Completely full tiles are drawn using a SOLID_FG tile,
     * and partially full tiles use the BARGRAPH series of tile images.
     */
    void hBargraph(Int2 topLeft, unsigned pixelWidth,
        enum Palette palette = BLACK, unsigned tileHeight = 1)
    {
        unsigned addr = tileAddr(topLeft);
        int wTiles = pixelWidth / 8;
        int wRemainder = pixelWidth % 8;

        while (tileHeight--) {
            _SYS_vbuf_fill(&sys.vbuf, addr,
                _SYS_TILE77(palette ^ SOLID_FG), wTiles);
            if (wRemainder)
                _SYS_vbuf_poke(&sys.vbuf, addr + wTiles,
                    _SYS_TILE77(palette ^ (H_BARGRAPH + wRemainder - 1)));
            addr += tileWidth();
        }
    }

    /**
     * @brief Draw text, using the builtin ROM font, starting at
     * location 'topLeft' in tiles.
     *
     * Drawing characters not present in the ROM font will have
     * undefined results.
     */
    void text(Int2 topLeft, const char *str, enum Palette palette = BLACK)
    {
        unsigned addr = tileAddr(topLeft);
        unsigned lineAddr = addr;
        char c;

        while ((c = *str)) {
            if (c == '\n')
                addr = (lineAddr += tileWidth());
            else
                _SYS_vbuf_poke(&sys.vbuf, addr++, _SYS_TILE77(charTile(c, palette)));
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

