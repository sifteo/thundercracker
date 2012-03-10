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
#include <sifteo.h>

using namespace Sifteo;

//statics
AudioMixer AudioMixer::instance;

AudioMixer::AudioMixer() :
    enabledChannelMask(0),
    playingChannelMask(0),
    nextHandle(0)
{
}

void AudioMixer::init()
{
    memset(channelSlots, 0, sizeof(channelSlots));
    enabledChannelMask = 0;
    playingChannelMask = 0;
}

/*
 * Userspace must supply a buffer for each channel they want to enable.
 * Once they do, the channel can be used in subsequent calls to play().
 */
void AudioMixer::enableChannel(struct _SYSAudioBuffer *buffer)
{
    // find the first disabled channel, init it and mark it as enabled
    for (unsigned idx = 0; idx < _SYS_AUDIO_MAX_CHANNELS; idx++) {
        if (!(enabledChannelMask & Intrinsic::LZ(idx))) {
            channelSlots[idx].init(buffer);
            Atomic::SetLZ(enabledChannelMask, idx);
            return;
        }
    }
}

/*
 * Slurp the data from all active channels into the out device's
 * buffer.
 * Called from the AudioOutDevice when it needs more data, from within the
 * sample rate timer ISR.
 */
int AudioMixer::pullAudio(int16_t *buffer, int numsamples)
{
    // NOTE - *must* bail ASAP if we don't have anything to do. we really
    // want to minimize the work done in this ISR since it's happening so
    // frequently
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
 * Time to grab more audio data from flash.
 * Check whether each channel needs more data and grab if necessary.
 *
 * This currently assumes that it's being run on the main thread, such that it
 * can operate synchronously with data arriving from flash.
 */
void AudioMixer::fetchData()
{
    uint32_t mask = playingChannelMask;
    while (mask) {
        unsigned idx = Intrinsic::CLZ(mask);
        AudioChannelSlot &ch = channelSlots[idx];
        mask &= ~Intrinsic::LZ(idx);

        if (ch.isStopped())
            Atomic::ClearLZ(playingChannelMask, idx);
        else
            ch.fetchData();
    }
}

/*
    Called from within Tasks::work to mix audio on the main thread, to be
    consumed by the audio out device.
*/
void AudioMixer::handleAudioOutEmpty(void *p) {
    AudioBuffer *buf = static_cast<AudioBuffer*>(p);

    unsigned bytesToWrite;
    int16_t *audiobuf = (int16_t*)buf->reserve(buf->writeAvailable(), &bytesToWrite);
    unsigned mixed = AudioMixer::instance.pullAudio(audiobuf, bytesToWrite / sizeof(int16_t));
    ASSERT(mixed < bytesToWrite * sizeof(int16_t));
    if (mixed > 0) {
        buf->commit(mixed * sizeof(int16_t));
    }
}

bool AudioMixer::play(const struct _SYSAudioModule *mod,
    _SYSAudioHandle *handle, _SYSAudioLoopType loopMode)
{
    // NB: "mod" is a temporary contiguous copy of SYSAudioModule in RAM.
    
    // find channels that are enabled but not playing
    uint32_t selector = enabledChannelMask & ~playingChannelMask;
    if (selector == 0) {
        return false;
    }

    unsigned idx = Intrinsic::CLZ(selector);
    ASSERT(idx < _SYS_AUDIO_MAX_CHANNELS);
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

void AudioMixer::setVolume(_SYSAudioHandle handle, int volume)
{
    if (AudioChannelSlot *ch = channelForHandle(handle, enabledChannelMask)) {
        ch->volume = Math::clamp(volume, 0, (int)Audio::MAX_VOLUME);
    }
}

int AudioMixer::volume(_SYSAudioHandle handle)
{
    if (AudioChannelSlot *ch = channelForHandle(handle, enabledChannelMask)) {
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
    while (mask) {
        unsigned idx = Intrinsic::CLZ(mask);
        AudioChannelSlot &ch = channelSlots[idx];
        mask &= ~Intrinsic::LZ(idx);

        if (ch.handle == handle)
            return &ch;
    }
    return 0;
}
