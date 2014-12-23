/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * Sifteo Thundercracker simulator
 * Micah Elizabeth Scott <micah@misc.name>
 *
 * Copyright <c> 2012 Sifteo, Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include <sifteo/abi/audio.h>
#include "volume.h"
#include "macros.h"
#include "system.h"
#include "system_mc.h"

namespace Volume {

/*
 * Default volume should be unity gain, both to keep the unit tests
 * happy and because this is really a sensible default for game development.
 */
static const int kDefault = MAX_VOLUME >> MIXER_GAIN_LOG2;

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
