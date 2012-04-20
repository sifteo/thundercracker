/*
 * Thundercracker Firmware -- Confidential, not for redistribution.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#include "audiomixer.h"
#include "flashlayer.h"
#include <stdio.h>
#include <string.h>
#include <limits.h>
#include "xmtrackerplayer.h"

AudioMixer AudioMixer::instance;

AudioMixer::AudioMixer() :
    playingChannelMask(0),
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
        ASSERT(idx < _SYS_AUDIO_MAX_CHANNELS);
        AudioChannelSlot &ch = channelSlots[idx];
        mask &= ~Intrinsic::LZ(idx);

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
        ASSERT(numSamples > 0);

        mixed = AudioMixer::instance.mixAudio(audiobuf, numSamples);
        audiobuf += mixed;
        totalBytesMixed += mixed * sizeof(int16_t);

        if (trackerInterval) {
            ASSERT(trackerCountdown != 0);
            ASSERT(mixed <= trackerCountdown);
            // Update the callback countdown
            trackerCountdown -= mixed;

            // Fire the callback
            if (trackerCountdown == 0) {
                XmTrackerPlayer::mixerCallback();
                trackerCountdown = trackerInterval;
            }
        }
    } while(mixed == numSamples && bytesToMix > totalBytesMixed);

    // mixed as returned by mixAudio measure samples, but we care about bytes
    ASSERT(totalBytesMixed <= bytesToMix);
    if (totalBytesMixed > 0) {
        buf->commit(totalBytesMixed);
    }
}

bool AudioMixer::play(const struct _SYSAudioModule *mod,
    _SYSAudioChannelID ch, _SYSAudioLoopType loopMode)
{
    // NB: "mod" is a temporary contiguous copy of SYSAudioModule in RAM.
    ASSERT(ch < _SYS_AUDIO_MAX_CHANNELS);

    // already playing? no no
    if (isPlaying(ch))
        return false;

    AudioChannelSlot &slot = channelSlots[ch];
    slot.play(mod, loopMode);
    Atomic::SetLZ(playingChannelMask, ch);

    return true;
}

bool AudioMixer::isPlaying(_SYSAudioChannelID ch)
{
    return (playingChannelMask & Intrinsic::LZ(ch)) != 0;
}

void AudioMixer::stop(_SYSAudioChannelID ch)
{
    ASSERT(ch < _SYS_AUDIO_MAX_CHANNELS);

    channelSlots[ch].stop();
    Atomic::ClearLZ(playingChannelMask, ch);
}

void AudioMixer::pause(_SYSAudioChannelID ch)
{
    ASSERT(ch < _SYS_AUDIO_MAX_CHANNELS);

    if (isPlaying(ch)) {
        channelSlots[ch].pause();
    }
}

void AudioMixer::resume(_SYSAudioChannelID ch)
{
    ASSERT(ch < _SYS_AUDIO_MAX_CHANNELS);

    if (isPlaying(ch)) {
        channelSlots[ch].resume();
    }
}

void AudioMixer::setVolume(_SYSAudioChannelID ch, uint16_t volume)
{
    ASSERT(ch < _SYS_AUDIO_MAX_CHANNELS);

    channelSlots[ch].volume = clamp((int)volume, 0, (int)_SYS_AUDIO_MAX_VOLUME);
}

int AudioMixer::volume(_SYSAudioChannelID ch)
{
    ASSERT(ch < _SYS_AUDIO_MAX_CHANNELS);

    return channelSlots[ch].volume;
}

uint32_t AudioMixer::pos(_SYSAudioChannelID ch)
{
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
    // But if we're not DEBUG, we may as well let it happen every sample.
    if (usec > 0 && trackerCallbackInterval == 0) {
        trackerCallbackInterval = 1;
    }

    trackerCallbackCountdown = trackerCallbackInterval;
}
