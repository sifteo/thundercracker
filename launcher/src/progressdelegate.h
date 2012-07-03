/*
 * Thundercracker Launcher -- Confidential, not for redistribution.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
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
