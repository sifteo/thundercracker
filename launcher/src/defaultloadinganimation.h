/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * Thundercracker launcher
 *
 * Copyright <c> 2012 Sifteo, Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
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
    void removeDot(Sifteo::CubeID cube);

    void drawDotAtPosition(Sifteo::CubeID cube, unsigned position, unsigned color);
};
