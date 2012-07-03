/*
 * Thundercracker Launcher -- Confidential, not for redistribution.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#include "assetloaderbypassdelegate.h"
#include <sifteo.h>
using namespace Sifteo;
 

void AssetLoaderBypassDelegate::begin(CubeSet cubes)
{
    SCRIPT(LUA, System():setAssetLoaderBypass(not os.getenv("BOOTSTRAP_SLOWLY")));
}

void AssetLoaderBypassDelegate::end(CubeSet cubes)
{
    SCRIPT(LUA, System():setAssetLoaderBypass(false));
}
