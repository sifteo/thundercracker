/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * STIR -- Sifteo Tiled Image Reducer
 * M. Elizabeth Scott <beth@sifteo.com>
 *
 * Copyright <c> 2011 Sifteo, Inc. All rights reserved.
 */

#ifndef _COLOR_H
#define _COLOR_H

#include <vector>
#include <functional>
#include <list>
#include <stdint.h>

#include "logger.h"


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

    // Make a slight (1 LSB) modification to the low byte
    RGB565 wiggle() const;

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


/*
 * CIELab --
 *
 *    CIE L*a*b* colorspace data type and conversion routines.  Based
 *    on the CIE L*a*b* code used by dcraw. See:
 *
 *       http://www.rawness.es/cielab/?lang=en
 *
 *    The following waiver applied to the original unmodified code:
 *
 *    To the extent possible under law, Manuel Llorens <manuelllorens@gmail.com>
 *    has waived all copyright and related or neighboring rights to this work.
 *    This code is licensed under CC0 v1.0, see license information at
 *    http://creativecommons.org/publicdomain/zero/1.0/
 *
 *    This routine has been verified for accurate round-trip
 *    conversion of 8-bit RGB colors.
 */

struct CIELab {

    union {
	struct {
	    double L, a, b;
	};
	double axis[3];
    };

    static const double gamma = 2.2;

    CIELab() {
	L = a = b = 0;
    }
    
    CIELab(double _L, double _a, double _b) {
	L = _L;
	a = _a;
	b = _b;
    }
  
    CIELab(uint32_t rgb);
    uint32_t rgb() const;
    double meanSquaredError(CIELab other);

    CIELab(RGB565 rgb) {
	*this = lut565[rgb.value];
    }
    
    CIELab& operator+= (const CIELab &other) {
	L += other.L;
	a += other.a;
	b += other.b;
	return *this;
    }
    
    CIELab& operator/= (const double &other) {
	L /= other;
	a /= other;
	b /= other;
	return *this;
    }

    static void initialize(void);

    // Algorithms in CIE L*a*b* color space
    static int findMajorAxis(RGB565 *colors, size_t count);

    struct sortAxis : public std::binary_function<RGB565 &, RGB565 &, bool> {
        sortAxis(int _axis) : axis(_axis) {}
	int axis;
	bool operator()(const RGB565 &a, const RGB565 &b) {
	    return CIELab(a).axis[axis] < CIELab(b).axis[axis];
	}
    };
    
private:
    static double f_cbrt(double r);
    static double decodeGamma(uint8_t v);
    static uint8_t encodeGamma(double v);

    static CIELab lut565[0x10000];
};


/*
 * ColorReducer --
 *
 *    Maintains a pool of color values, reduces those values using a
 *    particular maximum particular number of colors, and performs
 *    lookups in this color pool to find the nearest color to any
 *    given value.
 *
 *    Every occurrence of every pixel should be add()'ed to the
 *    ColorReducer, so that its prevalence in the input data can be
 *    properly assessed.
 *
 *    This uses the median cut algorithm, operating internally using
 *    CIE L*a*b* color space. To determine the amount of color
 *    reduction possible, this uses a Mean Squared Error analysis,
 *    weighted according to the color difference and the number of
 *    affected pixels.
 */

class ColorReducer {
 public:
    ColorReducer();

    void reduce(double maxMSE, Logger &log);

    void add(RGB565 color) {
	colors.push_back(color);
	colorWeights[color.value]++;
    }

    RGB565 nearest(RGB565 color) {
	return boxMedian(boxes[inverseLUT[color.value]]);
    }

    unsigned numColors() {
	return boxes.size();
    }

 private:
    // One median-cut box. Holds a (cached) average color for the box,
    // as well as indices for the box's boundaries within the 'colors' vector.
    struct box {
	unsigned begin, end;
    };

    std::vector<RGB565> colors;
    std::vector<box> boxes;
    std::list<unsigned> boxQueue;
    uint16_t inverseLUT[0x10000];
    uint32_t colorWeights[0x10000];

    void updateInverseLUT();
    double meanSquaredError();
    bool splitBox(box &b);
    void splitBox(box &b, int at);

    RGB565 boxMedian(box &b) {
	return colors[(b.begin + b.end) >> 1];
    }
};
    

#endif
