/*
 * Thundercracker Launcher -- Confidential, not for redistribution.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#include "shared.h"
#include <sifteo.h>
using namespace Sifteo;


VideoBuffer Shared::video[CUBE_ALLOCATION];
TiltShakeRecognizer Shared::motion[CUBE_ALLOCATION];
SystemTime Shared::connectTime[CUBE_ALLOCATION];

Random Shared::random;

AssetSlot Shared::primarySlot = AssetSlot::allocate();
AssetSlot Shared::iconSlot = AssetSlot::allocate();
