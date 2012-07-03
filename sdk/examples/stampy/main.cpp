/*
 * Sifteo SDK Example.
 */

#include <sifteo.h>
#include "assets.gen.h"
using namespace Sifteo;

static Metadata M = Metadata()
    .title("Stampy SDK Example")
    .package("com.sifteo.sdk.stampy", "1.0")
    .icon(Icon)
    .cubeRange(1);

// 28x27 pixels, 16 color
static const uint16_t MrPink_pixels[] = {
    0x8000, 0x0008, 0x8000, 0x8888, 0x0088, 0x0000, 0x0000,
    0x2800, 0x0008, 0x8000, 0x8888, 0x0088, 0x0000, 0x0000,
    0x2280, 0x0008, 0x4000, 0x4444, 0x0044, 0x0000, 0x0000,
    0x2280, 0x0008, 0x8888, 0x8888, 0x8888, 0x0000, 0x0000,
    0x2280, 0x0008, 0x2800, 0x2222, 0x0886, 0x0000, 0x0000,
    0x2228, 0x0008, 0x2280, 0x2222, 0x8862, 0x0000, 0x0000,
    0x2228, 0x0088, 0x8288, 0x8222, 0x8662, 0x0008, 0x0000,
    0x2228, 0x0882, 0x2228, 0x2222, 0x6622, 0x0088, 0x0000,
    0x2228, 0x8822, 0x8228, 0x8888, 0x6222, 0x0886, 0x0000,
    0x2228, 0x2222, 0x8222, 0x8888, 0x2222, 0x8866, 0x0000,
    0x2228, 0x2222, 0x2222, 0x2888, 0x2222, 0x8662, 0x0008,
    0x2280, 0x2222, 0x2222, 0x2222, 0x2222, 0x6622, 0x0088,
    0x2280, 0x2222, 0x2222, 0x2222, 0x2822, 0x6222, 0x0086,
    0x6680, 0x2222, 0x2222, 0x2222, 0x8822, 0x6222, 0x0086,
    0x6800, 0x2266, 0x2222, 0x2222, 0x8222, 0x6222, 0x0086,
    0x8000, 0x2668, 0x2222, 0x2222, 0x8822, 0x6622, 0x0088,
    0x0000, 0x2880, 0x2222, 0x2222, 0x2822, 0x8662, 0x0008,
    0x0000, 0x2800, 0x2222, 0x2222, 0x2822, 0x8862, 0x0000,
    0x0000, 0x2800, 0x2222, 0x2222, 0x8822, 0x0888, 0x0000,
    0x0000, 0x2800, 0x2222, 0x2222, 0x2222, 0x0866, 0x0000,
    0x0000, 0x2280, 0x2222, 0x2222, 0x2222, 0x0866, 0x0000,
    0x0000, 0x2280, 0x8662, 0x8888, 0x2222, 0x0866, 0x0000,
    0x0000, 0x2280, 0x0862, 0x8000, 0x2288, 0x8866, 0x0000,
    0x8000, 0x2228, 0x0866, 0x0000, 0x2280, 0x8662, 0x0008,
    0x8800, 0x6222, 0x0086, 0x0000, 0x2880, 0x6622, 0x0088,
    0x2800, 0x6622, 0x0086, 0x0000, 0x2800, 0x6222, 0x0086,
    0x8800, 0x8888, 0x0088, 0x0000, 0x8800, 0x8888, 0x0888,
};

// 16-color palette (Even entries used first)
static const RGB565 MrPink_palette[] = { 
    {0xffff}, {0x0000}, {0xf5fb}, {0x0000},
    {0x3dff}, {0x0000}, {0xfad3}, {0x0000},
    {0x0000}, {0x0000}, {0x0000}, {0x0000},
    {0x0000}, {0x0000}, {0x0000}, {0x0000},
};

// Shuffled stipple pattern
static const UByte2 Stipple[64] = {
    {3,2}, {4,7}, {1,0}, {6,2}, {2,7}, {0,6}, {2,3}, {2,1},
    {0,1}, {7,6}, {7,1}, {7,3}, {0,4}, {6,0}, {5,4}, {5,2},
    {1,4}, {3,1}, {7,7}, {0,7}, {7,5}, {1,7}, {6,6}, {3,0},
    {7,2}, {4,5}, {6,3}, {4,1}, {3,3}, {5,0}, {1,2}, {2,4},
    {2,5}, {0,5}, {0,0}, {6,7}, {0,3}, {5,3}, {6,4}, {7,4},
    {1,3}, {5,7}, {4,2}, {3,6}, {0,2}, {2,6}, {6,1}, {5,5},
    {3,7}, {2,2}, {5,6}, {1,5}, {6,5}, {4,0}, {4,3}, {7,0},
    {4,4}, {5,1}, {4,6}, {1,1}, {2,0}, {1,6}, {3,5}, {3,4},
};

void main()
{
    const CubeID cube(0);
    static VideoBuffer vid;
    unsigned stippleIndex = 0;
    TimeStep ts;

    Random r;
    Float2 position = LCD_center;
    Float2 velocity = polar(r.uniform(0, M_PI*2), 80.f);

    vid.initMode(STAMP);
    vid.colormap.set(MrPink_palette);
    vid.attach(cube);

    while (1) {
        /*
         * Update our character position, taking input from the accelerometer
         * and bouncing off the sides of the screen.
         */

        ts.next();

        const Float2 minPosition = { 0, 0 };
        const Float2 maxPosition = { LCD_width - 28, LCD_height - 27 };
        const float deadzone = 2.0f;
        const float accelScale = 0.5f;
        const float damping = 0.99f;

        Float2 accel = cube.accel().xy();
        if (accel.len2() > deadzone * deadzone)
            velocity += accel * accelScale;
        velocity *= damping;

        Float2 candidate = position + velocity * float(ts.delta());

        if (candidate.x < minPosition.x || candidate.x > maxPosition.x)
            velocity.x = -velocity.x;

        if (candidate.y < minPosition.y || candidate.y > maxPosition.y)
            velocity.y = -velocity.y;

        position += velocity * float(ts.delta());

        /*
         * Stamp the character on top of the display's exising contents.
         *
         * Use paintUnlimited() here to avoid the usual frame rate limiting,
         * since we want the per-frame delay to occur after this draw,
         * not before it.
         */

        auto &charFB = vid.stamp.initFB<28,27>();
        charFB.set(MrPink_pixels);
        vid.stamp.setBox(position.round(), charFB.size());
        vid.stamp.setKeyIndex(0);

        System::paintUnlimited();

        /*
         * Make use of a rotating stipple pattern to give the appearance
         * of a fading trail behind our hero. This uses an 8x8 stipple
         * pattern, where on each frame we pick a different pixel to draw.
         * We have a shuffled sequence of coordinates, which tells us the
         * order in which to clear each of these 64 pixels.
         */ 

        auto &stippleFB = vid.stamp.initFB<8,8>();
        vid.stamp.setKeyIndex(1);
        vid.stamp.setBox(vec(0,0), LCD_size);
        stippleFB.fill(1);

        // Multiple stipple pixels per paint
        for (unsigned p = 0; p < 2; ++p)
            stippleFB.plot(Stipple[++stippleIndex % arraysize(Stipple)], 0);

        System::paint();
    }
}
