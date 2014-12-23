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


/**
 * Delegate for receiving progress updates during a loading operation.
 *
 * ProgressDelegate subclasses can draw an animation to the screen
 * while the caller orchestrates one or more loading operations.
 */

class ProgressDelegate
{
public:

    /**
     * Called before the loading begins. The delegate may use this to
     * paint an initial frame to the screen. Guaranteed to be called
     * once per loading operation.
     */
    virtual void begin(Sifteo::CubeSet cubes) {}

    /**
     * Called after the loading finishes. Also guaranteed to be called
     * once per loading operation.
     */
    virtual void end(Sifteo::CubeSet cubes) {}

    /**
     * Update one or more cubes. Call this while idle, even if the percent
     * completeness hasn't changed, so the screen may animate on its own
     * if it needs to.
     *
     * Does not call System::paint(). The caller must do that once per frame.
     * It is legal to call paint() any number of times, possibly with different
     * cubes, as long as it's called at most once per frame for any particular
     * cube.
     */
    virtual void paint(Sifteo::CubeSet cubes, int percent) {}
};
