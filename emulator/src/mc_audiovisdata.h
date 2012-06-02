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


class MCAudioVisScope
{
public:
    // Size of scope buffer; also determines duration of each horizontal sweep.
    static const unsigned NUM_SAMPLES = 16000 / 440 * 8;

    MCAudioVisScope();

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
            for (unsigned i = 0; i != NUM_SAMPLES * 2; ++i)
                write(i);
            isClear = true;
        }
    }

    uint16_t *getSamples()
    {
        // Return the half of the buffer that isn't being written to.
        if (head >= NUM_SAMPLES)
            return &samples[0];
        else
            return &samples[NUM_SAMPLES];
    }

private:
    unsigned head;
    bool isClear;
    uint16_t samples[NUM_SAMPLES * 2];
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
