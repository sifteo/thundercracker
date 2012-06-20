/*
 * Thundercracker Firmware -- Confidential, not for redistribution.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#ifndef AUDIOCHANNEL_H_
#define AUDIOCHANNEL_H_

#include <sifteo/abi.h>
#include <stdint.h>
#include "machine.h"
#include "audiosampledata.h"

// Fixed-point math offsets
#define SAMPLE_FRAC_SIZE 12
#define SAMPLE_FRAC_MASK ((1 << SAMPLE_FRAC_SIZE) - 1)

class AudioChannelSlot {
public:
    AudioChannelSlot() { init(); }

    void init();

    void play(const struct _SYSAudioModule *module, _SYSAudioLoopType loopMode);

    void pause() {
        state |= STATE_PAUSED;
    }

    bool isPaused() const {
        return (state & STATE_PAUSED) != 0;
    }

    bool isStopped() const {
        return (state & STATE_STOPPED) != 0;
    }

    void resume() {
        state &= ~STATE_PAUSED;
    }

    void stop() {
        state |= STATE_STOPPED;
    }

    void setSpeed(uint32_t sampleRate);

    void setVolume(uint16_t newVolume) {
        ASSERT(newVolume <= _SYS_AUDIO_MAX_VOLUME);
        volume = clamp((int)newVolume, 0, _SYS_AUDIO_MAX_VOLUME);
    }

    void setLoop(_SYSAudioLoopType loopMode) {
        ASSERT(loopMode != _SYS_LOOP_UNDEF);
        if (loopMode == _SYS_LOOP_ONCE)
            state &= ~STATE_LOOP;
        else
            state |= STATE_LOOP;
    }

    void setPos(uint32_t ofs);

protected:
    bool mixAudio(int *buffer, uint32_t numFrames);
    friend class AudioMixer;    // mixer can tell us to mixAudio()

private:
    static const int STATE_PAUSED   = (1 << 0);
    static const int STATE_LOOP     = (1 << 1);
    static const int STATE_STOPPED  = (1 << 2);

    uint64_t offset;
    int32_t increment;
    int16_t volume;
    uint8_t state;

    struct _SYSAudioModule mod;
    AudioSampleData samples;
};

#endif /* AUDIOCHANNEL_H_ */
