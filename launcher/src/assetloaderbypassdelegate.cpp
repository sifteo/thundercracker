/*
 * Thundercracker Launcher -- Confidential, not for redistribution.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#include "assetloaderbypassdelegate.h"
#include <sifteo.h>
using namespace Sifteo;
 

void AssetLoaderBypassDelegate::begin()
{
    SCRIPT(LUA, System():setAssetLoaderBypass(not os.getenv("BOOTSTRAP_SLOWLY")));
}

void AssetLoaderBypassDelegate::end()
{
    SCRIPT(LUA, System():setAssetLoaderBypass(false));
}

void AssetLoaderBypassDelegate::paint(int percent)
{
    System::paint();
}
