/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * Thundercracker firmware
 *
 * Copyright <c> 2012 Sifteo, Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
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
    AudioChannelSlot() :
        state(STATE_STOPPED)
    {}

    void play(const struct _SYSAudioModule *module, _SYSAudioLoopType loopMode);

    ALWAYS_INLINE void pause() {
        state |= STATE_PAUSED;
    }

    ALWAYS_INLINE bool isPaused() const {
        return (state & STATE_PAUSED) != 0;
    }

    ALWAYS_INLINE bool isStopped() const {
        return (state & STATE_STOPPED) != 0;
    }

    void resume() {
        state &= ~STATE_PAUSED;
    }

    void stop() {
        state |= STATE_STOPPED;
    }

    void setSpeed(uint32_t sampleRate);

    ALWAYS_INLINE void setVolume(uint16_t newVolume) {
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
