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
#include "dac.h"

namespace DacAudioOut {
    static const HwTimer sampleTimer(&AUDIO_SAMPLE_TIM);

    static const uint16_t maxDacSample = 0xfff;
    static const uint16_t maxRawSample = 0xffff;

    static GPIOPin ampEn = AUDIO_DAC_EN_GPIO;
}

#if BOARD == BOARD_TC_MASTER_REV3

void AudioOutDevice::init()
{
    DacAudioOut::sampleTimer.init(72000000 / AudioMixer::SAMPLE_HZ, 0);

    GPIOPin dacOut = AUDIO_DAC_PIN;
    dacOut.setControl(GPIOPin::IN_ANALOG);

    DacAudioOut::ampEn.setControl(GPIOPin::OUT_2MHZ);
    DacAudioOut::ampEn.setLow();

    Dac::init();
    Dac::configureChannel(AUDIO_DAC_CHAN);
    Dac::enableChannel(AUDIO_DAC_CHAN);

    // Must set up default I/O state
    stop();
}

void AudioOutDevice::start()
{
    // Start clocking out samples
    DacAudioOut::ampEn.setHigh();
    DacAudioOut::sampleTimer.enableUpdateIsr();
}


void AudioOutDevice::stop()
{
    // No more sample data
    DacAudioOut::ampEn.setLow();
    DacAudioOut::sampleTimer.disableUpdateIsr();
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

    while (!AudioMixer::output.empty()) {

        uint16_t duty = AudioMixer::output.dequeue() + 0x8000;
        duty = duty * DacAudioOut::maxDacSample / DacAudioOut::maxRawSample; // scale to 12bit DAC output
        Dac::write(AUDIO_DAC_CHAN, duty, Dac::RightAlign12Bit);

        break;
    }
    // Ask for more audio data
    Tasks::trigger(Tasks::AudioPull);
}

#endif