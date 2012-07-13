/*
 * Thundercracker Firmware -- Confidential, not for redistribution.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

/*
 * This is one implementation of the generic AudioOutDevice, for
 * sign-magnitude PWM to directly drive an H-bridge from the
 * STM32. We process each sample in software from a high-priority
 * timer IRQ, while one hardware PWM generator creates our
 * actual output waveform.
 *
 * Note that this module is highly performance critical, so
 * we assign all hardware resources at compile-time rather
 * than at init-time as many of our modules do.
 */

#include "audiooutdevice.h"
#include "audiomixer.h"
#include "board.h"
#include "gpio.h"
#include "hwtimer.h"

namespace PwmAudioOut {

    static const unsigned PWM_FREQ = 2056;   // TODO: tune this!  -- 2056 for 35KHz

    static const HwTimer pwmTimer(&TIM1);
    static const HwTimer sampleTimer(&TIM4);
    static const GPIOPin outA(&AUDIO_PWMA_PORT, AUDIO_PWMA_PIN);
    static const GPIOPin outB(&AUDIO_PWMB_PORT, AUDIO_PWMB_PIN);

    static AudioMixer *mixer;
    static AudioBuffer buffer;

}

void AudioOutDevice::init(AudioMixer *mixer)
{
    // TIM1 partial remap for complementary channels
    AFIO.MAPR |= (1 << 6);
    
    PwmAudioOut::mixer = mixer;
    PwmAudioOut::buffer.init();

    STATIC_ASSERT(AudioMixer::SAMPLE_HZ == 16000);
    PwmAudioOut::sampleTimer.init(2200, 0);

    // Init the timer, both PWM outputs inverted
    PwmAudioOut::pwmTimer.init(PwmAudioOut::PWM_FREQ, 0);
    PwmAudioOut::pwmTimer.configureChannelAsOutput(AUDIO_PWM_CHAN,
        HwTimer::ActiveLow, HwTimer::Pwm1, HwTimer::ComplementaryOutput);
    PwmAudioOut::pwmTimer.invertComplementaryOutput(AUDIO_PWM_CHAN);

    // Must set up default I/O state
    stop();
}

void AudioOutDevice::start()
{
    // Start PWM
    PwmAudioOut::pwmTimer.enableChannel(AUDIO_PWM_CHAN);
    PwmAudioOut::pwmTimer.setDuty(AUDIO_PWM_CHAN, 0);
    
    // Start clocking out samples
    PwmAudioOut::sampleTimer.enableUpdateIsr();
    Tasks::setPending(Tasks::AudioPull, &PwmAudioOut::buffer);
}


void AudioOutDevice::stop()
{
    // No more sample data
    PwmAudioOut::sampleTimer.disableUpdateIsr();

    // Stop PWM, put speaker back in idle state
    PwmAudioOut::pwmTimer.disableChannel(AUDIO_PWM_CHAN);

    /*
     * High is the best default "off" state for both BTL outputs,
     * since that's what they'll float to during powerup. (Both have
     * pull-up resistors).
     *
     * We modulate sound by, depending on the sample's sign, "pulling"
     * one or the other leg of the speaker down from this default High state.
     */
    PwmAudioOut::outA.setControl(GPIOPin::OUT_2MHZ);
    PwmAudioOut::outB.setControl(GPIOPin::OUT_2MHZ);
    PwmAudioOut::outA.setHigh();
    PwmAudioOut::outB.setHigh();
}


IRQ_HANDLER ISR_TIM4()
{
    /*
     * This is the sampleTimer (TIM4) ISR, called regularly at our
     * audio sample rate. We're a really high priority ISR at a
     * really high frequency, so this needs to be quick!
     *
     * If you modify this function, please inspect the disassembly to
     * make sure it's still tiny and fast :)
     *
     * Note: We don't bother updating the SampleProfiler subsystsem
     *       here. This should be a really brief IRQ, so the time
     *       needed to simply switch subsystems is comparable to
     *       the duration of the ISR itself. Luckily, everything
     *       here should be inlined, so the sampling bucket for
     *       ISR_TIM4() itself is fully representative of the time
     *       spent in the PWM audio device.
     */

    // Acknowledge IRQ by clearing timer status
    TIM4.SR = 0; 

    // Default state, zero volts across speaker.
    GPIOPin::Control ctrlA = GPIOPin::OUT_2MHZ;
    GPIOPin::Control ctrlB = GPIOPin::OUT_2MHZ;

    if (!PwmAudioOut::buffer.empty()) {
        const HwTimer pwmTimer(&TIM1);
        int sample = PwmAudioOut::buffer.dequeue();

        if (sample > 0) {
            // + output held HIGH, - output modulated

            pwmTimer.setDuty(AUDIO_PWM_CHAN, (sample * PwmAudioOut::PWM_FREQ) >> 15);
            ctrlA = GPIOPin::OUT_ALT_50MHZ;

        } else if (sample < 0) {
            // + output modulated, - output held HIGH

            pwmTimer.setDuty(AUDIO_PWM_CHAN, (-sample * PwmAudioOut::PWM_FREQ) >> 15);
            ctrlB = GPIOPin::OUT_ALT_50MHZ;
        }
    }

    GPIOPin::setControl(&AUDIO_PWMA_PORT, AUDIO_PWMA_PIN, ctrlA);
    GPIOPin::setControl(&AUDIO_PWMB_PORT, AUDIO_PWMB_PIN, ctrlB);
}
