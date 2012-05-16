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

    static void test();

    bool play(const struct _SYSAudioModule *mod, _SYSAudioChannelID ch,
        _SYSAudioLoopType loopMode = _SYS_LOOP_ONCE);
    bool isPlaying(_SYSAudioChannelID ch);
    void stop(_SYSAudioChannelID ch);

    void pause(_SYSAudioChannelID ch);
    void resume(_SYSAudioChannelID ch);

    void setVolume(_SYSAudioChannelID ch, uint16_t volume);
    int volume(_SYSAudioChannelID ch);

    void setSpeed(_SYSAudioChannelID ch, uint32_t samplerate);

    void setPos(_SYSAudioChannelID ch, uint32_t ofs);
    uint32_t pos(_SYSAudioChannelID ch);

    bool active() const {
        return playingChannelMask != 0;
    }

    static void pullAudio(void *p);

    void setSampleRate(uint32_t samplerate) {
        curSampleRate = samplerate;
    }
    uint32_t sampleRate() {
        return curSampleRate;
    }

    void setMixerVolume(uint16_t volume);

protected:
    friend class XmTrackerPlayer; // can call setTrackerCallbackInterval()
    void setTrackerCallbackInterval(uint32_t usec);

private:
    uint32_t playingChannelMask;    // channels that are actively playing
    AudioChannelSlot channelSlots[_SYS_AUDIO_MAX_CHANNELS];
    uint32_t curSampleRate;
    uint16_t mixerVolume;

    // Tracker callback timer
    uint32_t trackerCallbackInterval;
    uint32_t trackerCallbackCountdown;

    int mixAudio(int16_t *buffer, uint32_t numsamples);
};

#endif /* AUDIOMIXER_H_ */
