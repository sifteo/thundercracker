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
 * This is a POD type to represent a 16-bit 5:6:5 color,
 * the native format used by our display. RGB565 colors can be converted
 * to and from 8-bit RGB, and this class can perform simple color blending.
 */
struct RGB565 {
    uint16_t value;

    /**
     * Create an RGB565 color from a 5-6-5-bit RGB color,
     * with each component specified separately.
     */
    static RGB565 from565(uint8_t r, uint8_t g, uint8_t b) {
        RGB565 result = { (r << 11) | (g << 5) | b };
        return result;
    }

    /**
     * Create an RGB565 color from an 8-bit RGB color. We accurately
     * round to the nearest representable 16-bit color.
     */
    static RGB565 fromRGB(uint8_t r, uint8_t g, uint8_t b) {
        /*
         * Round to the nearest 5/6 bit color. Note that simple
         * bit truncation does NOT produce the best result!
         */
        return from565( ((unsigned)r * 31 + 128) / 255,
                        ((unsigned)g * 63 + 128) / 255,
                        ((unsigned)b * 31 + 128) / 255 );
    }

    /**
     * Create an RGB565 color from a floating point RGB color.
     * We accurately round to the nearest representable 16-bit color.
     */
    static RGB565 fromRGB(float r, float g, float b) {
        return from565( r * 31.f + 0.5f,
                        g * 63.f + 0.5f,
                        b * 31.f + 0.5f );
    }

    /**
     * Create an RGB565 color from a packed-pixel 24-bit RGB color.
     * Blue is in the least significant byte, making this convenient
     * for specifying colors in the common hexadecimal format 0xRRGGBB.
     */
    static RGB565 fromRGB(uint32_t rgb) {
        return fromRGB((uint8_t)(rgb >> 16), (uint8_t)(rgb >> 8), (uint8_t)rgb);
    }

    /**
     * Return the color's red component, as a 5-bit value.
     */
    uint8_t red5() const {
        return (value >> 11) & 0x1F;
    }

    /**
     * Return the color's green component, as a 6-bit value
     */
    uint8_t green6() const {
        return (value >> 5) & 0x3F;
    }

    /**
     * Return the color's blue component, as a 5-bit value
     */
    uint8_t blue5() const {
        return value & 0x1F;
    }

    /**
     * Return the color's red component, extended to 8 bits.
     */
    uint8_t red() const {
        /*
         * A good approximation is (r5 << 3) | (r5 >> 2), but this
         * is still not quite as accurate as the implementation here.
         */
        return red5() * 255 / 31;
    }

    /**
     * Return the color's green component, extended to 8 bits.
     */
    uint8_t green() const {
        return green6() * 255 / 63;
    }

    /**
     * Return the color's blue component, extended to 8 bits.
     */
    uint8_t blue() const {
        return blue5() * 255 / 31;
    }

    /**
     * Return the color as a packed 24-bit RGB value.
     * Blue is in the least significant byte. The resulting
     * color, in hexadecimal, has the form 0xRRGGBB.
     */
    uint32_t packedRGB() const {
        return blue() | (green() << 8) | (red() << 16);
    }
    
    /**
     * Linear interpolation between this color and another color.
     * When alpha=255, this returns the "other" color. When alpha=0,
     * we return this color. Values in-between will result in a
     * correspondingly blended color.
     */
    RGB565 lerp(RGB565 other, uint8_t alpha) const {
        unsigned A = alpha;
        unsigned invA = 0xff - A;
        return from565( (red5()   * invA + other.red5()   * A) / 0xff,
                        (green6() * invA + other.green6() * A) / 0xff,
                        (blue5()  * invA + other.blue5()  * A) / 0xff );
    }

    bool operator== (const RGB565 &other) const { return value == other.value; }
    bool operator!= (const RGB565 &other) const { return value != other.value; }
};


/**
 * A ColormapSlot refers to a single colormap index on a single cube.
 * Typically these are transient objects which are used to implement
 * assignment and retrieval of colormap entries using the array indexing
 * operator, but it is also possible to store ColormapSlot references.
 *
 * A ColormapSlot acts as an accessor for memory which is part of your
 * VideoBuffer. When you read or write colormap entries via this object,
 * you're reading or writing VRAM.
 */
struct ColormapSlot {
    _SYSAttachedVideoBuffer *sys;
    unsigned index;

    /**
     * Set the color in this colormap slot to the specified RGB565 value.
     */
    void set(RGB565 color) const {
        _SYS_vbuf_poke(&sys->vbuf, _SYS_VA_COLORMAP / 2 + index, color.value);
    }

    /**
     * Set the color in this slot to the specified 8-bit RGB value.
     */
    void set(uint8_t r, uint8_t g, uint8_t b) const {
        set(RGB565::fromRGB(r, g, b));
    }

    /**
     * Set the color in this slot to the specified floating point RGB value.
     */
    void set(float r, float g, float b) const {
        set(RGB565::fromRGB(r, g, b));
    }

    /**
     * Set the color in this slot to the specified packed RGB color.
     * This can be a convenient way to specify colors as hexadecimal
     * constants, of the form 0xRRGGBB.
     */
    void set(uint32_t rgb) const {
        set(RGB565::fromRGB(rgb));
    }

    /**
     * Set the color in this slot to a specifc RGB565 value.
     */
    const ColormapSlot& operator= (RGB565 color) const {
        set(color);
        return *this;
    }

    /**
     * Set the color in this slot to the specified packed RGB color.
     * This can be a convenient way to specify colors as hexadecimal
     * constants, of the form 0xRRGGBB.
     */
    const ColormapSlot& operator= (uint32_t color) const {
        set(color);
        return *this;
    }

    /**
     * Retrieve this color, as an RGB565 value.
     */
    RGB565 get() const {
        RGB565 result = { _SYS_vbuf_peek(&sys->vbuf, _SYS_VA_COLORMAP / 2 + index) };
        return result;
    }
};


/**
 * This is an accessor for the colormap, with up to 16 colors.
 * This colormap is used by the SOLID_MODE, FB32, FB64, and FB128 modes.
 *
 * Colors are stored in 16-bit RGB565 format.
 *
 * Note that, due to details of the radio compression protocol, even-numbered
 * colormap indices will compress better in the FB32 mode. This means it
 * can give better performance if you allocate the most common color values
 * to even-numbered slots.
 */

struct Colormap {
    _SYSAttachedVideoBuffer sys;

    static const unsigned NUM_COLORS = 16;

    /**
     * Return a ColormapSlot which references a single colormap entry
     * on a single VideoBuffer. You don't need to store this reference
     * typically; for example:
     *
     *   vbuf.colormap[16].set(1.f, 0.f, 0.f);
     */
    ColormapSlot operator[](unsigned index) {
        ASSERT(index < NUM_COLORS);
        ColormapSlot result = { &sys, index };
        return result;
    }

    /**
     * Clear the palette to black.
     */
    void erase() {
        _SYS_vbuf_fill(&sys.vbuf, _SYS_VA_COLORMAP / 2, 0, NUM_COLORS);
    }

    /**
     * Splat the given color across all colormap slots.
     */
    void fill(RGB565 color) {
        _SYS_vbuf_fill(&sys.vbuf, _SYS_VA_COLORMAP / 2, color.value, NUM_COLORS);
    }

    /**
     * Set a range of colormap values, given an array of RGB565 values.
     */
    void setRange(const RGB565 *colors, unsigned firstColor, unsigned numColors) {
        ASSERT(firstColor <= NUM_COLORS && numColors <= NUM_COLORS
            && (firstColor + numColors) <= NUM_COLORS);
        _SYS_vbuf_write(&sys.vbuf, _SYS_VA_COLORMAP / 2 + firstColor,
            &colors[0].value, numColors);
    }

    /**
     * Set the entire palette, given an array of exactly 16 RGB565 values.
     */
    void set(const RGB565 *colors) {
        setRange(colors, 0, NUM_COLORS);
    }

    /**
     * Set the entire palette to the traditional EGA 16-color palette.
     * http://en.wikipedia.org/wiki/Enhanced_Graphics_Adapter#Color_palette
     */
    void setEGA() {
        static const RGB565 colors[] = {
            /*  0 */ { 0x0000 },
            /*  1 */ { 0x0015 },
            /*  2 */ { 0x0540 },
            /*  3 */ { 0x0555 },
            /*  4 */ { 0xa800 },
            /*  5 */ { 0xa815 },
            /*  6 */ { 0xaaa0 },
            /*  7 */ { 0xad55 },
            /*  8 */ { 0x52aa },
            /*  9 */ { 0x52bf },
            /* 10 */ { 0x57ea },
            /* 11 */ { 0x57ff },
            /* 12 */ { 0xfaaa },
            /* 13 */ { 0xfabf },
            /* 14 */ { 0xffea },
            /* 15 */ { 0xffff },
        };
        set(colors);
    }

    /**
     * Set a monochrome palette, in entries [0] and [1]. This is common
     * in the two-color modes, FB64 and FB128.
     */
    void setMono(RGB565 color0, RGB565 color1) {
        (*this)[0].set(color0);
        (*this)[1].set(color1);
    }

    /**
     * Set a monochrome palette, in entries [0] and [1]. This is common
     * in the two-color modes, FB64 and FB128. Colors are specified as
     * packed RGB values, in the format 0xRRGGBB.
     */
    void setMono(uint32_t color0, uint32_t color1) {
        (*this)[0].set(color0);
        (*this)[1].set(color1);
    }
};


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
