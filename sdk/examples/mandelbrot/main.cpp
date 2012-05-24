/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * Sifteo SDK Example.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#include <sifteo.h>
using namespace Sifteo;

static Metadata M = Metadata()
    .title("Mandelbrot SDK Example")
    .cubeRange(1);

RGB565 calculateMandelbrot(UInt2 pixel);
RGB565 calculateColorPalette(float shade);


void main()
{
    const CubeID cube(0);
    static VideoBuffer vid;

    /*
     * We use STAMP mode in a special way here, to do (slow) true-color
     * rendering: The framebuffer is simply set up as an identity mapping
     * that shows each of the 16 colors in our colormap. Now we can put
     * a row of 16 pixels directly into the colormap, and render the screen
     * using 1024 of these little 16x1 pixel "frames".
     *
     * Clearly this is really slow, and this technique is unlikely to be
     * frequently useful, but it's a fun parlour trick :)
     */

    vid.initMode(STAMP);
    vid.stamp.disableKey();
    vid.attach(cube);

    auto &fb = vid.stamp.initFB<16,1>();
    for (unsigned i = 0; i < 16; i++)
        fb.plot(vec(i, 0U), i);

    for (unsigned y = 0; y < LCD_height; y++)
        for (unsigned x = 0; x < LCD_width; x += 16) {

            /*
             * Render 16 pixels at a time, into a buffer in RAM.
             */

            static RGB565 pixels[16];

            for (unsigned i = 0; i < 16; i++)
                pixels[i] = calculateMandelbrot(vec(x+i, y));

            /*
             * Now copy to VRAM and start painting. By waiting until
             * now to call finish(), we're allowing the calculation above
             * to run concurrently with the cube's paint operation.
             *
             * Note that our "frames" are actually just tiny pieces of the
             * screen, so we need to avoid the default frame rate limits
             * in order to render at an at all reasonable rate. This is
             * where paintUnlimited() comes into play.
             */

            System::finish();
            vid.stamp.setBox(vec(x,y), vec(16,1));
            vid.colormap.set(pixels);
            System::paintUnlimited();
        }

    // Kill time (efficiently)
    while (1)
        System::paint();
}

RGB565 calculateMandelbrot(UInt2 pixel)
{
    /*
     * Calculate one pixel of a Mandelbrot fractal,
     * using continuous shading based on a renormalizing technique.
     *
     * Reference: http://linas.org/art-gallery/escape/escape.html
     */

    const float radius2 = 400.f;
    const unsigned maxIters = 12;
    const float scale = 0.022f;
    const Int2 center = LCD_center + vec(25, 0);

    // We use Float2 vectors to store complex numbers
    Float2 z = vec(0, 0);
    Float2 c = (pixel - center) * scale;
    unsigned iters = 0;
    float modulus2;

    // Traditional Mandelbrot iteration
    do {
        z = z.cmul(z) + c;
        modulus2 = z.len2();
    } while (++iters <= maxIters && modulus2 <= radius2);

    // A couple extra iterations
    z = z.cmul(z) + c;
    z = z.cmul(z) + c;
    iters += 2;

    // Continuous shading, taking into account the iteration number as well
    // as how far outside the escape radius we are.
    float mu = (iters - log(log(z.len())) / M_LN2) / maxIters;

    if (mu < 1.0f) {
        // Exterior of fractal
        return calculateColorPalette(mu);
    } else {
        // Interior (didn't escape within our limit, or NaN)
        return RGB565::fromRGB(0x000000);
    }
}

RGB565 calculateColorPalette(float shade)
{
    // Specific colors that we'll interpolate between
    static const RGB565 stops[] = {
        RGB565::fromRGB(0x000000),
        RGB565::fromRGB(0x3216b0),
        RGB565::fromRGB(0xffa400),
        RGB565::fromRGB(0xfff800),
        RGB565::fromRGB(0xffffff),
    };

    // Convert normalized shade in [0,1] to a fractional stop index
    shade *= arraysize(stops) - 1;

    // What's the first color in the pair we're interpolating?
    int index = clamp<int>(shade, 0, arraysize(stops) - 2);

    // How far along are we from the first color to the second?
    int alpha = clamp<int>((shade - index) * 256.f, 0, 255);

    return stops[index].lerp(stops[index + 1], alpha);
}
