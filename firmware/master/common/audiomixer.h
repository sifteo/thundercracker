/*
 * This file is part of the internal implementation of the Sifteo SDK.
 * Confidential, not for redistribution.
 *
 * Copyright <c> 2011 Sifteo, Inc. All rights reserved.
 */

#ifndef AUDIOMIXER_H_
#define AUDIOMIXER_H_

#include <stdint.h>
#include "speexdecoder.h"
#include "audiobuffer.h"
#include "audiochannel.h"
#include <sifteo/audio.h>
#include <stdio.h>

class AudioMixer
{
public:
    AudioMixer();

    static AudioMixer instance;

    void init();
    void enableChannel(struct _SYSAudioBuffer *buffer);

    static void test();

    //bool play(const struct _SYSAudioModule *mod, _SYSAudioHandle *handle, _SYSAudioLoopType loopMode = LoopOnce);
    bool play(struct _SYSAudioModule *mod, _SYSAudioHandle *handle, _SYSAudioLoopType loopMode = LoopOnce);
    bool isPlaying(_SYSAudioHandle handle);
    void stop(_SYSAudioHandle handle);

    void pause(_SYSAudioHandle handle);
    void resume(_SYSAudioHandle handle);

    void setVolume(_SYSAudioHandle handle, int volume);
    int volume(_SYSAudioHandle handle);

    uint32_t pos(_SYSAudioHandle handle);

    bool active() const { return activeChannelMask != 0; }

    int pullAudio(int16_t *buffer, int numsamples);
    void fetchData();
    static void handleAudioOutEmpty(void *p);

private:
    uint32_t enabledChannelMask;    // channels userspace has provided buffers for
    uint32_t activeChannelMask;     // channels that are actively playing
    uint32_t stoppedChannelMask;    // channels that have finished that need to be cleaned up

    _SYSAudioHandle nextHandle;

    AudioChannelSlot channelSlots[_SYS_AUDIO_MAX_CHANNELS];
    static const uint32_t ALL_CHANNELS_ENABLED = 0xFF;

    // decoders can be loaned to a channel for sample playback
    // TODO: Don't allocate both types of decoders
    SpeexDecoder decoders[_SYS_AUDIO_MAX_CHANNELS];
    PCMDecoder pcmDecoders[_SYS_AUDIO_MAX_CHANNELS];
    uint32_t availableDecodersMask;
    // 8 channels, but left aligned for use with CLZ() & friends
    static const int ALL_DECODERS_AVAILABLE = 0xFF000000;

    AudioChannelSlot* channelForHandle(_SYSAudioHandle handle);
    SpeexDecoder* getDecoder();
    PCMDecoder* getPCMDecoder();
    void stopChannel(AudioChannelSlot *ch);
    bool populateModuleMetaData(struct _SYSAudioModule *mod);
};

#endif /* AUDIOMIXER_H_ */
