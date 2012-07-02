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
    virtual void begin() {}

    /**
     * Called after the loading finishes. Also guaranteed to be called
     * once per loading operation.
     */
    virtual void end() {}

    /**
     * Update the screen. Call this while idle, even if the percent
     * completeness hasn't changed, so the screen may animate on its own
     * if it needs to.
     */
    virtual void paint(int percent) = 0;
};
