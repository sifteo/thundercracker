/*
 * Thundercracker Firmware -- Confidential, not for redistribution.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#ifndef PWM_AUDIO_OUT_H_
#define PWM_AUDIO_OUT_H_

#include "gpio.h"
#include "hardware.h"
#include "hwtimer.h"

#include "audiobuffer.h"
#include "audiooutdevice.h"
#include "speexdecoder.h"
class AudioMixer;

class PwmAudioOut
{
public:
    PwmAudioOut(HwTimer _pwmTimer, int pwmChannel, HwTimer _sampleTimer, GPIOPin a, GPIOPin b) :
        pwmTimer(_pwmTimer),
        pwmChan(pwmChannel),
        sampleTimer(_sampleTimer),
        outA(a),
        outB(b)
    {}
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

    AudioBuffer buf;
    _SYSAudioBuffer sys;

    AudioMixer *mixer;

    void dmaIsr(uint32_t flags);
    friend class AudioOutDevice;
};

#endif // PWM_AUDIO_OUT_H_
