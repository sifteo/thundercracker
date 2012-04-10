/*
 * Thundercracker Firmware -- Confidential, not for redistribution.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#include "audiomixer.h"
#include "flashlayer.h"
#include <stdio.h>
#include <string.h>
#include "xmtrackerplayer.h"

AudioMixer AudioMixer::instance;

AudioMixer::AudioMixer() :
    playingChannelMask(0),
    nextHandle(0),
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
    _SYSAudioHandle *handle, _SYSAudioLoopType loopMode)
{
    // NB: "mod" is a temporary contiguous copy of SYSAudioModule in RAM.
    
    // find channels that are enabled but not playing
    uint32_t selector = ~playingChannelMask;

    unsigned idx = Intrinsic::CLZ(selector);
    if (idx >= _SYS_AUDIO_MAX_CHANNELS) {
        return false;
    }
    AudioChannelSlot &ch = channelSlots[idx];
    ASSERT(nextHandle < UINT32_MAX);
    ch.handle = nextHandle++;
    *handle = ch.handle;

    ch.play(mod, loopMode);
    Atomic::SetLZ(playingChannelMask, idx);

    return true;
}

bool AudioMixer::isPlaying(_SYSAudioHandle handle)
{
    return channelForHandle(handle, playingChannelMask) != 0;
}

void AudioMixer::stop(_SYSAudioHandle handle)
{
    if (AudioChannelSlot *ch = channelForHandle(handle, playingChannelMask)) {
        ch->stop();
        Atomic::ClearLZ(playingChannelMask, ch - channelSlots);
    }
}

void AudioMixer::pause(_SYSAudioHandle handle)
{
    if (AudioChannelSlot *ch = channelForHandle(handle, playingChannelMask)) {
        ch->pause();
    }
}

void AudioMixer::resume(_SYSAudioHandle handle)
{
    if (AudioChannelSlot *ch = channelForHandle(handle, playingChannelMask)) {
        ch->resume();
    }
}

void AudioMixer::setVolume(_SYSAudioHandle handle, uint16_t volume)
{
    if (AudioChannelSlot *ch = channelForHandle(handle)) {
        ch->volume = clamp((int)volume, 0, (int)_SYS_AUDIO_MAX_VOLUME);
    }
}

int AudioMixer::volume(_SYSAudioHandle handle)
{
    if (AudioChannelSlot *ch = channelForHandle(handle)) {
        return ch->volume;
    }
    return 0;
}

uint32_t AudioMixer::pos(_SYSAudioHandle handle)
{
    if (AudioChannelSlot *ch = channelForHandle(handle, playingChannelMask)) {
        ch = 0;
        // TODO - implement
    }
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

AudioChannelSlot* AudioMixer::channelForHandle(_SYSAudioHandle handle, uint32_t mask)
{
    if (mask == 0) {
        for (unsigned idx = 0; idx < _SYS_AUDIO_MAX_CHANNELS; idx++) {
            AudioChannelSlot &ch = channelSlots[idx];
            if (ch.handle == handle)
                return &ch;
        }
    } else {
        while (mask) {
            unsigned idx = Intrinsic::CLZ(mask);
            AudioChannelSlot &ch = channelSlots[idx];
            mask &= ~Intrinsic::LZ(idx);
            
            if (ch.handle == handle)
                return &ch;
        }
    }
    return 0;
}
