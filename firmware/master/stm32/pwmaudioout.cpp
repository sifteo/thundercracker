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
#include "prng.h"

namespace PwmAudioOut {

    /*
     * The frequency of our PWM carrier is 72MHz / PWM_PERIOD.
     *
     * This can be any 16-bit number, but we save time
     * in the ISR if it's a power of two.
     *
     * Higher periods / lower frequencies give us more effective
     * resolution and better power efficiency, whereas lower
     * periods / higher frequencies reduce carrier noise at the cost
     * of power and resolution.
     *
     * We want the PWM frequency to be far enough above the range of
     * human hearing that we don't get any audible aliasing back down
     * into frequencies we can hear. I can still hear the carrier
     * pretty clearly at 40 Khz. 50 KHz seems to be fine.
     *
     * So, currently this is set to the next power of two,
     * which gives us 70 Khz and a nice round number.
     */
    static const unsigned PWM_PERIOD = 1024;

    /*
     * The conversion from 16-bit audio sample to PWM duty cycle is lossy
     * as long as PWM_PERIOD is less than 2^15. To decorrelate the
     * quantization error and improve our performance especially with
     * lower volume levels, add a random dither of less than 1 LSB.
     *
     * This value is the maximum amount of dither we add to a sample
     * before quantizing it. The ideal value for this would be 2^15
     * divided by PWM_PERIOD. For efficiency, however, this must be
     * a bit mask of the form (2^N)-1. If PWM_PERIOD is a power of two
     * we can choose the ideal value for this constant as well, but if
     * PWM_POWER is not a power of two we'll have to approximate.
     */
    static const unsigned DITHER_MASK = 31;

    static const HwTimer pwmTimer(&AUDIO_PWM_TIM);
    static const HwTimer sampleTimer(&AUDIO_SAMPLE_TIM);
    static const GPIOPin outA(&AUDIO_PWMA_PORT, AUDIO_PWMA_PIN);
    static const GPIOPin outB(&AUDIO_PWMB_PORT, AUDIO_PWMB_PIN);

    static _SYSPseudoRandomState dither;

    static AudioMixer *mixer;
    static AudioBuffer buffer;

}

void AudioOutDevice::init(AudioMixer *mixer)
{
    // TIM1 partial remap for complementary channels
    STATIC_ASSERT(&AUDIO_PWM_TIM == &TIM1);
    AFIO.MAPR |= (1 << 6);
    
    PwmAudioOut::mixer = mixer;
    PwmAudioOut::buffer.init();

    // Initialize PRNG for audio dithering
    PRNG::init(&PwmAudioOut::dither, 0);

    STATIC_ASSERT(AudioMixer::SAMPLE_HZ == 16000);
    PwmAudioOut::sampleTimer.init(2200, 0);

    // Init the timer, both PWM outputs inverted
    PwmAudioOut::pwmTimer.init(PwmAudioOut::PWM_PERIOD, 0);
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


IRQ_HANDLER ISR_FN(AUDIO_SAMPLE_TIM)()
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
    AUDIO_SAMPLE_TIM.SR = 0; 

    // Default state, zero volts across speaker.
    GPIOPin::Control ctrlA = GPIOPin::OUT_2MHZ;
    GPIOPin::Control ctrlB = GPIOPin::OUT_2MHZ;

    if (!PwmAudioOut::buffer.empty()) {
        int sample = PwmAudioOut::buffer.dequeue();

        /*
         * We've now extracted the sign, and the remaining sample is a 15-bit
         * unsigned value. Convert it to a hardware PWM duty cycle, ranging from
         * 0 to just below PWM_PERIOD.
         *
         * This conversion is lossy, since our PWM resolution is lower than our
         * original 16-bit audio. To decorrelate the quantization error and improve
         * our performance especially with lower volume levels, add a random dither
         * of less than 1 LSB.
         *
         * This means that, prior to scaling, our dither needs to be at
         * most 2^15 / PWM_PERIOD.
         */

        sample += PRNG::valueInline(&PwmAudioOut::dither) & PwmAudioOut::DITHER_MASK;
        sample = Intrinsic::SSAT(sample, 16);

        if (sample > 0) {
            // + output held HIGH, - output modulated
            ctrlA = GPIOPin::OUT_ALT_50MHZ;

        } else if (sample < 0) {
            // + output modulated, - output held HIGH
            sample = -sample;
            ctrlB = GPIOPin::OUT_ALT_50MHZ;
        }

        if (sample) {
            unsigned duty = (sample * PwmAudioOut::PWM_PERIOD) >> 15;
            const HwTimer pwmTimer(&AUDIO_PWM_TIM);
            pwmTimer.setDuty(AUDIO_PWM_CHAN, duty);
        }
    }

    GPIOPin::setControl(&AUDIO_PWMA_PORT, AUDIO_PWMA_PIN, ctrlA);
    GPIOPin::setControl(&AUDIO_PWMB_PORT, AUDIO_PWMB_PIN, ctrlB);
}
