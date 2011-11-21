
#include "audiooutdevice.h"
#include "pwmaudioout.h"

#include "hardware.h"
#include "gpio.h"
#include "hwtimer.h"

static const int pwmChan = 2;
static PwmAudioOut pwmAudioOut(HwTimer(&TIM1), pwmChan, HwTimer(&TIM4),
                                GPIOPin(&GPIOA, 9), GPIOPin(&GPIOB, 0));

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
    TIM4.SR = 0; // clear status to acknowledge the ISR
    pwmAudioOut.tmrIsr();
}
#endif

void AudioOutDevice::init(SampleRate samplerate, AudioMixer *mixer)
{
    AFIO.MAPR |= (1 << 6);                  // TIM1 partial remap for complementary channels
    NVIC.irqEnable(IVT.TIM4);
    NVIC.irqPrioritize(IVT.TIM4, 0x80);     //   Reduced priority

    pwmAudioOut.init(samplerate, mixer);
}

void AudioOutDevice::start()
{
    pwmAudioOut.start();
}

void AudioOutDevice::stop()
{
    pwmAudioOut.stop();
}

bool AudioOutDevice::isBusy()
{
    return pwmAudioOut.isBusy();
}

int AudioOutDevice::sampleRate()
{
    return pwmAudioOut.sampleRate();
}

void AudioOutDevice::setSampleRate(SampleRate samplerate)
{
//    pwmAudioOut.setSampleRate(samplerate); // not yet implemented
    (void)samplerate;
}

void AudioOutDevice::suspend()
{
    pwmAudioOut.suspend();
}

void AudioOutDevice::resume()
{
    pwmAudioOut.resume();
}
