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

    bool isEmpty();
    bool isValid();
    void set();
};
