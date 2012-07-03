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
    virtual void begin(Sifteo::CubeSet cubes);
    virtual void paint(Sifteo::CubeSet cubes, int percent);

private:
    Sifteo::SystemTime startTime;
};
