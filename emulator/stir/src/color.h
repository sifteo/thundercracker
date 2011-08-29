/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * STIR -- Sifteo Tiled Image Reducer
 * M. Elizabeth Scott <beth@sifteo.com>
 *
 * Copyright <c> 2011 Sifteo, Inc. All rights reserved.
 */

#ifndef _COLOR_H
#define _COLOR_H

#include <stdint.h>
#include <math.h>


struct CIELab {
    // Based on the CIE L*a*b code used by dcraw. See: http://www.rawness.es/cielab/?lang=en
    //
    // To the extent possible under law, Manuel Llorens <manuelllorens@gmail.com>
    // has waived all copyright and related or neighboring rights to this work.
    // This code is licensed under CC0 v1.0, see license information at
    // http://creativecommons.org/publicdomain/zero/1.0/

    double L, a, b;
    
    CIELab(double _L, double _a, double _b) {
	L = _L;
	a = _a;
	b = _b;
    }
  
    CIELab(uint32_t rgb) {
	double r = 65535.0 * (uint8_t)rgb / 255.0;
	double g = 65535.0 * (uint8_t)(rgb >> 8) / 255.0;
	double b = 65535.0 * (uint8_t)(rgb >> 16) / 255.0;

	double xyz[3] = { 0, 0, 0 };
	static const double m[3][3] = {
	    {0.79767484649999998, 0.13519170820000001, 0.031353408800000003},
	    {0.28804020250000001, 0.71187413249999998, 8.5665099999999994e-05},
	    {0, 0, 0.82521143890000004}
	};
	for (int i = 0; i < 3; i++) {
	    xyz[i] += m[i][0] * r;
	    xyz[i] += m[i][1] * g;
	    xyz[i] += m[i][2] * b;
	}

	static const float d50_white[3] = {0.964220, 1, 0.825211}; 
	for (int i = 0; i < 3; i++) {
	    xyz[i] = f_cbrt(xyz[i] / d50_white[i]);
	}

	L = 116.0 * xyz[1] - 16.0;
	a = 500.0 * (xyz[0] - xyz[1]);
	b = 200.0 * (xyz[1] - xyz[2]);
    }

    uint32_t rgb() const {
        // Convert L*a*b* to XYZ  
	double fy = (L+16.0)/116.0;
        double fx = fy + a/500.0;
        double fz = fy - b/200.0;
  
	static const double ep = 216.0 / 24389.0;
	static const double ka = 24389.0 / 27.0;
	static const float d50_white[3]={0.964220,1,0.825211}; 

	double xyz[3];
        xyz[0] = 255.0 * d50_white[0] * (fx*fx*fx>ep?fx*fx*fx:(116.0*fx-16.0)/ka);  
        xyz[1] = 255.0 * d50_white[1] * (L>ka*ep?pow(fy,3.0):L/ka);  
        xyz[2] = 255.0 * d50_white[2] * (fz*fz*fz>ep?fz*fz*fz:(116.0*fz-16.0)/ka);  
  
        // Convert XYZ to RGB  
	double rgb[3] = { 0, 0, 0 };
	static const double m[3][3] = {
	    {1.3459434662015495, -0.54459884249024337, -6.9388939039072284e-18},
	    {-0.25560754075639425, 1.5081672430343562, 3.9573379295720912e-18},
	    {-0.05111177218797338, 0.020535140503506362, 1.2118106376869808}
	};
	for (int i = 0; i < 3; i++) {
            rgb[0] += xyz[i] * m[i][0];  
            rgb[1] += xyz[i] * m[i][1];  
            rgb[2] += xyz[i] * m[i][2];  
        }  

	uint32_t rgb32 = 0;
	for (int i = 0; i < 3; i++) {
	    // Round, not truncate
	    double v = rgb[i] + 0.5;

	    if (v < 0) v = 0;
	    if (v > 255) v = 255;

	    rgb32 |= (uint8_t)v << (i*8);
        }  

	return rgb32;
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

private:
    double f_cbrt(double r){  
	static const double ep = 216.0 / 24389.0;
	static const double ka = 24389.0 / 27.0;
	r/=65535.0;  
	return r > ep ? pow(r,1/3.0) : (ka*r+16)/116.0;  
    }  
};


struct RGB565 {
    uint16_t value;

    RGB565(uint16_t _value) {
	value = _value;
    }

    RGB565(uint32_t rgb) {
	RGB565 v((uint8_t)rgb, (uint8_t)(rgb >> 8), (uint8_t)(rgb >> 16));
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

    bool operator==(RGB565 &other) const {
	return value == other.value;
    }
};

#endif
