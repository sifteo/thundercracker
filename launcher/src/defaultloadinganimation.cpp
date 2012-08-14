/*
 * Thundercracker Launcher -- Confidential, not for redistribution.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#include "defaultloadinganimation.h"
#include "shared.h"
#include <sifteo.h>
using namespace Sifteo;

#include "resources/loading-bitmap.h"

// Parameters
namespace {

    // Color scheme
    const Sifteo::RGB565 bgColor  = Sifteo::RGB565::fromRGB(0x000000);  // Black screen
    const Sifteo::RGB565 dotColor = Sifteo::RGB565::fromRGB(0xff9100);  // Sifteo logo orange
    const Sifteo::RGB565 gradient[5] = {
        { 0x25bf },
        { 0x1c98 },
        { 0x1372 },
        { 0x0a4c },
        { 0x0126 },
    };

    // Palette layout
    const unsigned bgIndex = 0;
    const unsigned dotIndex = 15;
    const unsigned firstAnimIndex = 1;
    const unsigned lastAnimIndex = 14;

    // Animation parameters
    const int shimmerPeriod = 64;       // Breathing time at the end
    const float frameRate = 30.0;
};

DefaultLoadingAnimation::DefaultLoadingAnimation()
    : startTime(SystemTime::now())
{}

void DefaultLoadingAnimation::begin(CubeSet cubes)
{
    for (CubeID cube : cubes) {
        VideoBuffer &vid = Shared::video[cube];
        vid.initMode(FB32);
        vid.attach(cube);

        // Initial image
        vid.fb32.set(LoadingBitmap);

        // Constant palette entries
        vid.colormap[bgIndex] = bgColor;
        vid.colormap[dotIndex] = dotColor;

        // All dot positions available
        dotPositions[cube].mark();
        dotCounts[cube] = 0;
    }
}

void DefaultLoadingAnimation::paint(CubeSet cubes, int percent)
{
    int shimmerIndex = (SystemTime::now() - startTime).frames(TimeDelta::hz(frameRate)) % shimmerPeriod;
    int nextDotCount = MIN(numDots, (percent * numDots + 50) / 100);

    for (CubeID cube : cubes) {
        /*
         * Palette animation!
         */

        auto& cmap = Shared::video[cube].colormap;
        for (int i = firstAnimIndex; i <= lastAnimIndex; i++) {
            int x = shimmerIndex - i;
            cmap[i] = (x >= 0 && x < arraysize(gradient)) ? gradient[x] : bgColor;
        }

        /*
         * Plot new dots, sufficient to cover the progress made since last time.
         * Each cube can have a different pseudorandom pattern of dots.
         */

        while (dotCounts[cube] < nextDotCount) {
            drawNextDot(cube);
            dotCounts[cube]++;
        }

        /*
         * It's also possible for loading progress to go backwards. In that case,
         * we may need to remove dots.
         */

        while (dotCounts[cube] > nextDotCount) {
            removeDot(cube);
            dotCounts[cube]--;
        }
    }
}

void DefaultLoadingAnimation::drawNextDot(Sifteo::CubeID cube)
{
    /*
     * Pick the next random unused dot from 'dotPositions' for this cube, and draw it.
     */

    ASSERT(dotCounts[cube] <= numDots);
    ASSERT(!dotPositions[cube].empty());

    // Random choice from the remaining dots
    unsigned position;
    unsigned choice = Shared::random.randrange(numDots - dotCounts[cube]);
    if (dotPositions[cube].clearN(position, choice))
        drawDotAtPosition(cube, position, dotIndex);
}

void DefaultLoadingAnimation::removeDot(Sifteo::CubeID cube)
{
    /*
     * Backtrack: Remove an arbitrary dot, putting it back into
     * 'dotPositions'. This is not common, but we may need
     * to backtrack, for example if a new cube arrives or there's
     * an error on one cube.
     *
     * For simplicity, instead of doing a proper random choice
     * here we'll just start repopulating from the beginning.
     */

    ASSERT(dotCounts[cube] > 0);

    Sifteo::BitArray<numDots> usedDots = ~dotPositions[cube];

    unsigned position;
    unsigned choice = Shared::random.randrange(dotCounts[cube]);
    if (usedDots.clearN(position, choice)) {
        // Clear the dot
        dotPositions[cube].mark(position);
        drawDotAtPosition(cube, position, 0);
    }
}

void DefaultLoadingAnimation::drawDotAtPosition(Sifteo::CubeID cube, unsigned position, unsigned color)
{
    /*
     * Our image has room for 50 2x2-pixel dots currently, arranged in two 5x5 grids
     * in the two otherwise-empty corners, with a 1-pixel margin between dots.
     */

    STATIC_ASSERT(numDots == 50);
    ASSERT(position < numDots);

    UInt2 v = { 1, 1 };

    if (position >= 25) {
        // Top-right box
        v.x += 16;
        position -= 25;
    } else {
        // Bottom-left box
        v.y += 16;
    }

    v.x += 3 * (position % 5);
    v.y += 3 * (position / 5);

    Shared::video[cube].fb32.fill(v, vec(2,2), color);
}
