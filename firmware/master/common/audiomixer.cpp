/*
 * Thundercracker Firmware -- Confidential, not for redistribution.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#include "audiomixer.h"
#include "flash_blockcache.h"
#include <stdio.h>
#include <string.h>
#include <limits.h>
#include "xmtrackerplayer.h"

AudioMixer AudioMixer::instance;

AudioMixer::AudioMixer() :
    playingChannelMask(0),
    mixerVolume(_SYS_AUDIO_MAX_VOLUME),
    trackerCallbackInterval(0),
    trackerCallbackCountdown(0)
{
}

/*
 * Mix audio from flash into the audio device's buffer via each of the channels.
 *
 * This currently assumes that it's being run on the main thread, such that it
 * can operate synchronously with data arriving from flash.
 */
int AudioMixer::mixAudio(int16_t *buffer, uint32_t numsamples)
{
    if (!active())
        return 0;

    memset(buffer, 0, numsamples * sizeof(*buffer));

    int samplesMixed = 0;
    uint32_t mask = playingChannelMask;
    while (mask) {
        unsigned idx = Intrinsic::CLZ(mask);
        mask &= ~Intrinsic::LZ(idx);

        if (idx >= _SYS_AUDIO_MAX_CHANNELS) {
            ASSERT(idx < _SYS_AUDIO_MAX_CHANNELS);
            continue;
        }

        AudioChannelSlot &ch = channelSlots[idx];

        if (ch.isStopped()) {
            Atomic::ClearLZ(playingChannelMask, idx);
            continue;
        }

        if (ch.isPaused()) {
            continue;
        }
        
        // Each channel individually mixes itself with the existing buffer contents
        int mixed = ch.mixAudio(buffer, numsamples);

        // Update size of overall mixed audio buffer
        if (mixed > samplesMixed) {
            samplesMixed = mixed;
        }
    }

    // Apply master volume control.
    for (unsigned i = 0; i < samplesMixed; i++) {
        buffer[i] = buffer[i] * mixerVolume / _SYS_AUDIO_MAX_VOLUME;
    }

    return samplesMixed;
}

/*
    Called from within Tasks::work to mix audio on the main thread, to be
    consumed by the audio out device.
*/
void AudioMixer::pullAudio(void *p) {
    if (p == NULL) return;
    AudioBuffer *buf = static_cast<AudioBuffer*>(p);
    if (buf->writeAvailable() < sizeof(int16_t)) return;

    unsigned bytesToMix;
    int16_t *audiobuf = (int16_t*)buf->reserve(buf->writeAvailable(), &bytesToMix);
    bytesToMix -= bytesToMix % sizeof(int16_t);
    unsigned totalBytesMixed = 0;

    unsigned mixed;
    uint32_t numSamples;

    uint32_t &trackerInterval = AudioMixer::instance.trackerCallbackInterval;
    uint32_t &trackerCountdown = AudioMixer::instance.trackerCallbackCountdown;

    do {
        numSamples = (bytesToMix - totalBytesMixed) / sizeof(int16_t);
        numSamples = trackerInterval > 0 && trackerCountdown < numSamples
                   ? trackerCountdown : numSamples;
        if (!numSamples) {
            ASSERT(numSamples);
            return;
        }

        mixed = AudioMixer::instance.mixAudio(audiobuf, numSamples);
        audiobuf += mixed;
        totalBytesMixed += mixed * sizeof(int16_t);

        if (trackerInterval) {
            if (mixed == 0) {
                mixed = numSamples;
                totalBytesMixed += mixed * sizeof(int16_t);
            }

            /* Tracker countdown should never be 0 when an interval is set--
             * when it reaches 0 the callback is fired and the counter
             * immediately reset.
             */
            if (!trackerCountdown) {
                ASSERT(trackerCountdown);
                trackerCountdown = mixed;
            }

            // The mixer should never mix beyond the limit of the countdown.
            if (mixed > trackerCountdown) {
                ASSERT(mixed <= trackerCountdown);
                trackerCountdown = mixed;
            }

            // Update the callback countdown
            trackerCountdown -= mixed;

            // Fire the callback
            if (trackerCountdown == 0) {
                XmTrackerPlayer::mixerCallback();
                trackerCountdown = trackerInterval;
            }
        }
    } while(mixed == numSamples && bytesToMix > totalBytesMixed);

    // Check for buffer overrun.
    ASSERT(totalBytesMixed <= bytesToMix);

    if (totalBytesMixed > 0) {
        buf->commit(totalBytesMixed);
    }
}

void AudioMixer::setMixerVolume(uint16_t volume)
{
    ASSERT(volume < _SYS_AUDIO_MAX_VOLUME);
    mixerVolume = clamp((int)volume, 0, _SYS_AUDIO_MAX_VOLUME);
}

bool AudioMixer::play(const struct _SYSAudioModule *mod,
    _SYSAudioChannelID ch, _SYSAudioLoopType loopMode)
{
    // NB: "mod" is a temporary contiguous copy of _SYSAudioModule in RAM.

    // Invalid channel?
    if (ch >= _SYS_AUDIO_MAX_CHANNELS) {
        ASSERT(ch < _SYS_AUDIO_MAX_CHANNELS);
        return false;
    }

    // Already playing?
    if (isPlaying(ch))
        return false;

    AudioChannelSlot &slot = channelSlots[ch];
    slot.play(mod, loopMode);
    Atomic::SetLZ(playingChannelMask, ch);

    return true;
}

bool AudioMixer::isPlaying(_SYSAudioChannelID ch)
{
    // Invalid channel?
    if (ch >= _SYS_AUDIO_MAX_CHANNELS) {
        ASSERT(ch < _SYS_AUDIO_MAX_CHANNELS);
        return false;
    }

    return (playingChannelMask & Intrinsic::LZ(ch)) != 0;
}

void AudioMixer::stop(_SYSAudioChannelID ch)
{
    // Invalid channel?
    if (ch >= _SYS_AUDIO_MAX_CHANNELS) {
        ASSERT(ch < _SYS_AUDIO_MAX_CHANNELS);
        return;
    }


    channelSlots[ch].stop();
    Atomic::ClearLZ(playingChannelMask, ch);
}

void AudioMixer::pause(_SYSAudioChannelID ch)
{
    // Invalid channel?
    if (ch >= _SYS_AUDIO_MAX_CHANNELS) {
        ASSERT(ch < _SYS_AUDIO_MAX_CHANNELS);
        return;
    }

    if (isPlaying(ch)) {
        channelSlots[ch].pause();
    }
}

void AudioMixer::resume(_SYSAudioChannelID ch)
{
    // Invalid channel?
    if (ch >= _SYS_AUDIO_MAX_CHANNELS) {
        ASSERT(ch < _SYS_AUDIO_MAX_CHANNELS);
        return;
    }

    if (isPlaying(ch)) {
        channelSlots[ch].resume();
    }
}

void AudioMixer::setVolume(_SYSAudioChannelID ch, uint16_t volume)
{
    // Invalid channel?
    if (ch >= _SYS_AUDIO_MAX_CHANNELS) {
        ASSERT(ch < _SYS_AUDIO_MAX_CHANNELS);
        return;
    }

    // XXX: should call setVolume
    channelSlots[ch].volume = clamp((int)volume, 0, (int)_SYS_AUDIO_MAX_VOLUME);
}

int AudioMixer::volume(_SYSAudioChannelID ch)
{
    // Invalid channel?
    if (ch >= _SYS_AUDIO_MAX_CHANNELS) {
        ASSERT(ch < _SYS_AUDIO_MAX_CHANNELS);
        return -1;
    }

    return channelSlots[ch].volume;
}

void AudioMixer::setSpeed(_SYSAudioChannelID ch, uint32_t samplerate)
{
    // Invalid channel?
    if (ch >= _SYS_AUDIO_MAX_CHANNELS) {
        ASSERT(ch < _SYS_AUDIO_MAX_CHANNELS);
        return;
    }

    channelSlots[ch].setSpeed(samplerate);
}

void AudioMixer::setPos(_SYSAudioChannelID ch, uint32_t ofs)
{
    // Invalid channel?
    if (ch >= _SYS_AUDIO_MAX_CHANNELS) {
        ASSERT(ch < _SYS_AUDIO_MAX_CHANNELS);
        return;
    }

    channelSlots[ch].setPos(ofs);
}

uint32_t AudioMixer::pos(_SYSAudioChannelID ch)
{
    // Invalid channel?
    if (ch >= _SYS_AUDIO_MAX_CHANNELS) {
        ASSERT(ch < _SYS_AUDIO_MAX_CHANNELS);
        return (uint32_t)-1;
    }

    // TODO - implement
    return 0;
}

void AudioMixer::setTrackerCallbackInterval(uint32_t usec)
{
    static const uint64_t kMicroSecondsPerSecond = 1000000;

    // Convert to frames
    trackerCallbackInterval = ((uint64_t)usec * (uint64_t)curSampleRate) / kMicroSecondsPerSecond;

    // Catch underflow. No one should ever need callbacks this often. Ever.
    ASSERT(usec == 0 || trackerCallbackInterval > 0);
    // But if we're not DEBUG, we may as well let it happen every sample. Gross.
    if (usec > 0 && trackerCallbackInterval == 0) {
        trackerCallbackInterval = 1;
    }

    trackerCallbackCountdown = trackerCallbackInterval;
}
