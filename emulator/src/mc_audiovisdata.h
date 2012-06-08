/*
 * Thundercracker Firmware -- Confidential, not for redistribution.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
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
