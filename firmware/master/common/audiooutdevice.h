/*
 * Thundercracker Firmware -- Confidential, not for redistribution.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#ifndef AUDIOOUTDEVICE_H_
#define AUDIOOUTDEVICE_H_

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

    static int sampleRate();
    static void setSampleRate(SampleRate samplerate);

    static void suspend();
    static void resume();
};

#endif // AUDIOOUTDEVICE_H_
