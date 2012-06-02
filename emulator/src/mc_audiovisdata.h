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

    void write(int16_t sample)
    {
        samples[head++] = sample;
        if (head == arraysize(samples))
            head = 0;
    }

    int16_t *getSamples()
    {
        // Return the half of the buffer that isn't being written to.
        if (head >= NUM_SAMPLES)
            return &samples[0];
        else
            return &samples[NUM_SAMPLES];
    }

private:
    unsigned head;
    int16_t samples[NUM_SAMPLES * 2];
};


class MCAudioVisChannel
{
public:
    MCAudioVisScope scope;
};


class MCAudioVisData
{
public:
    MCAudioVisChannel channels[_SYS_AUDIO_MAX_CHANNELS];

    static MCAudioVisData instance;

    static void writeChannelSample(unsigned c, int sample)
    {
        ASSERT(c < arraysize(instance.channels));
        instance.channels[c].scope.write(sample);
    }
};


#endif  // MC_AUDIOVISDATA_H
