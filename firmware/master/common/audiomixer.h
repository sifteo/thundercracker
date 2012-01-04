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
    static const int MAXVOLUME;

    void init();
    void enableChannel(struct _SYSAudioBuffer *buffer);

    static void test();

    bool play(const struct _SYSAudioModule *mod, _SYSAudioHandle *handle, _SYSAudioLoopType loopMode = LoopOnce);
    bool isPlaying(_SYSAudioHandle handle);
    void stop(_SYSAudioHandle handle);

    void pause(_SYSAudioHandle handle);
    void resume(_SYSAudioHandle handle);

    void setVolume(_SYSAudioHandle handle, int volume);
    int volume(_SYSAudioHandle handle);

    uint32_t pos(_SYSAudioHandle handle);

    bool active() const { return activeChannelMask; }

    int pullAudio(int16_t *buffer, int numsamples);
    void fetchData();

private:
    uint32_t enabledChannelMask;    // channels userspace has provided buffers for
    uint32_t activeChannelMask;     // channels that are actively playing

    _SYSAudioHandle nextHandle;

    AudioChannelWrapper channels[_SYS_AUDIO_MAX_CHANNELS];
    static const uint32_t ALL_CHANNELS_ENABLED = 0xFF;

    // decoders can be loaned to a channel for sample playback
    SpeexDecoder decoders[_SYS_AUDIO_MAX_CHANNELS];
    uint32_t availableDecodersMask;
    static const int ALL_DECODERS_AVAILABLE = 0xFF;

    AudioChannelWrapper* channelForHandle(_SYSAudioHandle handle);
    SpeexDecoder* getDecoder();
    void stopChannel(AudioChannelWrapper *ch);
};

#endif /* AUDIOMIXER_H_ */
