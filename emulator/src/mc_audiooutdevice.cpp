/*
 * Thundercracker Firmware -- Confidential, not for redistribution.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#include "audiooutdevice.h"
#include "mc_portaudiooutdevice.h"
#include "macros.h"
#include "audiomixer.h"

/*
 * Host side implementation of AudioOutDevice - just forward calls to the
 * appropriate hw specific device.
 */

static PortAudioOutDevice gPortAudio;


void AudioOutDevice::init(AudioMixer *mixer)
{
    gPortAudio.init(mixer);
}

void AudioOutDevice::start()
{
    gPortAudio.start();
}

void AudioOutDevice::stop()
{
    gPortAudio.stop();
}

void AudioOutDevice::suspend()
{
    // Nothing to do yet
}

void AudioOutDevice::resume()
{
    // Nothing to do yet
}
