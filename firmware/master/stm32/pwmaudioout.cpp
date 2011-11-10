
#include "pwmaudioout.h"
#include "audiomixer.h"
#include <stdio.h>
#include <string.h>

// uncomment to show sample rate on gpio pin
//#define SAMPLE_RATE_GPIO

#ifdef SAMPLE_RATE_GPIO
static GPIOPin tim4TestPin(&GPIOC, 4);
#endif

PwmAudioOut::PwmAudioOut(HwTimer _pwmTimer, int pwmChannel, HwTimer _sampleTimer, GPIOPin outputA, GPIOPin outputB) :
    pwmTimer(_pwmTimer),
    pwmChan(pwmChannel),
    sampleTimer(_sampleTimer),
    outA(outputA),
    outB(outputB)
{
}

void PwmAudioOut::init(AudioOutDevice::SampleRate samplerate, AudioMixer *mixer)
{
#ifdef SAMPLE_RATE_GPIO
    tim4TestPin.setControl(GPIOPin::OUT_50MHZ);
#endif
    this->mixer = mixer;
    memset(audioBufs, 0, sizeof(audioBufs));
    outA.setControl(GPIOPin::OUT_ALT_50MHZ);
    outB.setControl(GPIOPin::OUT_ALT_50MHZ);

    switch (samplerate) {
    case AudioOutDevice::kHz8000: sampleTimer.init(2200, 0); break;
    case AudioOutDevice::kHz16000: sampleTimer.init(1100, 0); break;
    case AudioOutDevice::kHz32000: sampleTimer.init(550, 0); break;
    }

    pwmTimer.init(500, 0); // TODO - tune the PWM freq we want
    pwmTimer.configureChannel(this->pwmChan,
                                HwTimer::ActiveHigh,
                                HwTimer::Pwm1,
                                HwTimer::ComplementaryOutput);
}

void PwmAudioOut::start()
{
    sampleTimer.setUpdateIsrEnabled(true);
    pwmTimer.enableChannel(this->pwmChan);
}

void PwmAudioOut::stop()
{
    sampleTimer.setUpdateIsrEnabled(false);
    suspend(); // TODO - full shut down?
}

void PwmAudioOut::suspend()
{
    pwmTimer.disableChannel(this->pwmChan);
}

void PwmAudioOut::resume()
{
    pwmTimer.enableChannel(this->pwmChan);
}

void PwmAudioOut::dmaIsr(uint32_t flags)
{
    // figure out which of our buffers to pull into
    AudioOutBuffer *b = &audioBufs[0]; // TODO: ping pong
    b->remaining = mixer->pullAudio(b->data, sizeof(b->data));
    // kick off next DMA transfer
    if (!b->remaining) {
        suspend();
    }
}

/*
 * Called when our sample rate timer's update ISR has fired.
 * Push the next sample into the output device.
 */
void PwmAudioOut::tmrIsr()
{
#ifdef SAMPLE_RATE_GPIO
    tim4TestPin.toggle();
#endif
    AudioOutBuffer *b = &audioBufs[0];
    if (b->remaining <= 0) {
        b->index = 0;
        b->remaining = mixer->pullAudio(b->data, sizeof(b->data));
        if (!b->remaining) {
            return;
        }
    }
    uint16_t duty = (*(uint16_t*)(b->data + b->index)) + 0x8000;
    duty = (duty * pwmTimer.period()) / 0xFFFF; // scale to timer period
    pwmTimer.setDuty(this->pwmChan, duty);
    b->index = (b->index + 2) % sizeof(b->data);
    b->remaining -= 2;
}
