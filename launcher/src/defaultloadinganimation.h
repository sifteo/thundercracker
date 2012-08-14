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
    // When did the animation start?
    Sifteo::SystemTime startTime;

    // How many dots have already been plotted, on each cube?
    uint8_t dotCounts[CUBE_ALLOCATION];

    // The pattern of remaining dot positions, on each cube
    static const unsigned numDots = 50;
    Sifteo::BitArray<numDots> dotPositions[CUBE_ALLOCATION];

    void drawNextDot(Sifteo::CubeID cube);
    void drawDotAtPosition(Sifteo::CubeID cube, unsigned position);
};
