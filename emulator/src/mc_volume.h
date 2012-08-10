/*
 * Thundercracker Firmware -- Confidential, not for redistribution.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#ifndef _MC_VOLUME_H
#define _MC_VOLUME_H

/*
 * Emulator-only volume controls
 */

#include "volume.h"

namespace Volume
{
    void setSystemVolume(int v);
    void toggleMute();
}

#endif // _MC_VOLUME_H
