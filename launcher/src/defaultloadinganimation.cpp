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
        vid.fb32.set(LoadingBitmap);
    }
}

void DefaultLoadingAnimation::paint(CubeSet cubes, int percent)
{
    /*
     * Palette animation!
     */

    // Color scheme
    const RGB565 bgColor = RGB565::fromRGB(0xffffff);
    const RGB565 fgColor = RGB565::fromRGB(0x21b6ff);

    // Palette layout
    const unsigned bgIndex = 0;
    const unsigned firstAnimIndex = 1;
    const unsigned lastAnimIndex = 14;

    // Convert the completion percentage to a palette index,
    // including and after which the image is fully filled-in.
    unsigned completionIndex = 1 + lastAnimIndex - (1 + lastAnimIndex - firstAnimIndex) * percent / 100;

    const int shimmerPeriod = 32;
    int shimmerIndex = SystemTime::now().cycleFrame(TimeDelta::hz(1.0), shimmerPeriod);

    for (CubeID cube : cubes) {
        auto& cmap = Shared::video[cube].colormap;

        cmap[bgIndex] = bgColor;

        for (int i = firstAnimIndex; i <= lastAnimIndex; i++) {

            if (i >= completionIndex) {
                cmap[i] = fgColor;
                continue;
            }

            if (i > shimmerIndex) {
                cmap[i] = bgColor;
                continue;
            }

            cmap[i] = fgColor.lerp(bgColor, clamp((shimmerIndex - i) * 50, 0, 255));
        }
    }
}
