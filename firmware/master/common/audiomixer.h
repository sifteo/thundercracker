/*
 * Thundercracker Firmware -- Confidential, not for redistribution.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#ifndef AUDIOMIXER_H_
#define AUDIOMIXER_H_

#include "ringbuffer.h"
#include "audiochannel.h"
#include <stdint.h>
#include <stdio.h>
#include <sifteo/abi.h>

class AudioMixer
{
public:
    // Global sample rate for mixing and audio output
    static const unsigned SAMPLE_HZ = 16000;

    // Type and size for output buffer, between mixer and audio device
    typedef RingBuffer<1024, int16_t> OutputBuffer;

    AudioMixer();

    void init();

    static AudioMixer instance;
    static OutputBuffer output;

    static void test();

    bool play(const struct _SYSAudioModule *mod, _SYSAudioChannelID ch,
        _SYSAudioLoopType loopMode = _SYS_LOOP_ONCE);
    bool isPlaying(_SYSAudioChannelID ch) const;
    void stop(_SYSAudioChannelID ch);

    void pause(_SYSAudioChannelID ch);
    void resume(_SYSAudioChannelID ch);

    void setVolume(_SYSAudioChannelID ch, uint16_t volume);
    int volume(_SYSAudioChannelID ch) const;

    void setSpeed(_SYSAudioChannelID ch, uint32_t samplerate);

    void setPos(_SYSAudioChannelID ch, uint32_t ofs);
    uint32_t pos(_SYSAudioChannelID ch) const;

    void setLoop(_SYSAudioChannelID ch, _SYSAudioLoopType loopMode);

    ALWAYS_INLINE bool active() const {
        return playingChannelMask != 0;
    }

    static void pullAudio();

    ALWAYS_INLINE unsigned channelID(AudioChannelSlot *slot) {
        return slot - &channelSlots[0];
    }

protected:
    friend class XmTrackerPlayer; // can call setTrackerCallbackInterval()
    void setTrackerCallbackInterval(uint32_t usec);

private:
    // Tracker callback timer
    uint32_t trackerCallbackInterval;
    uint32_t trackerCallbackCountdown;

    uint32_t playingChannelMask;    // channels that are actively playing

    AudioChannelSlot channelSlots[_SYS_AUDIO_MAX_CHANNELS];

    bool mixAudio(int *buffer, uint32_t numFrames);
    static int16_t softLimiter(int sample);
};

#endif /* AUDIOMIXER_H_ */
