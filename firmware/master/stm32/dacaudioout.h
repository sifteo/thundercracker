/*
 * Thundercracker Firmware -- Confidential, not for redistribution.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#ifndef DAC_AUDIO_OUT_H
#define DAC_AUDIO_OUT_H

#include "dac.h"
#include "gpio.h"
#include "hardware.h"
#include "hwtimer.h"

#include "audiobuffer.h"
#include "audiooutdevice.h"
#include "speexdecoder.h"
class AudioMixer;

class DacAudioOut
{
public:
    DacAudioOut(int dacChannel, HwTimer _sampleTimer) :
        dac(Dac::instance),
        dacChan(dacChannel),
        sampleTimer(_sampleTimer)
    {}
    void init(AudioOutDevice::SampleRate samplerate, AudioMixer *mixer, GPIOPin &output);

    void start();
    void stop();

    bool isBusy() const;

    int sampleRate() const;
    void setSampleRate(AudioOutDevice::SampleRate samplerate);

    void suspend();
    void resume();

    void tmrIsr();

private:
    Dac &dac;
    int dacChan;
    HwTimer sampleTimer;

    AudioBuffer buf;
    _SYSAudioBuffer sys;

    AudioMixer *mixer;

    void dmaIsr(uint32_t flags);
//    void tmrIsr();
    friend class AudioOutDevice;
};

#endif // DAC_AUDIO_OUT_H
