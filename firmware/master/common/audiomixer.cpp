/*
 * Thundercracker Firmware -- Confidential, not for redistribution.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#include "audiomixer.h"
#include "audiooutdevice.h"
#include "flashlayer.h"
#include "cubecodec.h"  // TODO: This can be removed when the asset header structs are moved to a common file.
#include <stdio.h>
#include <string.h>

AudioMixer AudioMixer::instance;

AudioMixer::AudioMixer() :
    playingChannelMask(0),
    nextHandle(0)
{
}

void AudioMixer::init()
{
    playingChannelMask = 0;
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

    uint32_t bytesToWrite;
    int16_t *audiobuf = (int16_t*)buf->reserve(buf->writeAvailable(), &bytesToWrite);
    unsigned mixed = AudioMixer::instance.mixAudio(audiobuf, bytesToWrite / sizeof(int16_t));

    // mixed as returned by mixAudio measure samples, but we care about bytes
    mixed *= sizeof(int16_t);
    ASSERT(mixed <= bytesToWrite);
    if (mixed > 0) {
        buf->commit(mixed);
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
