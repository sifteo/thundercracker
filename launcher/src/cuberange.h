/*
 * Thundercracker Launcher -- Confidential, not for redistribution.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#pragma once
#include <sifteo.h>


/**
 * A CubeRange represents the minimum and maximum number of cubes
 * for a particular task.
 */
 
class CubeRange
{
public:
    _SYSMetadataCubeRange sys;

    CubeRange(const _SYSMetadataCubeRange *cr = 0);

    bool isValid();

    /**
     * Set up the system to use exactly the minimum number of cubes.
     * This is most meaningful in simulation, where we can actaully
     * change the number of cubes.
     *
     * Returns a CubeSet representing the cubes we are using.
     */
    Sifteo::CubeSet initMinimum();
};
