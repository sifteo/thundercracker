/*
 * Thundercracker Firmware -- Confidential, not for redistribution.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#include "volume.h"
#include <sifteo/abi/audio.h>
#include "macros.h"

static bool initialized = false;

namespace Volume {

void init()
{
    initialized = true;
}

uint16_t systemVolume()
{
    ASSERT(initialized);

    // TODO: hook this into something in the siftulator.
    return _SYS_AUDIO_MAX_VOLUME;
}

} // namespace Volume
