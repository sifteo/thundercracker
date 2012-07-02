/*
 * Thundercracker Launcher -- Confidential, not for redistribution.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#pragma once
#include <sifteo.h>
#include "progressdelegate.h"


/**
 * A stub ProgressDelegate which uses Asset Loader Bypass mode
 * to load assets immediately.
 */

class AssetLoaderBypassDelegate : public ProgressDelegate
{
public:
    virtual void begin();
    virtual void end();
    virtual void paint(int percent);
};
