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
    samples.init(mod);
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

bool AudioChannelSlot::mixAudio(int *buffer, uint32_t numFrames)
{
    /*
     * Add this channel's contribution to 'buffer' for
     * 'numFrames' audio frames. If the buffer is NULL,
     * update state without outputting any audio.
     */

    // Early out if this channel is in the process of being stopped by the main thread.
    if (state & STATE_STOPPED) {
        #ifdef SIFTEO_SIMULATOR
            MCAudioVisData::clearChannel(AudioMixer::instance.channelID(this));
        #endif
        return false;
    }

    ASSERT(numFrames > 0);

    // Read from slot only once
    const int latchedVolume = volume;
    const int latchedIncrement = increment;
    const unsigned loopEnd = mod.loopEnd;           // Before first sample
    const unsigned loopStart = mod.loopStart;       // After last sample

    // Local copy of offset, to avoid writing back to RAM every time
    uint64_t localOffset = offset;

    do {
        unsigned index = localOffset >> SAMPLE_FRAC_SIZE;
        unsigned fractional = localOffset & SAMPLE_FRAC_MASK;

        // Looping logic
        if (UNLIKELY(index >= loopEnd)) {
            if (state & STATE_LOOP) {
                localOffset -= (loopEnd - loopStart) << SAMPLE_FRAC_SIZE;
                index = localOffset >> SAMPLE_FRAC_SIZE;
            } else {
                #ifdef SIFTEO_SIMULATOR
                    MCAudioVisData::clearChannel(AudioMixer::instance.channelID(this));
                #endif

                stop();
                break;
            }
        }

        // Compute the next sample
        if (buffer) {
            int sample;

            if (!fractional) {
                // Offset is aligned with an asset sample
                sample = samples.getSample(index, mod);

            } else {
                // At all other times, we use linear interpolation between two samples.

                int next;

                if (LIKELY(index + 1 < loopEnd)) {
                    // Fast path for linear interpolation between two adjacent samples
                    next = samples.getSamplePair(index, mod, sample);

                } else if (state & STATE_LOOP) {
                    // Next sample is on the other side of the loop
                    sample = samples.getSample(index, mod);
                    next = samples.getSample(loopStart, mod);

                } else {
                    // Not looping. Next sample is an implied zero.
                    sample = samples.getSample(index, mod);
                    next = 0;
                }

                sample += ((next - sample) * int(fractional)) >> SAMPLE_FRAC_SIZE;
            }

            // Mix volume
            sample = (sample * latchedVolume) >> _SYS_AUDIO_MAX_VOLUME_LOG2;

            #ifdef SIFTEO_SIMULATOR
                MCAudioVisData::writeChannelSample(AudioMixer::instance.channelID(this), sample);
            #endif

            // Mix into buffer (No need to clamp yet)
            *buffer += sample;
            buffer++;
        }

        // Advance to the next output sample
        localOffset += latchedIncrement;

    } while (--numFrames);

    offset = localOffset;
    samples.releaseRef();

    return true;
}

void AudioChannelSlot::setPos(uint32_t ofs)
{
    // Seeking past the end of the loop?
    if (ofs >= mod.loopEnd) {
        if (state & STATE_LOOP) {
            ofs = mod.loopStart + ((ofs - mod.loopStart) % (mod.loopEnd - mod.loopStart));
        } else {
            ofs = mod.loopEnd;
        }
    }

    offset = ofs << SAMPLE_FRAC_SIZE;
}
