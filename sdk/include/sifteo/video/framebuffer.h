/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * This file is part of the public interface to the Sifteo SDK.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#ifndef _SIFTEO_VIDEO_FRAMEBUFFER_H
#define _SIFTEO_VIDEO_FRAMEBUFFER_H

#ifdef NO_USERSPACE_HEADERS
#   error This is a userspace-only header, not allowed by the current build.
#endif

#include <sifteo/abi.h>
#include <sifteo/macros.h>
#include <sifteo/math.h>

namespace Sifteo {


/**
 * An FBDrawable object is a VRAM accessor for drawing pixel graphics,
 * in one of the cube's supported framebuffer drawing modes.
 *
 * FBDrawable is a template which is parameterized for each of the
 * supported video modes. Normally, you'll access these specific
 * FBDrawable instances via a VideoBuffer's fb32, fb64, and fb128 members.
 */
template <unsigned tWidth, unsigned tHeight, unsigned tBitsPerPixel>
struct FBDrawable {
    _SYSAttachedVideoBuffer sys;

    /**
     * Return the width, in pixels, of this mode
     */
    static unsigned width() {
        return tWidth;
    }

    /**
     * Return the height, in pixels, of this mode
     */
    static unsigned height() {
        return tHeight;
    }

    /**
     * Return the size of this mode as a vector, in pixels.
     */
    static UInt2 size() {
        return Vec2(tWidth, tHeight);
    }

    /**
     * Return the total number of colors this mode supports. This is
     * equal to the number of colormap entries used by the mode.
     */
    static unsigned numColors() {
        return 1 << tBitsPerPixel;
    }

    /**
     * Returns the number of bits per pixel this framebuffer mode uses
     * to store color indices.
     */
    static unsigned bitsPerPixel() {
        return tBitsPerPixel;
    }

    /**
     * Returns the size of this framebuffer's data, in bytes
     */
    static unsigned sizeInBytes() {
        return tWidth * tHeight * tBitsPerPixel / 8;
    }

    /**
     * Returns the size of this framebuffer's data, in 16-bit words
     */
    static unsigned sizeInWords() {
        return tWidth * tHeight * tBitsPerPixel / 16;
    }

    /**
     * Plot a single pixel, at the specified location. Locations are
     * specified in pixels, with (0,0) at the top-left corner, +X to the
     * right, and +Y down.
     *
     * The pixel coordinates must be in range. This call does not perform
     * any clipping.
     */
    void plot(unsigned colorIndex, UInt2 pos)
    {
        const unsigned pixelsPerByte = 8 / tBitsPerPixel;
        const unsigned bytesPerLine = tWidth / pixelsPerByte;
        const unsigned xByte = pos.x / pixelsPerByte;
        const unsigned xBit = (pos.x % pixelsPerByte) * tBitsPerPixel;
        const unsigned byteAddr = xByte + pos.y * bytesPerLine;
        const unsigned pixelMask = ((1 << tBitsPerPixel) - 1) << xBit;

        uint8_t byte = _SYS_vbuf_peekb(&sys.vbuf, byteAddr);
        byte &= ~pixelMask;
        byte |= colorIndex << xBit;
        _SYS_vbuf_pokeb(&sys.vbuf, byteAddr, byte);
    }

    /**
     * Given a single color index, return an expanded version where
     * the single color has been replicated to fill a 16-bit word.
     * This can be used to implement fills, since VideoBuffers operate
     * in units of 16 bits at a time.
     */
    static uint16_t expand16(unsigned colorIndex)
    {
        switch (tBitsPerPixel) {
            case 1:  return colorIndex ? 0xFFFF : 0x0000;
            case 2:  colorIndex |= colorIndex << 2;
            case 4:  colorIndex |= colorIndex << 4;
            case 8:  colorIndex |= colorIndex << 8;
            case 16: return colorIndex;
            default: ASSERT(0);
        }
    }

    /**
     * Plot a horizontal span of pixels, given the position of the
     * leftmost pixel, and the number of pixels to plot.
     *
     * All coordinates must be in range. This function performs no clipping.
     */
    void span(unsigned colorIndex, UInt2 pos, unsigned width)
    {
        ASSERT(pos.x <= tWidth && width <= tWidth &&
            (pos.x + width) <= tWidth && pos.y < tHeight);

        const unsigned pixelsPerWord = 16 / tBitsPerPixel;
        const unsigned wordsPerLine = tWidth / pixelsPerWord;
        const unsigned colorWord = expand16(colorIndex);

        // Calculate a base address in words, and
        // left/right boundaries relative to that in bits.
        unsigned addr = pos.x / pixelsPerWord + pos.y * wordsPerLine;
        int start = (pos.x % pixelsPerWord) * tBitsPerPixel;
        int end = start + width * tBitsPerPixel;

        while (end > 0) {
            if (start <= 0 && end >= 16) {
                // A run of complete words

                unsigned count = end / 16;
                _SYS_vbuf_fill(&sys.vbuf, addr, colorWord, count);
                addr += count;
                unsigned bits = count * 16;
                start -= bits;
                end -= bits;

            } else {
                // One partial word. (The first, last or only word)

                unsigned mask = bitRange<uint16_t>(start, end);
                unsigned word = _SYS_vbuf_peek(&sys.vbuf, addr);
                word &= ~mask;
                word |= colorWord & mask;
                _SYS_vbuf_poke(&sys.vbuf, addr, word);
                addr++;
                start -= 16;
                end -= 16;
            }
        }
    }

    /**
     * Fill a rectangle of pixels, specified as a top-left corner
     * location and a size.
     *
     * All coordinates must be in range. This function performs no clipping.
     */
    void fill(unsigned colorIndex, UInt2 topLeft, UInt2 size)
    {
        while (size.y) {
            span(colorIndex, topLeft, size.x);
            size.y--;
            topLeft.y++;
        }
    }

    /**
     * Fill the entire framebuffer with a specific color index.
     */
    void fill(unsigned colorIndex)
    {
        _SYS_vbuf_fill(&sys.vbuf, 0, expand16(colorIndex), sizeInWords());
    }

    /**
     * Draw a span of pixels from a packed-pixel bitmap, of the
     * same color depth as this framebuffer mode.
     *
     * All coordinates must be in range. This function performs no clipping.
     */
    void bitmapSpan(const uint8_t *data, UInt2 pos, unsigned width)
    {
        ASSERT(pos.x <= tWidth && width <= tWidth &&
            (pos.x + width) <= tWidth && pos.y < tHeight);

        const unsigned pixelsPerByte = 8 / tBitsPerPixel;
        const unsigned bytesPerLine = tWidth / pixelsPerByte;

        // Base address in bits
        unsigned addr = pos.x / pixelsPerByte + pos.y * bytesPerLine;

        // Bitmap alignment
        unsigned shift = (pos.x % pixelsPerByte) * tBitsPerPixel;
        unsigned invShift = 8 - shift;

        // Initial mask range, in bits, relative to the base address
        int start = shift;
        int end = start + width * tBitsPerPixel;

        while (end > 0) {
            // One source byte at a time, splatted to either one or two
            // destination bytes in VRAM.

            uint8_t source = *data;
            data++;

            // First byte
            unsigned mask = (0xFF << shift) & bitRange<uint16_t>(start, end);
            unsigned byte = _SYS_vbuf_peekb(&sys.vbuf, addr);
            byte &= ~mask;
            byte |= (source << shift) & mask;
            _SYS_vbuf_pokeb(&sys.vbuf, addr, byte);

            addr++;
            start -= 8;
            end -= 8;

            // Optional second byte
            if (shift) {
                unsigned mask = (0xFF >> invShift) & bitRange<uint16_t>(start, end);
                unsigned byte = _SYS_vbuf_peekb(&sys.vbuf, addr);
                byte &= ~mask;
                byte |= (source >> invShift) & mask;
                _SYS_vbuf_pokeb(&sys.vbuf, addr, byte);
            }
        }
    }

    /**
     * Draw a packed-pixel bitmap, of the same color depth as
     * this framebuffer mode. The destination rectangle
     * is specified as a top-left corner and size, both in
     * pixels.
     *
     * The bitmap does not need any special alignment.
     * The source bitmap stride is specified in bytes.
     *
     * All coordinates must be in range. This function performs no clipping.
     */
    void bitmap(const uint8_t *data, unsigned stride, UInt2 topLeft, UInt2 size)
    {
        while (size.y) {
            bitmapSpan(data, topLeft, size.x);
            size.y--;
            topLeft.y++;
            data += stride;
        }
    }

    /**
     * Draw a pre-formatted bitmap to this framebuffer. The bitmap
     * must already be in the proper format, and it must be 16-bit-aligned.
     */
    void set(const uint16_t *data) {
        _SYS_vbuf_write(&sys.vbuf, 0, data, sizeInWords());
    }
};


// Specific typedefs for each framebuffer mode
typedef FBDrawable<32,32,4> FB32Drawable;
typedef FBDrawable<64,64,1> FB64Drawable;
typedef FBDrawable<128,48,1> FB128Drawable;


};  // namespace Sifteo

#endif
