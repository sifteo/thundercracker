/*
 * This file is part of the internal implementation of the Sifteo SDK.
 * Confidential, not for redistribution.
 *
 * Copyright <c> 2011 Sifteo, Inc. All rights reserved.
 */

#ifndef PWM_AUDIO_OUT_H_
#define PWM_AUDIO_OUT_H_

#include "gpio.h"
#include "hardware.h"
#include "hwtimer.h"

#include "audiooutdevice.h"
class AudioMixer;

class PwmAudioOut
{
public:
    PwmAudioOut(HwTimer _pwmTimer, int pwmChannel, HwTimer _sampleTimer, GPIOPin outputA, GPIOPin outputB);
    void init(AudioOutDevice::SampleRate samplerate, AudioMixer *mixer);

    void start();
    void stop();

    bool isBusy() const;

    int sampleRate() const;
    void setSampleRate(AudioOutDevice::SampleRate samplerate);

    void suspend();
    void resume();

    void tmrIsr();

private:
    HwTimer pwmTimer;
    int pwmChan;
    HwTimer sampleTimer;
    GPIOPin outA;
    GPIOPin outB;

    typedef struct AudioBuffer_t {
        char data[256];
        int remaining;
        int index;
    } AudioBuffer;

    AudioMixer *mixer;
    AudioBuffer audioBufs[2];

    void dmaIsr(uint32_t flags);
//    void tmrIsr();
    friend class AudioOutDevice;
};

#endif // PWM_AUDIO_OUT_H_
