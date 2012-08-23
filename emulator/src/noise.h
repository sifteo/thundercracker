/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * Sifteo Thundercracker simulator
 * M. Elizabeth Scott <beth@sifteo.com>
 *
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

/*
 * Noise generation functions, especially for Perlin noise.
 *
 * A good reference on Perlin noise basics:
 *    http://freespace.virgin.net/hugo.elias/models/m_perlin.htm
 */

#ifndef _NOISE_H
#define _NOISE_H

namespace Noise {


/*
 * Integer noise function, a.k.a. a deterministic random number generator.
 * This should produce uncorrelated-looking float noise for any given 32-bit
 * integer input. Outputs are between -1 and +1.
 */
inline double integer1D(int32_t x)
{
    // This is a pretty simple algorithm based on a polynomial with prime coefficients...
    // but it seems nobody knows where it comes from?
    // http://forums.tigsource.com/index.php?topic=21257.0;wap2

    x ^= x >> 13;
    int y = (x * (x * x * 60493 + 19990303) + 1376312589) & 0x7fffffff;
    
    // Scale from 31-bit int to signed double.
    return 1.0 - (double(y) / 1073741824.0);
}

/*
 * Integer noise function in 2 dimensions
 */
inline double integer2D(int32_t x, int32_t y)
{
    // Scale one axis by a large prime
    return integer1D(x ^ (y * 12553));
}

/*
 * Linearly interpolated 2D noise
 */
inline double linear2D(double x, double y)
{
    // Lower integer corner of this grid square
    double fx = floor(x);
    double fy = floor(y);
    int32_t ix = fx;
    int32_t iy = fy;

    // Distance from corner
    double ax = x - fx;
    double ay = y - fy;
    double axI = 1.0 - ax;
    double ayI = 1.0 - ay;  

    // Calculate all corner values
    double c00 = integer2D(ix, iy);
    double c01 = integer2D(ix, iy+1);
    double c10 = integer2D(ix+1, iy);
    double c11 = integer2D(ix+1, iy+1);

    // Interpolate along X edges of square
    double x0 = c10 * ax + c00 * axI;
    double x1 = c11 * ax + c01 * axI;

    // Interpolate Y axis
    return x1 * ay + x0 * ayI;
}

/*
 * Perlin fractal noise 
 */
inline double perlin2D(double x, double y, unsigned octaves)
{
    double r = 0.0;
    double amplitude = 1.0;

    while (octaves--) {
        r += amplitude * linear2D(x, y);
        amplitude *= 0.5;
        x *= 2.0;
        y *= 2.0;
    }

    return r;
}


/*
 * Test harness for perlin noise. Generates a PGM image file on stdout.
 */
inline void perlinTest()
{
    printf("P2 512 512 255\n");
    for (unsigned x = 0; x < 512; ++x) {
        for (unsigned y = 0; y < 512; ++y) {
            printf("%d ", int(128 + 64*Noise::perlin2D(x/64.0, y/64.0, 8)));
        }
        printf("\n");
    }
}


} // end namespace Noise

#endif
