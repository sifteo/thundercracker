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
#include "speexdecoder.h"
class AudioMixer;

class PwmAudioOut
{
public:
    PwmAudioOut(HwTimer _pwmTimer, int pwmChannel, HwTimer _sampleTimer) :
        pwmTimer(_pwmTimer),
        pwmChan(pwmChannel),
        sampleTimer(_sampleTimer)
    {}
    void init(AudioOutDevice::SampleRate samplerate, AudioMixer *mixer, GPIOPin &outA, GPIOPin &outB);

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

    typedef struct AudioOutBuffer_t {
        int16_t data[SpeexDecoder::DECODED_FRAME_SIZE];
        int count;
        int offset;
    } AudioOutBuffer;

    AudioMixer *mixer;
    AudioOutBuffer audioBufs[2];

    void dmaIsr(uint32_t flags);
//    void tmrIsr();
    friend class AudioOutDevice;
};

#endif // PWM_AUDIO_OUT_H_
