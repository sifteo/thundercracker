/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * Sifteo Thundercracker simulator
 * M. Elizabeth Scott <beth@sifteo.com>
 *
 * Copyright <c> 2011 Sifteo, Inc. All rights reserved.
 */

#ifndef _COLOR_H
#define _COLOR_H

/*
 * RGB565 --
 *
 *    16-bit 5:6:5 color representation and conversion routines. This
 *    has been verified for accurate round-trip conversion to and from
 *    8-bit RGB.
 */

struct RGB565 {
    uint16_t value;

    RGB565() {
        value = 0;
    }

    RGB565(uint16_t _value) {
        value = _value;
    }

    RGB565(uint32_t rgb) {
        RGB565 v((uint8_t)rgb, (uint8_t)(rgb >> 8), (uint8_t)(rgb >> 16));
        value = v.value;
    }

    RGB565(uint8_t *rgb) {
        RGB565 v(rgb[0], rgb[1], rgb[2]);
        value = v.value;
    }   
    
    RGB565(uint8_t r, uint8_t g, uint8_t b) {
        /*
         * Round to the nearest 5/6 bit color. Note that simple
         * bit truncation does NOT produce the best result!
         */
        uint16_t r5 = ((uint16_t)r * 31 + 128) / 255;
        uint16_t g6 = ((uint16_t)g * 63 + 128) / 255;
        uint16_t b5 = ((uint16_t)b * 31 + 128) / 255;
        value = (r5 << 11) | (g6 << 5) | b5;
    }

    uint8_t red() const {
        /*
         * A good approximation is (r5 << 3) | (r5 >> 2), but this
         * is still not quite as accurate as the implementation here.
         */
        uint16_t r5 = (value >> 11) & 0x1F;
        return r5 * 255 / 31;
    }

    uint8_t green() const {
        uint16_t g6 = (value >> 5) & 0x3F;
        return g6 * 255 / 63;
    }
    
    uint8_t blue() const {
        uint16_t b5 = value & 0x1F;
        return b5 * 255 / 31;
    }

    uint32_t rgb() const {
        return red() | (green() << 8) | (blue() << 16);
    }

    bool operator== (const RGB565 &other) const {
        return value == other.value;
    }

    bool operator!= (const RGB565 &other) const {
        return value != other.value;
    }

    bool operator< (const RGB565 &other) const {
        return value < other.value;
    }
};

#endif
