/*
 * Thundercracker Firmware -- Confidential, not for redistribution.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#ifndef AUDIOOUTDEVICE_H_
#define AUDIOOUTDEVICE_H_

#include <stdint.h>
#include <assert.h>

class AudioMixer;

class AudioOutDevice
{
public:
    enum SampleRate {
        kHz8000,
        kHz16000,
        kHz32000
    };
    static void init(SampleRate samplerate, AudioMixer *mixer);

    static void start();
    static void stop();

    static bool isBusy();

    static uint32_t sampleRate(SampleRate samplerate) {
        {
            switch(samplerate) {
                case kHz8000:
                    return 8000;
                case kHz16000:
                    return 16000;
                case kHz32000:
                    return 32000;
                default:
                    assert(0);
            }
        }
    }
    static void setSampleRate(SampleRate samplerate);

    static void suspend();
    static void resume();
private:
    static AudioMixer *mixer;
};

#endif // AUDIOOUTDEVICE_H_
