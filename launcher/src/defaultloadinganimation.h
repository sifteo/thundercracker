/*
 * Thundercracker Launcher -- Confidential, not for redistribution.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#pragma once
#include <sifteo.h>
#include "progressdelegate.h"


/**
 * A simple loading animation, usable at any time.
 */

class DefaultLoadingAnimation : public ProgressDelegate
{
public:
    DefaultLoadingAnimation();

    virtual void begin(Sifteo::CubeSet cubes);
    virtual void paint(Sifteo::CubeSet cubes, int percent);

private:
    // Color scheme
    const Sifteo::RGB565 bgColor  = Sifteo::RGB565::fromRGB(0x000000);  // Black screen
    const Sifteo::RGB565 fgColor  = Sifteo::RGB565::fromRGB(0x24b6ff);  // Sifteo logo blue
    const Sifteo::RGB565 dotColor = Sifteo::RGB565::fromRGB(0xff9100);  // Sifteo logo orange

    // Palette layout
    static const unsigned bgIndex = 0;
    static const unsigned dotIndex = 15;
    static const unsigned firstAnimIndex = 1;
    static const unsigned lastAnimIndex = 14;

    // Animation parameters
    static const int shimmerPeriod = 32;
    static const float frameRate = 30.0;
    static const unsigned numDots = 50;

    // When did the animation start?
    Sifteo::SystemTime startTime;

    // How many dots have already been plotted, on each cube?
    uint8_t dotCounts[CUBE_ALLOCATION];

    // The pattern of remaining dot positions, on each cube
    Sifteo::BitArray<numDots> dotPositions[CUBE_ALLOCATION];

    void drawNextDot(Sifteo::CubeID cube);
    void drawDotAtPosition(Sifteo::CubeID cube, unsigned position);
};
