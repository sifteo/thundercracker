/*
 * Thundercracker Firmware -- Confidential, not for redistribution.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#include "macros.h"
#include "svmmemory.h"
#include "audiochannel.h"
#include <limits.h>
#include "audiomixer.h"

#ifdef SIFTEO_SIMULATOR
#   include "mc_audiovisdata.h"
#endif


void AudioChannelSlot::init()
{
    state = STATE_STOPPED;
}

void AudioChannelSlot::setSpeed(uint32_t sampleRate)
{
    increment = (sampleRate << SAMPLE_FRAC_SIZE) / AudioMixer::instance.sampleRate();
}

void AudioChannelSlot::play(const struct _SYSAudioModule *module, _SYSAudioLoopType loopMode)
{
    mod = *module;
    samples.init(&mod, mod.loopStart);
    offset = 0;

    // Let the module decide
    if (loopMode == _SYS_LOOP_UNDEF)
        loopMode = _SYSAudioLoopType(mod.loopType);

    if (loopMode == _SYS_LOOP_ONCE) {
        state &= ~STATE_LOOP;
    } else {
        state |= STATE_LOOP;
    }

    // Set up initial playback parameters
    setSpeed(mod.sampleRate);
    setVolume(mod.volume);

    state &= ~STATE_STOPPED;
}

bool AudioChannelSlot::mixAudio(int16_t *buffer, uint32_t numFrames)
{
    // Early out if this channel is in the process of being stopped by the main thread.
    if (state & STATE_STOPPED || samples.numSamples() == 0)
        return false;

    uint64_t fpLimit = (state & STATE_LOOP) && mod.loopEnd
                     ? ((uint64_t)mod.loopEnd) << SAMPLE_FRAC_SIZE
                     : ((uint64_t)samples.numSamples() - 1) << SAMPLE_FRAC_SIZE;

    while (numFrames--) {
        // Looping logic
        if (offset > fpLimit) {
            if (state & STATE_LOOP) {
                uint64_t fpLoopStart = ((uint64_t)mod.loopStart) << SAMPLE_FRAC_SIZE;
                offset = fpLoopStart + (offset - fpLimit);
            } else {
                stop();
                break;
            }
        }

        // Compute the next sample
        int32_t sample;
        if ((offset & SAMPLE_FRAC_MASK) == 0) {
            // Offset is aligned with an asset sample
            sample = samples[offset >> SAMPLE_FRAC_SIZE];
        } else {
            int32_t sample_next = samples[(offset >> SAMPLE_FRAC_SIZE) + 1];
            sample = samples[offset >> SAMPLE_FRAC_SIZE];
            // Linearly interpolate between the two relevant samples
            sample += ((sample_next - sample) * (offset & SAMPLE_FRAC_MASK)) >> SAMPLE_FRAC_SIZE;
        }

        // Mix volume
        sample = (sample * (int32_t)volume) / _SYS_AUDIO_MAX_VOLUME;

        #ifdef SIFTEO_SIMULATOR
            MCAudioVisData::writeChannelSample(AudioMixer::instance.channelID(this), sample);
        #endif

        // Mix into buffer
        sample += *buffer;

        // TODO - more subtle compression instead of hard limiter
        *buffer = clamp(sample, (int32_t)SHRT_MIN, (int32_t)SHRT_MAX);

        // Advance to the next output sample
        offset += increment;
        buffer++;
    }

    samples.releaseRef();

    return true;
}

void AudioChannelSlot::setPos(uint32_t ofs)
{
    uint32_t numSamples = samples.numSamples();
    uint32_t loopOffset = 0;

    if (mod.loopType == _SYS_LOOP_EMULATED_PING_PONG) {
        loopOffset = ((((mod.loopEnd + 1) - mod.loopStart) + 2) / 2) - 2;
        numSamples -= loopOffset;
    }

    if(ofs < numSamples) {
        offset = ofs + (ofs > mod.loopEnd ? loopOffset : 0);
    } else if (state & STATE_LOOP) {
        if (ofs == numSamples) {
            offset = mod.loopStart;
        } else {
            /* Seeking out of bonds in a ping-pong sample is not well-defined;
             * no tracker seems to implement loop directionality.
             * Compute the offset based on the original sample's length, not
             * the synthesized sample's length, and loop forward (same as
             * MilkyTracker).
             */
            uint32_t loopLength = mod.loopEnd + 1 - (mod.loopStart + loopOffset);
            // begin at loop start, modulo the length of the loop
            offset = (ofs - mod.loopStart) % loopLength + mod.loopStart;
        }
    } else {
        return;
    }
    offset <<= SAMPLE_FRAC_SIZE;
}
