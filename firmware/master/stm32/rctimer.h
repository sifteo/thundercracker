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

#ifndef RCTIMER_H
#define RCTIMER_H

#include "gpio.h"
#include "hwtimer.h"


class RCTimer
{
public:
    RCTimer(HwTimer hwtimer, int channel, GPIOPin gpio) :
        filterState(0),
        startTime(0),
        timer(hwtimer),
        pin(gpio),
        timerChan(channel)
    {}

    void init(uint16_t period, uint16_t prescaler);
    void isr();

    ALWAYS_INLINE int lastReading() const {
        // Units are arbitrary, and depend on FILTER_SHIFT
        return filterState;
    }

private:
    // Logarithmic, higher values = slower filter
    static const int FILTER_SHIFT = 8;

    // Initial value for filterState
    static const int INIT_FILTER_STATE = 0x7fffffff;

    int filterState;
    uint16_t startTime;
    HwTimer timer;
    GPIOPin pin;
    const uint8_t timerChan;

    void beginSample();
};


/*
 * Set our hardware timer up as an input - period doesn't much matter since
 * we only want to interrupt on capture.
 */
inline void RCTimer::init(uint16_t period, uint16_t prescaler)
{
    // Reset the filter
    filterState = INIT_FILTER_STATE;

    pin.setControl(GPIOPin::OUT_2MHZ);
    pin.setLow();

    timer.init(period, prescaler);

    timer.disableChannel(timerChan);
    timer.configureChannelAsInput(timerChan, HwTimer::FallingEdge);
    timer.enableUpdateIsr();
    timer.enableCompareCaptureIsr(timerChan);
    timer.enableChannel(timerChan);
}


/*
 * Set the pin high, and then measure how long it takes to generate a
 * falling edge.
 */
ALWAYS_INLINE void RCTimer::beginSample()
{
    // charge the cap
    pin.setControl(GPIOPin::OUT_2MHZ);
    pin.setHigh();

    /*
     * The cap charges effectively instantaneously since the RC is only
     * on the discharge portion of the circuit. No need to wait
     * around for any additional charge time.
     */

    startTime = timer.count();
    pin.setControl(GPIOPin::IN_FLOAT);
}


ALWAYS_INLINE void RCTimer::isr()
{
    uint32_t status = timer.status();
    timer.clearStatus();

    // update event - time to start a new sample
    if (status & 0x1) {
        beginSample();
    }

    // capture event
    if (status & (1 << timerChan)) {
        int sample = timer.lastCapture(timerChan) - startTime;

        // keep this pin low between samples to avoid power draw
        pin.setLow();
        pin.setControl(GPIOPin::OUT_2MHZ);

        /*
         * Update our low-pass filter. This is a really simple and fast-to-calculate
         * first order filter, based on a power of two gain. We use a special sentinel
         * value to indicate that the filter state is uninitialized, so that we can
         * initialize it the first time.
         *
         * (Otherwise, the filter would ramp up slowly from zero at poweron, leading
         * us to miss the beginning of the power-on sound)
         */

        int state = filterState;
        if (state == INIT_FILTER_STATE)
            filterState = sample << FILTER_SHIFT;
        else
            filterState = state + sample - (filterState >> FILTER_SHIFT);
    }
}

#endif // RCTIMER_H
