/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * Thundercracker firmware
 *
 * Copyright <c> 2012 Sifteo, Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include "led.h"
#include "gpio.h"
#include "board.h"
#include "macros.h"
#include "hwtimer.h"

namespace LED {
    static LEDSequencer sequencer;
}

void LED::init()
{
    sequencer.init();

    const HwTimer pwmTimer(&LED_PWM_TIM);
    const HwTimer seqTimer(&LED_SEQUENCER_TIM);
    const GPIOPin outRed = LED_RED_GPIO;
    const GPIOPin outGreen = LED_GREEN_GPIO;

    /*
     * PWM: Full 16-bit resolution, about 1 kHz period
     */
    pwmTimer.init(0xFFFF, 0);
    pwmTimer.configureChannelAsOutput(LED_PWM_GREEN_CHAN,
        HwTimer::ActiveLow, HwTimer::Pwm1, HwTimer::SingleOutput);
    pwmTimer.configureChannelAsOutput(LED_PWM_RED_CHAN,
        HwTimer::ActiveLow, HwTimer::Pwm1, HwTimer::SingleOutput);

    pwmTimer.enableChannel(LED_PWM_GREEN_CHAN);
    pwmTimer.enableChannel(LED_PWM_RED_CHAN);

    pwmTimer.setDuty(LED_PWM_RED_CHAN, 0);
    pwmTimer.setDuty(LED_PWM_GREEN_CHAN, 0);

    seqTimer.init(72000 / LEDSequencer::TICK_HZ, 1000);
    seqTimer.enableUpdateIsr();

    outRed.setControl(GPIOPin::OUT_ALT_2MHZ);
    outGreen.setControl(GPIOPin::OUT_ALT_2MHZ);
}

void LED::set(const LEDPattern *pattern, bool immediate)
{
    sequencer.setPattern(pattern, immediate);
}

IRQ_HANDLER ISR_FN(LED_SEQUENCER_TIM)()
{
    const HwTimer pwmTimer(&LED_PWM_TIM);
    const HwTimer seqTimer(&LED_SEQUENCER_TIM);

    seqTimer.clearStatus();

    LEDSequencer::LEDState state;
    LED::sequencer.tickISR(state);

    pwmTimer.setDuty(LED_PWM_RED_CHAN, state.red);
    pwmTimer.setDuty(LED_PWM_GREEN_CHAN, state.green);
}
