/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * Sifteo Thundercracker simulator
 * Micah Elizabeth Scott <micah@misc.name>
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

/*
 * Shared data model for the audio mixer and audio visualizer.
 */

#ifndef MC_AUDIOVISDATA_H
#define MC_AUDIOVISDATA_H

#include <sifteo/abi.h>
#include "macros.h"
#include "audiomixer.h"


class MCAudioVisScope
{
public:
    // Size of scope buffer; also determines duration of each horizontal sweep.
    static const unsigned NUM_SAMPLES = AudioMixer::SAMPLE_HZ / 440 * 10;

    // Size of array; holds two logical buffers
    static const unsigned ARRAY_SIZE = NUM_SAMPLES * 2;

    // Number of samples to search for a trigger
    static const unsigned TRIGGER_SEARCH_LEN = NUM_SAMPLES / 2;

    // Number of samples visible in one scope sweep.
    static const unsigned SWEEP_LEN = NUM_SAMPLES - TRIGGER_SEARCH_LEN;

    MCAudioVisScope();
    uint16_t *getSweep();

    void write(int16_t sample)
    {
        isClear = false;
        samples[head++] = sample + 0x8000;
        if (head == arraysize(samples))
            head = 0;
    }

    void clear()
    {
        if (!isClear) {
            for (unsigned i = 0; i != ARRAY_SIZE; ++i)
                write(i);
            isClear = true;
        }
    }

private:
    unsigned head;
    bool isClear;
    uint16_t samples[ARRAY_SIZE];
};


class MCAudioVisChannel
{
public:
    MCAudioVisScope scope;
};


class MCAudioVisData
{
public:
    static const unsigned NUM_CHANNELS = _SYS_AUDIO_MAX_CHANNELS;

    bool mixerActive;
    MCAudioVisChannel channels[NUM_CHANNELS];

    static MCAudioVisData instance;

    static void writeChannelSample(unsigned c, int sample)
    {
        ASSERT(c < arraysize(instance.channels));
        instance.channels[c].scope.write(sample);
    }

    static void clearChannel(unsigned c)
    {
        ASSERT(c < arraysize(instance.channels));
        instance.channels[c].scope.clear();
    }
};


#endif  // MC_AUDIOVISDATA_H
