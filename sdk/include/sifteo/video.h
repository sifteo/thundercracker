/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * This file is part of the public interface to the Sifteo SDK.
 * Copyright <c> 2011 Sifteo, Inc. All rights reserved.
 */

#ifndef _SIFTEO_VIDEO_H
#define _SIFTEO_VIDEO_H

#include <stdio.h>
#include <stdarg.h>

#include <sifteo/macros.h>
#include <sifteo/machine.h>
#include <sifteo/math.h>

namespace Sifteo {


/**
 * A memory buffer which holds graphics data. This is a mirror of the
 * remote graphics memory in each cube's hardware. By writing to this
 * buffer, changes are enqueued for later transmission to the physical
 * video buffer.
 *
 * This class only handles low-level memory management. The layout of
 * this memory is specific to the video mode you're using.
 *
 * If you're accessing the VideoBuffer directly, it is strongly
 * recommended that you're aware of the synchronization protocol used
 * in this buffer, to keep it consistent between the system software
 * and your application.  See abi.h for details on this protocol.
 */

class VideoBuffer {
 public:

    /**
     * Initialize the buffer. Note that this doesn't initialize the
     * *contents* of VRAM, as that's format specific. This just
     * initializes the change maps, so that on the next unlock() we'll
     * send the entire buffer.
     *
     * Keep in mind that this should not be invoked just because
     * you're changing video modes. It's only necessary when first
     * initializing this VideoBuffer for use, or if you're migrating
     * one VideoBuffer to a different cube.
     *
     * If you aren't doing anything fancy, don't call
     * this. Cube::enable() has already done this for you.
     */
    void init() {
        _SYS_vbuf_init(&sys);
    }

    /**
     * Prepare to modify a particular address. Addresses must be
     * locked prior to the buffer memory being modified. Multiple
     * calls to lock may be made in a row, without an intervening
     * unlock().
     */
    void lock(uint16_t addr) {
        _SYS_vbuf_lock(&sys, addr);
    }

    /**
     * End a sequence of modifications to VRAM. If the system is not
     * already busy flushing updates to the cube, this allows it to begin.
     */
    void unlock() {
        _SYS_vbuf_unlock(&sys);
    }

    /**
     * Mark the VideoBuffer as having changed, without actually modifying
     * any memory. This will force the next System::paint() to actually
     * redraw this cube, even if it seems like nothing has changed.
     */
    void touch() {
        Sifteo::Atomic::SetLZ(sys.needPaint, 0);
    }

    /**
     * Modify a word of VRAM, automatically locking it and marking the
     * change only if that word has actually been modified. After a
     * sequence of poke() calls, the caller is responsible for issuing
     * one unlock().
     */
    void poke(uint16_t addr, uint16_t word) {
        _SYS_vbuf_poke(&sys, addr, word);
    }

    /**
     * Like poke(), but modifies a single byte. Less efficient, but
     * sometimes you really do just want to modify one byte.
     */
    void pokeb(uint16_t addr, uint8_t byte) {
        _SYS_vbuf_pokeb(&sys, addr, byte);
    }

    /**
     * Poke a 14-bit tile index into a particular VRAM word.
     */
    void pokei(uint16_t addr, uint16_t index) {
        _SYS_vbuf_poke(&sys, addr, ((index << 2) & 0xFE00) | ((index << 1) & 0x00FE));
    }

    /**
     * Read one word of VRAM
     */
    uint16_t peek(uint16_t addr) const {
        uint16_t word;
        _SYS_vbuf_peek(&sys, addr, &word);
        return word;
    }

    /**
     * Read one byte of VRAM
     */
    uint8_t peekb(uint16_t addr) const {
        uint8_t byte;
        _SYS_vbuf_peekb(&sys, addr, &byte);
        return byte;
    }

    _SYSVideoBuffer sys;    
};


/**
 * Abstract base class for any video mode.  There isn't much that all the
 * video modes have in common, but this class implements that. Most notably,
 * all video modes have the capacity to do split-screen (by restricting
 * the output to a range of scanlines) or to do whole-screen rotation.
 */

class VidMode {
 public:
    VidMode(VideoBuffer &vbuf)
        : buf(vbuf) {}
    
    enum Rotation {
        ROT_NORMAL              = 0,
        ROT_LEFT_90_MIRROR      = _SYS_VF_XY_SWAP,
        ROT_MIRROR              = _SYS_VF_X_FLIP,
        ROT_LEFT_90             = _SYS_VF_XY_SWAP | _SYS_VF_X_FLIP,
        ROT_180_MIRROR          = _SYS_VF_Y_FLIP,
        ROT_RIGHT_90            = _SYS_VF_XY_SWAP | _SYS_VF_Y_FLIP,
        ROT_180                 = _SYS_VF_X_FLIP | _SYS_VF_Y_FLIP,
        ROT_RIGHT_90_MIRROR     = _SYS_VF_XY_SWAP | _SYS_VF_X_FLIP | _SYS_VF_Y_FLIP,
    };

    static const unsigned LCD_width = 128;
    static const unsigned LCD_height = 128;
    static const unsigned TILE = 8;

    void setWindow(uint8_t firstLine, uint8_t numLines) {
        buf.poke(offsetof(_SYSVideoRAM, first_line) / 2, firstLine | (numLines << 8));
    }

    void setRotation(enum Rotation r) {
        const uint8_t mask = _SYS_VF_XY_SWAP | _SYS_VF_X_FLIP | _SYS_VF_Y_FLIP;
        uint8_t flags = buf.peekb(offsetof(_SYSVideoRAM, flags));
        flags &= ~mask;
        flags |= r & mask;
        buf.pokeb(offsetof(_SYSVideoRAM, flags), flags);
    }
    
 protected:
    VideoBuffer &buf;
};


/**
 * A video mode that supports one scrolling layer of 8x8-pixel tiles.
 *
 * The native format of this mode is an 18x18 grid of tiles, with
 * horizontal and vertical wrap-around. Other classes build on this
 * basic primitive to do infinite scrolling, or to add additional
 * layers on top of this background.
 */

class VidMode_BG0 : public VidMode {
 public:
    VidMode_BG0(VideoBuffer &vbuf)
        : VidMode(vbuf) {}
    
    void init() {
        _SYS_vbuf_fill(&buf.sys, 0, 0, BG0_width * BG0_height);
        _SYS_vbuf_pokeb(&buf.sys, offsetof(_SYSVideoRAM, mode), _SYS_VM_BG0);
        BG0_setPanning(Vec2(0,0));
        setWindow(0, LCD_height);
    }

    static const unsigned BG0_width = _SYS_VRAM_BG0_WIDTH;
    static const unsigned BG0_height = _SYS_VRAM_BG0_WIDTH;

    void BG0_setPanning(Vec2 pixels) {
        pixels.x = pixels.x % (int)(BG0_width * TILE);
        pixels.y = pixels.y % (int)(BG0_height * TILE);
        if (pixels.x < 0) pixels.x += BG0_width * TILE;
        if (pixels.y < 0) pixels.y += BG0_height * TILE;
        buf.poke(offsetof(_SYSVideoRAM, bg0_x) / 2, pixels.x | (pixels.y << 8));
    }

    uint16_t BG0_addr(const Vec2 &point) {
        return point.x + (point.y * BG0_width);
    }

    void BG0_putTile(const Vec2 &point, unsigned index) {
        buf.pokei(BG0_addr(point), index);
    }

    /*
     * XXX: We already have two different kinds of assets (pinned vs. mapped) overloaded
     *      in AssetImage. We should probably have STIR generate totally different classes
     *      for assets that have different storage formats, so that we can have overloaded
     *      routines to handle them differently.
     *
     * XXX: Support for pinned assets currently TOTALLY bogus. STIR hasn't even given
     *      us a base address, we're just assuming zero.
     */

    void BG0_drawAsset(const Vec2 &point, const Sifteo::AssetImage &asset, unsigned frame=0) {
        uint16_t addr = BG0_addr(point);
        unsigned offset = asset.width * asset.height * frame;
        const unsigned base = 0;

        for (unsigned y = 0; y < asset.height; y++) {
            if (asset.tiles)
                _SYS_vbuf_writei(&buf.sys, addr, asset.tiles + offset, base, asset.width);
            else
                _SYS_vbuf_seqi(&buf.sys, addr, offset + base, asset.width);

            addr += BG0_width;
            offset += asset.width;
        }
    }

    /**
     * Draw text using a fixed-width font, with a linear mapping from characters to frames.
     *
     * XXX: There will be other font formats, and STIR will probably
     *      generate different kinds of assets for them. Lots of room
     *      for cleanup here...  Especially in the assumed mapping
     *      here! Ugh.
     *
     * XXX: There should also probably just be a higher-level text renderer
     *      that can work with different combinations of video modes and font
     *      types. It sucks to have even high-level things like printf() depend
     *      on those details.
     */

    void BG0_text(const Vec2 &point, const Sifteo::AssetImage &font, char c) {
        unsigned index = c - (int)' ';
        if (index < font.frames)
            BG0_drawAsset(point, font, index);
    }

    void BG0_text(const Vec2 &point, const Sifteo::AssetImage &font, const char *str) {
        Vec2 p = point;
        char c;

        while ((c = *str)) {
            if (c == '\n') {
                p.x = point.x;
                p.y += font.height;
            } else {
                BG0_text(p, font, c);
                p.x += font.width;
            }
            str++;
        }
    }

    void BG0_textf(const Vec2 &point, const Sifteo::AssetImage &font, const char *fmt, ...) {
        char buf[128];
        va_list ap;

        va_start(ap, fmt);
        vsnprintf(buf, sizeof buf - 1, fmt, ap);
        buf[sizeof buf - 1] = 0;
        BG0_text(point, font, buf);
        va_end(ap);
    }
};    
  

/**
 * A video mode that uses only data resident in the cube's ROM. It's
 * equivalent to BG0, except that the tile indices no longer refer to
 * asset data loaded into flash memory. Instead, it's all stored in
 * the cube's ROM.
 *
 * XXX: We should explicitly define which ROM data is part of our stable
 *      ABI, and which should be considered implementation details.
 *
 * XXX: We should add support to STIR for generating graphics using
 *      the ROM atlas as source data for an Asset Group. That would make
 *      it practical for games to ship their own ROM-friendly graphics
 *      if necessary.
 */

class VidMode_BG0_ROM : public VidMode_BG0 {
 public:
    VidMode_BG0_ROM(VideoBuffer &vbuf)
        : VidMode_BG0(vbuf) {}
    
    void init() {
        _SYS_vbuf_fill(&buf.sys, 0, 0, BG0_width * BG0_height);
        _SYS_vbuf_pokeb(&buf.sys, offsetof(_SYSVideoRAM, mode), _SYS_VM_BG0_ROM);
        BG0_setPanning(Vec2(0,0));
        setWindow(0, LCD_height);
    }

    void BG0_text(const Vec2 &point, char c) {
        BG0_putTile(point, c - ' ');
    }

    void BG0_text(const Vec2 &point, const char *str) {
        Vec2 p = point;
        char c;

        while ((c = *str)) {
            if (c == '\n') {
                p.x = point.x;
                p.y++;
            } else {
                BG0_text(p, c);
                p.x++;
            }
            str++;
        }
    }

    void BG0_textf(const Vec2 &point, const char *fmt, ...) {
        char buf[128];
        va_list ap;

        va_start(ap, fmt);
        vsnprintf(buf, sizeof buf - 1, fmt, ap);
        buf[sizeof buf - 1] = 0;
        BG0_text(point, buf);
        va_end(ap);
    }

    void BG0_progressBar(const Vec2 &point, int pixelWidth, int tileHeight=1) {
        /*
         * XXX: This is kind of the hugest hack.. we should have some good way
         *      of using "well-known assets" from ROM somehow. This could either
         *      be via syscalls, or via data that gets published by the firmware's
         *      tilerom generator, or maybe it happens via STIR.
         */

        uint16_t addr = BG0_addr(point);
        int tileWidth = pixelWidth / TILE;
        int remainder = pixelWidth % TILE;

        for (int y = 0; y < tileHeight; y++) {
            _SYS_vbuf_fill(&buf.sys, addr, 0x26fe, tileWidth);
            if (remainder)
                buf.pokei(addr + tileWidth, 0x085f + remainder);
            addr += BG0_width;
        }
    }

};


};  // namespace Sifteo

#endif
