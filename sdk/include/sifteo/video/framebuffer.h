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
 * @brief A templatized VRAM accessor for drawing pixel graphics,
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
     * @brief Return the width, in pixels, of this mode
     */
    static unsigned width() {
        return tWidth;
    }

    /**
     * @brief Return the height, in pixels, of this mode
     */
    static unsigned height() {
        return tHeight;
    }

    /**
     * @brief Return the size of this mode as a vector, in pixels.
     */
    static UInt2 size() {
        return vec(tWidth, tHeight);
    }

    /**
     * @brief Return the total number of colors this mode supports. This is
     * equal to the number of colormap entries used by the mode.
     */
    static unsigned numColors() {
        return 1 << tBitsPerPixel;
    }

    /**
     * @brief Returns the number of bits per pixel this framebuffer mode uses
     * to store color indices.
     */
    static unsigned bitsPerPixel() {
        return tBitsPerPixel;
    }

    /**
     * @brief Returns the size of this framebuffer's data, in bytes
     */
    static unsigned sizeInBytes() {
        return tWidth * tHeight * tBitsPerPixel / 8;
    }

    /**
     * @brief Returns the size of this framebuffer's data, in 16-bit words
     */
    static unsigned sizeInWords() {
        return tWidth * tHeight * tBitsPerPixel / 16;
    }

    /**
     * @brief Plot a single pixel, at the specified location.
     *
     * Locations are specified in pixels, with (0,0) at the top-left
     * corner, +X to the right, and +Y down.
     *
     * The pixel coordinates must be in range. This call does not perform
     * any clipping.
     */
    void plot(UInt2 pos, unsigned colorIndex)
    {
        ASSERT(pos.x <= tWidth && pos.y <= tHeight);

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
     * @brief Given a single color index, return an expanded version where
     * the single color has been replicated to fill a 16-bit word.
     *
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
     * @brief Plot a horizontal span of pixels, given the position of the
     * leftmost pixel, and the number of pixels to plot.
     *
     * All coordinates must be in range. This function performs no clipping.
     */
    void span(UInt2 pos, unsigned width, unsigned colorIndex)
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
     * @brief Fill a rectangle of pixels, specified as a top-left corner
     * location and a size.
     *
     * All coordinates must be in range. This function performs no clipping.
     */
    void fill(UInt2 topLeft, UInt2 size, unsigned colorIndex)
    {
        while (size.y) {
            span(topLeft, size.x, colorIndex);
            size.y--;
            topLeft.y++;
        }
    }

    /**
     * @brief Fill the entire framebuffer with a specific color index.
     */
    void fill(unsigned colorIndex)
    {
        _SYS_vbuf_fill(&sys.vbuf, 0, expand16(colorIndex), sizeInWords());
    }

    /**
     * @brief Draw a span of pixels from a packed-pixel bitmap, of the
     * same color depth as this framebuffer mode.
     *
     * All coordinates must be in range. This function performs no clipping.
     */
    void bitmapSpan(UInt2 pos, unsigned width, const uint8_t *data)
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
     * @brief Draw a packed-pixel bitmap, of the same color depth as
     * this framebuffer mode.
     *
     * The destination rectangle is specified as a top-left corner
     * and size, both in pixels.
     *
     * The bitmap does not need any special alignment.
     * The source bitmap stride is specified in bytes.
     *
     * All coordinates must be in range. This function performs no clipping.
     */
    void bitmap(UInt2 topLeft, UInt2 size, const uint8_t *data, unsigned stride)
    {
        while (size.y) {
            bitmapSpan(topLeft, size.x, data);
            size.y--;
            topLeft.y++;
            data += stride;
        }
    }

    /**
     * @brief Draw a pre-formatted bitmap to this framebuffer
     *
     * The bitmap must already be in the proper format, and it must be 16-bit-aligned.
     */
    void set(const uint16_t *data) {
        _SYS_vbuf_write(&sys.vbuf, 0, data, sizeInWords());
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


/// Typedef for the FBDrawable instance used in FB32 mode
typedef FBDrawable<32,32,4> FB32Drawable;

/// Typedef for the FBDrawable instance used in FB64 mode
typedef FBDrawable<64,64,1> FB64Drawable;

/// Typedef for the FBDrawable instance used in FB128 mode
typedef FBDrawable<128,48,1> FB128Drawable;


/**
 * @brief A VRAM accessor for the STAMP mode, a special
 * purpose 16-color framebuffer mode which supports color-keying and
 * tiling.
 *
 * It is so named because it can be used like a rubber stamp,
 * to draw patterns or images over top of whatever already exists on the
 * display.
 */
struct StampDrawable {
    _SYSAttachedVideoBuffer sys;

    /**
     * @brief Change the geometry of the framebuffer memory used by this stamp.
     *
     * The framebuffer can be any size, so long as the width is an even
     * number of pixels (corresponding to an integer number of bytes),
     * the total number of pixels <= 1536, and each dimension is <= 128.
     */
    void resizeFB(Int2 pixelSize) {
        ASSERT((pixelSize.x & 1) == 0 &&
            pixelSize.x * pixelSize.y <= 1536 &&
            pixelSize.x <= 128 &&
            pixelSize.y <= 128);
    
        _SYS_vbuf_poke(&sys.vbuf, offsetof(_SYSVideoRAM, stamp_pitch)/2,
            (pixelSize.x >> 1) | (pixelSize.y << 8));
    }

    /**
     * @brief Obtain an FBDrawable accessor for the framebuffer memory, using
     * the supplied framebuffer dimensions.
     *
     * Does not automatically resize the framebuffer to those dimensions;
     * this is for cases when you know the framebuffer is already set up.
     * If you don't know this for sure, or you're accessing the framebuffer
     * for the first time without an explicit call to resize(), use initFB()
     * instead.
     */
    template <unsigned tWidth, unsigned tHeight>
    FBDrawable<tWidth, tHeight, 4>& getFB()
    {
        auto &fb = *reinterpret_cast<FBDrawable<tWidth, tHeight, 4>*>(this);
        ASSERT(&fb.sys == &sys);
        return fb;
    }

    /**
     * @brief Obtain an FBDrawable accessor for the framebuffer memory, using
     * the supplied framebuffer dimensions.
     *
     * Automatically resizes the
     * framebuffer to the specified dimensions, if necessary. Just like
     * VideoBuffer::initMode() we automatically issue a System::finish()
     * before resizing the framebuffer.
     */
    template <unsigned tWidth, unsigned tHeight>
    FBDrawable<tWidth, tHeight, 4>& initFB()
    {
        _SYS_finish();
        resizeFB(vec(tWidth, tHeight));
        return getFB<tWidth, tHeight>();
    }
    
    /**
     * @brief Set the horizontal window. This is the mode-specific X-axis
     * counterpart to VideoBuffer::setWindow().
     *
     * We start drawing at 'firstColumn', and draw a total of 'numColumns' pixels
     * per line. If numColumns is greater than the framebuffer width,
     * the framebuffer repeats horizontally.
     */
    void setHWindow(uint8_t firstColumn, uint8_t numColumns) {
        _SYS_vbuf_poke(&sys.vbuf, offsetof(_SYSVideoRAM, stamp_x)/2,
            firstColumn | (numColumns << 8));
    }

    /**
     * @brief Set both the horizontal and vertical windows, to define a 2D
     * box of pixels that we'll draw into.
     *
     * If either dimension of this
     * box is larger than our framebuffer, the framebuffer will be tiled.
     */
    void setBox(Int2 topLeft, Int2 size) {
        _SYS_vbuf_poke(&sys.vbuf, offsetof(_SYSVideoRAM, first_line)/2,
            topLeft.y | (size.y << 8));
        setHWindow(topLeft.x, size.x);
    }

    /**
     * @brief Set the palette index of the "key" color. 
     *
     * Any pixel with this palette index is skipped, leaving a
     * transparent "hole" at that pixel.
     */
    void setKeyIndex(unsigned index) {
        _SYS_vbuf_pokeb(&sys.vbuf, offsetof(_SYSVideoRAM, stamp_key), index);
    }

    /**
     * @brief Disable transparency
     *
     * This is equivalent to setting the key index to a color which is never used.
     */
    void disableKey() {
        setKeyIndex(16);
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
