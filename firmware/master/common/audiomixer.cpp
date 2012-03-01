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
    activeChannelMask(0),
    stoppedChannelMask(0),
    nextHandle(0),
    availableDecodersMask(ALL_DECODERS_AVAILABLE)
{
}

void AudioMixer::init()
{
    memset(channelSlots, 0, sizeof(channelSlots));
    // NOTE: several games are failing their builds for hardware because the
    // speex libs & their own asset metadata overflow MCU internal flash.
    // Disabling this init allows the linker to strip all the speex code, and
    // for many games to fit in flash - of course, if you try to run on HW with
    // speex encoded samples, it will go up in flames. run on HW with uncompressed
    // samples for now, until absolutely everything is being pulled from external
    // flash, in which case we should be able to pull speex back in.
#ifdef SIFTEO_SIMULATOR
    for (int i = 0; i < _SYS_AUDIO_MAX_CHANNELS; i++) {
        decoders[i].init();
    }
#endif
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
    if (activeChannelMask == 0) {
        return 0;
    }
    memset(buffer, 0, numsamples * sizeof(*buffer));

    int samplesMixed = 0;
    // iterate through active channels that are not waiting to be stopped
    uint32_t mask = activeChannelMask & ~stoppedChannelMask;
    while (mask) {
        unsigned idx = Intrinsic::CLZ(mask);
        ASSERT(idx < _SYS_AUDIO_MAX_CHANNELS);
        AudioChannelSlot &ch = channelSlots[idx];
        Atomic::ClearLZ(mask, idx);

        if (ch.isPaused()) {
            continue;
        }
        
        // Each channel individually mixes itself with the existing buffer contents
        int mixed = ch.mixAudio(buffer, numsamples);

        // Update size of overall mixed audio buffer
        if (mixed > samplesMixed) {
            samplesMixed = mixed;
        }
        else if (mixed < 0) {
            Atomic::SetLZ(stoppedChannelMask, idx);
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
    // first, handle any channels that have finished since we last rolled around
    while (stoppedChannelMask) {
        unsigned idx = Intrinsic::CLZ(stoppedChannelMask);
        stopChannel(&channelSlots[idx]);
        Atomic::ClearLZ(stoppedChannelMask, idx);
    }

    uint32_t mask = activeChannelMask;
    while (mask) {
        unsigned idx = Intrinsic::CLZ(mask);
        channelSlots[idx].fetchData();
        Atomic::ClearLZ(mask, idx);
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

/*
    Read the details of this module out of flash.
*/
bool AudioMixer::populateModuleMetaData(struct _SYSAudioModule *mod)
{
    // TODO: can we tag these as already populated and avoid subsequent lookups?
    FlashRegion ar;
    unsigned entryOffset = mod->id * sizeof(AssetIndexEntry);
    // XXX: this is assuming an offset of 0 for the whole asset segment.
    // 'entryOffset' will eventually need to be applied to the offset of the segment itself in flash
    if (!FlashLayer::getRegion(0 + entryOffset, FlashLayer::BLOCK_SIZE, &ar)) {
        LOG(("got null AssetIndexEntry from flash"));
        return false;
    }

    AssetIndexEntry entry;
    memcpy(&entry, ar.data(), sizeof(AssetIndexEntry));
    FlashLayer::releaseRegion(ar);

    unsigned offset = entry.offset;
    FlashRegion hr;
    if (!FlashLayer::getRegion(offset, sizeof(SoundHeader), &hr)) {
        LOG(("Got null SoundHeader from flashlayer\n"));
        return false;
    }

    SoundHeader header;
    memcpy(&header, hr.data(), sizeof(SoundHeader));
    FlashLayer::releaseRegion(hr);

    mod->offset = offset + sizeof(SoundHeader);
    mod->size = header.dataSize;
    mod->type = (_SYSAudioType)header.encoding;
    return true;
}

bool AudioMixer::play(struct _SYSAudioModule *mod, _SYSAudioHandle *handle, _SYSAudioLoopType loopMode)
{
    // find channels that are enabled, not active & not marked as stopped
    uint32_t selector = enabledChannelMask & ~activeChannelMask & ~stoppedChannelMask;
    if (selector == 0) {
        return false;
    }

    if (!populateModuleMetaData(mod)) {
        return false;
    }

    unsigned idx = Intrinsic::CLZ(selector);
    ASSERT(idx < _SYS_AUDIO_MAX_CHANNELS);
    AudioChannelSlot &ch = channelSlots[idx];
    ch.handle = nextHandle++;
    *handle = ch.handle;

    // does this module require a decoder? if so, get one
    if (mod->type == Sample) {
        SpeexDecoder *dec = getDecoder();
        if (dec == NULL) {
            LOG(("ERROR: No SpeexDecoder available.\n"));
            return false;
        }
        ch.play(mod, loopMode, dec);
    }
    else if (mod->type == PCM) {
        PCMDecoder *pcmdec = getPCMDecoder();
        if (pcmdec == NULL) {
            LOG(("ERROR: No PCMDecoder available.\n"));
            return false;
        }
        ch.play(mod, loopMode, pcmdec);
    }
    else {
        LOG(("ERROR: Unknown audio encoding. id: %d type: %d.\n", mod->id, mod->type));
        return false;
    }

    Atomic::SetLZ(activeChannelMask, idx);

    return true;
}

bool AudioMixer::isPlaying(_SYSAudioHandle handle)
{
    uint32_t mask = activeChannelMask & ~stoppedChannelMask;
    return channelForHandle(handle, mask) != 0;
}

void AudioMixer::stop(_SYSAudioHandle handle)
{
    uint32_t mask = activeChannelMask & ~stoppedChannelMask;
    if (AudioChannelSlot *ch = channelForHandle(handle, mask)) {
        stopChannel(ch);
    }
}

void AudioMixer::stopChannel(AudioChannelSlot *ch)
{
    unsigned channelIndex = ch - channelSlots;
    ASSERT(channelIndex < _SYS_AUDIO_MAX_CHANNELS);
    ASSERT(activeChannelMask & Intrinsic::LZ(channelIndex));

    // mark this channel's decoder as available again
    if (ch->channelType() == Sample) {
        unsigned decoderIndex = ch->decoder - decoders;
        ASSERT(decoderIndex < _SYS_AUDIO_MAX_CHANNELS);
        ASSERT(!(availableDecodersMask & Intrinsic::LZ(decoderIndex)));
        Atomic::SetLZ(availableDecodersMask, decoderIndex);
    }
    else if (ch->channelType() == PCM) {
        unsigned decoderIndex = ch->pcmDecoder - pcmDecoders;
        ASSERT(decoderIndex < _SYS_AUDIO_MAX_CHANNELS);
        ASSERT(!(availableDecodersMask & Intrinsic::LZ(decoderIndex)));
        Atomic::SetLZ(availableDecodersMask, decoderIndex);
    }
    ch->onPlaybackComplete();
    Atomic::ClearLZ(activeChannelMask, channelIndex);
}

void AudioMixer::pause(_SYSAudioHandle handle)
{
    uint32_t mask = activeChannelMask & ~stoppedChannelMask;
    if (AudioChannelSlot *ch = channelForHandle(handle, mask)) {
        ch->pause();
    }
}

void AudioMixer::resume(_SYSAudioHandle handle)
{
    uint32_t mask = activeChannelMask & ~stoppedChannelMask;
    if (AudioChannelSlot *ch = channelForHandle(handle, mask)) {
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
    uint32_t mask = activeChannelMask & ~stoppedChannelMask;
    if (AudioChannelSlot *ch = channelForHandle(handle, mask)) {
        ch = 0;
        // TODO - implement
    }
    return 0;
}

AudioChannelSlot* AudioMixer::channelForHandle(_SYSAudioHandle handle, uint32_t mask)
{
    while (mask) {
        unsigned idx = Intrinsic::CLZ(mask);
        if (channelSlots[idx].handle == handle) {
            return &channelSlots[idx];
        }
        Atomic::ClearLZ(mask, idx);
    }
    return 0;
}

SpeexDecoder* AudioMixer::getDecoder()
{
    if (availableDecodersMask == 0) {
        return NULL;
    }

    unsigned idx = Intrinsic::CLZ(availableDecodersMask);
    Atomic::ClearLZ(availableDecodersMask, idx);
    return &decoders[idx];
}

PCMDecoder* AudioMixer::getPCMDecoder()
{
    if (availableDecodersMask == 0) {
        return NULL;
    }

    unsigned idx = Intrinsic::CLZ(availableDecodersMask);
    Atomic::ClearLZ(availableDecodersMask, idx);
    return &pcmDecoders[idx];
}
