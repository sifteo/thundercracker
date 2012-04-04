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

};


};  // namespace Sifteo

#endif
