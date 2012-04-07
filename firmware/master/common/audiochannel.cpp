/*
 * Thundercracker Firmware -- Confidential, not for redistribution.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#include "macros.h"
#include "svmmemory.h"
#include "audiochannel.h"
#include <limits.h>


void AudioChannelSlot::init()
{
    state = STATE_STOPPED;
    volume = _SYS_AUDIO_DEFAULT_VOLUME;
    sampleNum = 0;
}

void AudioChannelSlot::play(const struct _SYSAudioModule *module, _SYSAudioLoopType loopMode)
{
    mod = *module;
    samples.init(&mod);
    state = (loopMode == LoopOnce ? 0 : STATE_LOOP);
    sampleNum = 0;
}

uint32_t AudioChannelSlot::mixAudio(int16_t *buffer, uint32_t len)
{
    // Early out if this channel is in the process of being stopped by the main thread.
    if (state & STATE_STOPPED)
        return 0;

    uint32_t framesLeft = len;

    while(framesLeft > 0) {
        // Mix a sample, after volume adjustment, with the existing buffer contents
        int32_t sample = *buffer + ((samples[sampleNum++] * (int32_t)volume) / _SYS_AUDIO_MAX_VOLUME);
                
        // TODO - more subtle compression instead of hard limiter
        *buffer = clamp(sample, (int32_t)SHRT_MIN, (int32_t)SHRT_MAX);
        buffer++;
        framesLeft--;

        // EOF?
        if (sampleNum >= samples.numSamples()) {
            // loop!?
            if (state & STATE_LOOP)
                sampleNum = 0;
            else
                break;
        }
    }

    samples.releaseRef();

    return len - framesLeft;
}
