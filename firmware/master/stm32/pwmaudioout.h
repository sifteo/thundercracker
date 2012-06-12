/*
 * Thundercracker Firmware -- Confidential, not for redistribution.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
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
    PwmAudioOut(HwTimer _pwmTimer, int pwmChannel, HwTimer _sampleTimer, GPIOPin a, GPIOPin b) :
        pwmTimer(_pwmTimer),
        pwmChan(pwmChannel),
        sampleTimer(_sampleTimer),
        outA(a),
        outB(b)
    {}

    void init(AudioMixer *mixer);

    void start();
    void stop();

    void suspend();
    void resume();

    void tmrIsr();

private:
    static const unsigned PWM_FREQ = 2056;   // TODO: tune this!  -- 2056 for 35KHz

    HwTimer pwmTimer;
    int pwmChan;
    HwTimer sampleTimer;
    GPIOPin outA;
    GPIOPin outB;

    AudioBuffer buf;
    AudioMixer *mixer;

    void dmaIsr(uint32_t flags);
    friend class AudioOutDevice;
};

#endif // PWM_AUDIO_OUT_H_
