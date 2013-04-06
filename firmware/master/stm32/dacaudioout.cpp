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
#include "dac.h"
#include "dma.h"

#ifdef USE_AUDIO_DAC

namespace DacAudioOut {
    static const HwTimer sampleTimer(&AUDIO_SAMPLE_TIM);
    static GPIOPin ampEn = AUDIO_DAC_EN_GPIO;
    static volatile DMAChannel_t *dmaChannel;

    static void dmaCallback(void *p, uint8_t flags)
    {
        /*
         * DMA half-complete or complete IRQ.
         * Poke the mixer, asynchronously ask it to fill the buffer some more.
         * Update our ring buffer's pointers.
         */

        AudioMixer::output.dequeueWithDMACount(dmaChannel->CNDTR);
        Tasks::trigger(Tasks::AudioPull);
    }
}

void AudioOutDevice::init()
{
    // Sample rate timer, connected to DAC trigger input.
    DacAudioOut::sampleTimer.init(72000000 / AudioMixer::SAMPLE_HZ, 0);
    DacAudioOut::sampleTimer.configureTriggerOutput();

    // DAC output must be configired as an Analog IN, as far as the GPIO controller is concerned.
#ifdef HAS_DIFFERENTIAL_AUDIO
    GPIOPin dacOutNegative = AUDIO_DAC_NEG_PIN;
    GPIOPin dacOutPositive = AUDIO_DAC_POS_PIN;
    dacOutNegative.setControl(GPIOPin::IN_ANALOG);
    dacOutPositive.setControl(GPIOPin::IN_ANALOG);
#else
    GPIOPin dacOut = AUDIO_DAC_PIN;
    dacOut.setControl(GPIOPin::IN_ANALOG);
#endif

    // Amplifier power switch
    DacAudioOut::ampEn.setControl(GPIOPin::OUT_2MHZ);
    DacAudioOut::ampEn.setLow();

    // Set up DMA to pull directly from the circular mixing buffer
    DacAudioOut::dmaChannel = Dma::initChannel(&AUDIO_DAC_DMA, AUDIO_DAC_DMA_CHAN-1, DacAudioOut::dmaCallback, 0);
    DacAudioOut::dmaChannel->CNDTR = AudioMixer::output.getDMACount();
    DacAudioOut::dmaChannel->CMAR = AudioMixer::output.getDMABuffer();
#ifdef HAS_DIFFERENTIAL_AUDIO
    DacAudioOut::dmaChannel->CPAR = Dac::dualAddress(Dac::LeftAlign12Bit);
#else
    DacAudioOut::dmaChannel->CPAR = Dac::address(AUDIO_DAC_CHAN, Dac::LeftAlign12Bit);
#endif
    DacAudioOut::dmaChannel->CCR =  Dma::VeryHighPrio |
                                    (2 << 8) |  // PSIZE - 32-bit peripheral register size
                                    (1 << 7) |  // MINC - memory pointer increment
                                    (1 << 5) |  // CIRC - circular mode enabled
                                    (1 << 4) |  // DIR - direction, 1 == memory -> peripheral
                                    (0 << 3) |  // TEIE - transfer error ISR enable
                                    (1 << 2) |  // HTIE - half complete ISR enable
                                    (1 << 1) ;  // TCIE - transfer complete ISR enable

#ifdef HAS_DIFFERENTIAL_AUDIO
    DacAudioOut::dmaChannel->CCR |= (1 << 11); // MSIZE - 32-bit memory data word size
#else
    DacAudioOut::dmaChannel->CCR |= (1 << 10); // MSIZE - 16-bit memory data word size
#endif

    // Leave the DMA engine enabled. We only trigger it when the DAC's DMA is enabled and
    // the sample timer's trigger fires. If we turn off DMA, we'll lose our place in the ring buffer.
    DacAudioOut::dmaChannel->CCR |= 1;

    // Set up the DAC to trigger on the sample timer, and prepare the DMA channel.
    Dac::init();

#ifdef HAS_DIFFERENTIAL_AUDIO
    Dac::configureChannel(AUDIO_DAC_NEG_CHAN, Dac::AUDIO_SAMPLE_TIM);
    Dac::configureChannel(AUDIO_DAC_POS_CHAN, Dac::AUDIO_SAMPLE_TIM);
    Dac::triggerEnable(AUDIO_DAC_NEG_CHAN);
#else
    Dac::configureChannel(AUDIO_DAC_CHAN, Dac::AUDIO_SAMPLE_TIM);
#endif

    // Must set up default I/O state
    stop();
}

void AudioOutDevice::start()
{
    // Start clocking out samples from a new audio block.
#ifdef HAS_DIFFERENTIAL_AUDIO

    // Enable both DAC channels but only
    // set one channel's DMA enable. This
    // enables us to use 1 DMA channel to drive
    // both DAC channels

    Dac::enableChannel(AUDIO_DAC_NEG_CHAN);
    Dac::enableChannel(AUDIO_DAC_POS_CHAN);
    Dac::enableDMA(AUDIO_DAC_NEG_CHAN);
#else
    Dac::enableChannel(AUDIO_DAC_CHAN);
    Dac::enableDMA(AUDIO_DAC_CHAN);
#endif
    DacAudioOut::ampEn.setHigh();
}


void AudioOutDevice::stop()
{

#ifdef HAS_DIFFERENTIAL_AUDIO
    Dac::disableChannel(AUDIO_DAC_NEG_CHAN);
    Dac::disableChannel(AUDIO_DAC_POS_CHAN);
    Dac::disableDMA(AUDIO_DAC_NEG_CHAN);
#else
    Dac::disableChannel(AUDIO_DAC_CHAN);
    Dac::disableDMA(AUDIO_DAC_CHAN);
#endif

    DacAudioOut::ampEn.setLow();
}

int AudioOutDevice::getSampleBias()
{
    // Convert signed to unsigned samples, with 1/2 full-scale bias.
    return 0x8000;
}

#endif
