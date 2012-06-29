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

void AudioChannelSlot::setSpeed(uint32_t sampleRate)
{
    increment = (sampleRate << SAMPLE_FRAC_SIZE) / AudioMixer::SAMPLE_HZ;
}

void AudioChannelSlot::play(const struct _SYSAudioModule *module, _SYSAudioLoopType loopMode)
{
    mod = *module;
    samples.init(mod.loopStart);
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

    // Pre-calculate the loop endpoint (relatively expensive)
    offsetLimit = (state & STATE_LOOP) && mod.loopEnd
        ? ((uint64_t)mod.loopEnd) << SAMPLE_FRAC_SIZE
        : ((uint64_t)samples.numSamples(mod) - 1) << SAMPLE_FRAC_SIZE;

    state &= ~STATE_STOPPED;
}

bool AudioChannelSlot::mixAudio(int *buffer, uint32_t numFrames)
{
    // Early out if this channel is in the process of being stopped by the main thread.
    if ((state & STATE_STOPPED) || mod.dataSize == 0) {
        #ifdef SIFTEO_SIMULATOR
            MCAudioVisData::clearChannel(AudioMixer::instance.channelID(this));
        #endif
        return false;
    }

    ASSERT(samples.numSamples(mod) > 0);
    ASSERT(numFrames > 0);

    // Read from slot only once
    const int latchedVolume = volume;
    const int latchedIncrement = increment;
    const uint64_t latchedLimit = offsetLimit;

    // Local copy of offset, to avoid writing back to RAM every time
    uint64_t localOffset = offset;

    do {
        // Looping logic
        if (UNLIKELY(localOffset > latchedLimit)) {
            if (state & STATE_LOOP) {
                uint64_t fpLoopStart = ((uint64_t)mod.loopStart) << SAMPLE_FRAC_SIZE;
                localOffset = fpLoopStart + (localOffset - latchedLimit);
            } else {
                #ifdef SIFTEO_SIMULATOR
                    MCAudioVisData::clearChannel(AudioMixer::instance.channelID(this));
                #endif

                stop();
                break;
            }
        }

        // Compute the next sample
        int sample;
        unsigned index = localOffset >> SAMPLE_FRAC_SIZE;
        unsigned fractional = localOffset & SAMPLE_FRAC_MASK;

        if (!fractional) {
            // Offset is aligned with an asset sample
            sample = samples.getSample(index, mod);
        } else {
            // Linearly interpolate between the two relevant samples            
            int next = samples.getSamplePair(index, mod, sample);
            sample += ((next - sample) * int(fractional)) >> SAMPLE_FRAC_SIZE;
        }

        // Mix volume
        sample = (sample * latchedVolume) >> _SYS_AUDIO_MAX_VOLUME_LOG2;

        #ifdef SIFTEO_SIMULATOR
            MCAudioVisData::writeChannelSample(AudioMixer::instance.channelID(this), sample);
        #endif

        // Mix into buffer (No need to clamp yet)
        *buffer += sample;

        // Advance to the next output sample
        localOffset += latchedIncrement;
        buffer++;
    } while (--numFrames);

    offset = localOffset;
    samples.releaseRef();

    return true;
}

void AudioChannelSlot::setPos(uint32_t ofs)
{
    uint32_t numSamples = samples.numSamples(mod);
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
            /* Seeking out of bounds in a ping-pong sample is not well-defined;
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
