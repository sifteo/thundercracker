
#include "audiooutdevice.h"
#include "pwmaudioout.h"
#include "dacaudioout.h"

#include "hardware.h"
#include "gpio.h"
#include "hwtimer.h"

#define PWM_BACKEND         0
#define DAC_BACKEND         1
#define AUDIOOUT_BACKEND    DAC_BACKEND


// a bit cheesy, but pound define the static audio backend instance
// since we don't want to bother with inheritance/virtual methods
#if AUDIOOUT_BACKEND == PWM_BACKEND

static const int pwmChan = 2;
static PwmAudioOut pwmAudioOut(HwTimer(&TIM1), pwmChan, HwTimer(&TIM4));
#define audioOutBackend  pwmAudioOut

#elif AUDIOOUT_BACKEND == DAC_BACKEND

static const int dacChan = 1;
static DacAudioOut dacAudioOut(dacChan, HwTimer(&TIM4));
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

#if 1
IRQ_HANDLER ISR_TIM4()
{
    TIM4.SR = 0; // must clear status to acknowledge the ISR
    audioOutBackend.tmrIsr();
}
#endif

void AudioOutDevice::init(SampleRate samplerate, AudioMixer *mixer)
{
    AFIO.MAPR |= (1 << 6);                  // TIM1 partial remap for complementary channels
    NVIC.irqEnable(IVT.TIM4);
    NVIC.irqPrioritize(IVT.TIM4, 0x80);     //   Reduced priority

#if AUDIOOUT_BACKEND == PWM_BACKEND
    GPIOPin outA(&GPIOA, 9);
    GPIOPin outB(&GPIOB, 0);
    pwmAudioOut.init(samplerate, mixer, outA, outB);
#elif AUDIOOUT_BACKEND == DAC_BACKEND
    GPIOPin dacOut(&GPIOA, 4);      // DAC_OUT1
//    GPIOPin dacOut(&GPIOA, 5);      // DAC_OUT2
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
