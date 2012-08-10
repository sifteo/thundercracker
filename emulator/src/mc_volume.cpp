/*
 * Thundercracker Firmware -- Confidential, not for redistribution.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#include <sifteo/abi/audio.h>
#include "volume.h"
#include "macros.h"
#include "system.h"
#include "system_mc.h"

namespace Volume {

static const int kDefault = MAX_VOLUME;

static int currentVolume = 0;
static int unmuteVolume = 0;

void init()
{
    currentVolume = SystemMC::getSystem()->opt_mute ? 0 : kDefault;
    unmuteVolume = kDefault;
}

int systemVolume()
{
    ASSERT(currentVolume >= 0 && currentVolume <= MAX_VOLUME);
    return currentVolume;
}

void setSystemVolume(int v)
{
	unmuteVolume = currentVolume = clamp(v, 0, MAX_VOLUME);
}

void toggleMute()
{
	if (currentVolume) {
		unmuteVolume = currentVolume;
		currentVolume = 0;
	} else if (unmuteVolume) {
		currentVolume = unmuteVolume;
	} else {
		currentVolume = kDefault;
	}
}

} // namespace Volume
