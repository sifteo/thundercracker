/*
 * Thundercracker Launcher -- Confidential, not for redistribution.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#include "defaultloadinganimation.h"
#include "shared.h"
#include <sifteo.h>
using namespace Sifteo;

#include "resources/loading-bitmap.h"


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
    }

    startTime = SystemTime::now();
    bzero(dotCounts);
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
        for (int i = firstAnimIndex; i <= lastAnimIndex; i++)
            cmap[i] = i > shimmerIndex ? bgColor : fgColor.lerp(bgColor, clamp((shimmerIndex - i) * 50, 0, 255));

        /*
         * Plot new dots, sufficient to cover the progress made since last time.
         * Each cube can have a different pseudorandom pattern of dots.
         */

        while (dotCounts[cube] < nextDotCount) {
            drawNextDot(cube);
            dotCounts[cube]++;
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
    unsigned i = Shared::random.randrange(numDots - dotCounts[cube]);

    // Find the i'th remaining dot on this cube
    for (unsigned position : dotPositions[cube]) {
        if (i--)
            continue;

        dotPositions[cube].clear(position);
        drawDotAtPosition(cube, position);
        break;
    }
}

void DefaultLoadingAnimation::drawDotAtPosition(Sifteo::CubeID cube, unsigned position)
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

    Shared::video[cube].fb32.fill(v, vec(2,2), dotIndex);
}
