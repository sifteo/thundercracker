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

    void fadeOut() {
        if (outputBufferIsSilent())
            return;

        // http://graphics.stanford.edu/~seander/bithacks.html#CopyIntegerSign
        // if v < 0 then -1, else +1
        int sign = +1 | ((lastSample) >> (sizeof(int) * 8 - 1));
        lastSample &= FADE_MASK;
        fadeStep = sign * FADE_INCREMENT;
    }

    ALWAYS_INLINE bool outputBufferIsSilent() const {
        return numSilentSamples > output.capacity();
    }

    ALWAYS_INLINE bool active() const {
        /*
         * We like to pause the mixer when no sound is playing, to conserve CPU and power.
         *
         * We can only pause the mixer when the output buffer is filled with
         * silence. Any time the mixer wakes up, we need to re-zero the buffer
         * before it goes back to sleep. This is to support output devices which
         * do a looping DMA transfer from the buffer.
         */
        return playingChannelMask != 0 || !outputBufferIsSilent();
    }

    static void pullAudio();

    ALWAYS_INLINE unsigned channelID(AudioChannelSlot *slot) {
        return slot - &channelSlots[0];
    }

protected:
    friend class XmTrackerPlayer; // can call setTrackerCallbackInterval()
    void setTrackerCallbackInterval(uint32_t usec);

private:
    static const int FADE_INCREMENT = 8;
    static const int FADE_MASK = 0xfffffff8;
    static const int FADE_TEST = 0x7;

    int lastSample;
    int fadeStep;

    // Tracker callback timer
    uint32_t trackerCallbackInterval;
    uint32_t trackerCallbackCountdown;

    uint32_t playingChannelMask;    // channels that are actively playing
    uint32_t numSilentSamples;      // Number of samples in 'output' guaranteed to be silent

    AudioChannelSlot channelSlots[_SYS_AUDIO_MAX_CHANNELS];

    bool mixAudio(int *buffer, uint32_t numFrames);

    static int softLimiter(int32_t sample);
};

#endif /* AUDIOMIXER_H_ */
