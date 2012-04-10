/*
 * Thundercracker Firmware -- Confidential, not for redistribution.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#ifndef AUDIOMIXER_H_
#define AUDIOMIXER_H_

#include "audiobuffer.h"
#include "audiochannel.h"
#include <stdint.h>
#include <stdio.h>
#include <sifteo/abi.h>

class AudioMixer
{
public:
    AudioMixer();

    static AudioMixer instance;

    void init();

    static void test();

    bool play(const struct _SYSAudioModule *mod, _SYSAudioHandle *handle,
        _SYSAudioLoopType loopMode = _SYS_LOOP_ONCE);
    bool isPlaying(_SYSAudioHandle handle);
    void stop(_SYSAudioHandle handle);

    void pause(_SYSAudioHandle handle);
    void resume(_SYSAudioHandle handle);

    void setVolume(_SYSAudioHandle handle, uint16_t volume);
    int volume(_SYSAudioHandle handle);

    uint32_t pos(_SYSAudioHandle handle);

    bool active() const { return playingChannelMask != 0; }

    int mixAudio(int16_t *buffer, uint32_t numsamples);
    static void pullAudio(void *p);

    void setSampleRate(uint32_t samplerate) { curSampleRate = samplerate; };
    uint32_t sampleRate() { return curSampleRate; }
private:
    uint32_t playingChannelMask;    // channels that are actively playing

    _SYSAudioHandle nextHandle;

    AudioChannelSlot channelSlots[_SYS_AUDIO_MAX_CHANNELS];

    AudioChannelSlot* channelForHandle(_SYSAudioHandle handle, uint32_t mask = 0);

    uint32_t curSampleRate;
};

#endif /* AUDIOMIXER_H_ */
