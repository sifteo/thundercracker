
#include "pwmaudioout.h"
#include "audiomixer.h"
#include "tasks.h"
#include <stdio.h>
#include <string.h>

// uncomment to show sample rate on gpio pin
//#define SAMPLE_RATE_GPIO

#ifdef SAMPLE_RATE_GPIO
static GPIOPin tim4TestPin(&GPIOC, 4);
#endif

void PwmAudioOut::init(AudioOutDevice::SampleRate samplerate, AudioMixer *mixer, GPIOPin &outA, GPIOPin &outB)
{
#ifdef SAMPLE_RATE_GPIO
    tim4TestPin.setControl(GPIOPin::OUT_50MHZ);
#endif
    this->mixer = mixer;
    buf.init(&this->sys);
    outA.setControl(GPIOPin::OUT_ALT_50MHZ);
    outB.setControl(GPIOPin::OUT_ALT_50MHZ);

    switch (samplerate) {
    case AudioOutDevice::kHz8000: sampleTimer.init(2200, 0); break;
    case AudioOutDevice::kHz16000: sampleTimer.init(2200, 0); break;
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
    sampleTimer.enableUpdateIsr();
    pwmTimer.enableChannel(this->pwmChan);
}

void PwmAudioOut::stop()
{
    sampleTimer.disableUpdateIsr();
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

/*
 * Called when our sample rate timer's update ISR has fired.
 * Push the next sample into the output device.
 */
void PwmAudioOut::tmrIsr()
{
#ifdef SAMPLE_RATE_GPIO
    tim4TestPin.toggle();
#endif
    // TODO - tune the refill threshold if needed
    if (buf.readAvailable() < buf.capacity() / 2) {
        Tasks::setPending(Tasks::AudioOutEmpty, &buf);
        if (buf.readAvailable() < 2)
            return;
    }

    uint16_t duty = (buf.dequeue() | (buf.dequeue() << 8)) + 0x8000;
    duty = (duty * pwmTimer.period()) / 0xFFFF; // scale to timer period
    pwmTimer.setDuty(this->pwmChan, duty);
}
