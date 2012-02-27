
#include "audiooutdevice.h"
#include "pwmaudioout.h"
#include "dacaudioout.h"

#include "board.h"
#include "gpio.h"
#include "hwtimer.h"

#define PWM_BACKEND         0
#define DAC_BACKEND         1
#define AUDIOOUT_BACKEND    DAC_BACKEND


// a bit cheesy, but pound define the static audio backend instance
// since we don't want to bother with inheritance/virtual methods
#if AUDIOOUT_BACKEND == PWM_BACKEND

static PwmAudioOut pwmAudioOut(HwTimer(&TIM1), AUDIO_PWM_CHAN, HwTimer(&TIM4));
#define audioOutBackend  pwmAudioOut

#elif AUDIOOUT_BACKEND == DAC_BACKEND

static DacAudioOut dacAudioOut(AUDIO_DAC_CHAN, HwTimer(&TIM4));
#define audioOutBackend dacAudioOut

#else
#error unknown audio out device :(
#endif

/*
 * STM32 implementation of AudioOutDevice - just forward calls to the
 * appropriate hw specific device. This sucks at the moment, since
 * we're paying for function call overhead, according to the map file - even at O2.
 *
 * There's probably a better way to organize this, but this at least
 * allows us to keep the same audiooutdevice.h between the cube and the
 * simulator.
 */

IRQ_HANDLER ISR_TIM4()
{
    TIM4.SR = 0; // must clear status to acknowledge the ISR
    audioOutBackend.tmrIsr();
}

void AudioOutDevice::init(SampleRate samplerate, AudioMixer *mixer)
{
#if AUDIOOUT_BACKEND == PWM_BACKEND

    AFIO.MAPR |= (1 << 6);          // TIM1 partial remap for complementary channels
    GPIOPin outA = AUDIO_PWMA_GPIO;
    GPIOPin outB = AUDIO_PWMB_GPIO;
    pwmAudioOut.init(samplerate, mixer, outA, outB);

#elif AUDIOOUT_BACKEND == DAC_BACKEND

    GPIOPin dacOut = AUDIO_DAC_GPIO;
    dacAudioOut.init(samplerate, mixer, dacOut);

    // XXX: Amplifier always on
    GPIOPin ampEnable(&GPIOA, 6);
    ampEnable.setControl(GPIOPin::OUT_2MHZ);
    ampEnable.setHigh();

#endif
}

void AudioOutDevice::start()
{
    audioOutBackend.start();
}

void AudioOutDevice::stop()
{
    audioOutBackend.stop();
}

bool AudioOutDevice::isBusy()
{
    return false; //audioOutBackend.isBusy();
}

int AudioOutDevice::sampleRate()
{
    return 0; //audioOutBackend.sampleRate();
}

void AudioOutDevice::setSampleRate(SampleRate samplerate)
{
    // TODO: implement?
    (void)samplerate;
}

void AudioOutDevice::suspend()
{
    audioOutBackend.suspend();
}

void AudioOutDevice::resume()
{
    audioOutBackend.resume();
}
