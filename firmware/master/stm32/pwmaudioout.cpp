/*
 * Thundercracker Firmware -- Confidential, not for redistribution.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#include "pwmaudioout.h"
#include "audiomixer.h"
#include "tasks.h"
#include "sampleprofiler.h"
#include "led.h"

#include <stdio.h>
#include <string.h>

void PwmAudioOut::init(AudioMixer *mixer)
{
    this->mixer = mixer;
    buf.init();

    STATIC_ASSERT(AudioMixer::SAMPLE_HZ == 16000);
    sampleTimer.init(2200, 0);

    // Init the timer, both PWM outputs inverted
    pwmTimer.init(PWM_FREQ, 0);
    pwmTimer.configureChannelAsOutput(pwmChan,
        HwTimer::ActiveLow, HwTimer::Pwm1, HwTimer::ComplementaryOutput);
    pwmTimer.invertComplementaryOutput(pwmChan);

    // Must set up default I/O state
    suspend();
}

void PwmAudioOut::start()
{
    resume();
}

void PwmAudioOut::stop()
{
    suspend();
}

void PwmAudioOut::suspend()
{
    // No more sample data
    sampleTimer.disableUpdateIsr();

    // Stop PWM, put speaker back in idle state
    pwmTimer.disableChannel(pwmChan);

    /*
     * High is the best default "off" state for both BTL outputs,
     * since that's what they'll float to during powerup. (Both have
     * pull-up resistors).
     *
     * We modulate sound by, depending on the sample's sign, "pulling"
     * one or the other leg of the speaker down from this default High state.
     */
    outA.setControl(GPIOPin::OUT_2MHZ);
    outB.setControl(GPIOPin::OUT_2MHZ);
    outA.setHigh();
    outB.setHigh();
}

void PwmAudioOut::resume()
{
    // Start PWM
    pwmTimer.enableChannel(pwmChan);
    pwmTimer.setDuty(pwmChan, 0);
    
    // Start clocking out samples
    sampleTimer.enableUpdateIsr();
    Tasks::setPending(Tasks::AudioPull, &buf);
}

/*
 * Called when our sample rate timer's update ISR has fired.
 * Push the next sample into the output device.
 */
void PwmAudioOut::tmrIsr()
{
    SampleProfiler::SubSystem s = SampleProfiler::subsystem();
    SampleProfiler::setSubsystem(SampleProfiler::AudioISR);

    int period = pwmTimer.period();
    int sample = 0;
    if (buf.readAvailable())
        sample = buf.dequeue();

    if (sample > 0) {
        // + output held HIGH, - output modulated

        pwmTimer.setDuty(pwmChan, (sample * period) >> 15);
        outA.setControl(GPIOPin::OUT_ALT_50MHZ);
        outB.setControl(GPIOPin::OUT_2MHZ);

    } else if (sample < 0) {
        // + output modulated, - output held HIGH

        pwmTimer.setDuty(pwmChan, (-sample * period) >> 15);
        outA.setControl(GPIOPin::OUT_2MHZ);
        outB.setControl(GPIOPin::OUT_ALT_50MHZ);

    } else {
        // Idle / zero volts across speaker.

        outA.setControl(GPIOPin::OUT_2MHZ);
        outB.setControl(GPIOPin::OUT_2MHZ);
    }

    SampleProfiler::setSubsystem(s);
}
