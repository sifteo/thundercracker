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

#include "batterylevel.h"
#include "rctimer.h"
#include "board.h"
#include "neighbor_tx.h"
#include "neighbor_protocol.h"
#include "powermanager.h"
#include "macros.h"

#include <sifteo/abi.h>

#ifdef USE_RC_BATT_MEAS

/*
 * BatteryLevel measurement via RC timer.
 * Only used on rev2 and earlier, since we could not
 * run the ADC at 2V.
 *
 * Later hardware revs make use of batterylevel_adc.cpp
 */

namespace BatteryLevel {

static unsigned lastReading;

enum State {
    VBattCapture,
    VSysCapture,
};

static unsigned lastVsysReading;
static State currentState;

/*
 * As we're sharing a timer with NeighborTX, the end of each neighbor transmission
 * is an opportunity for us to begin a sample. Lets wait 2.5 seconds between
 * each sample. The minimum amount of wait time is 75 ms (DELAY_PRESCALER ~= 25)
 *
 * 2500 / (NUM_TX_WAIT_PERIODS * BIT_PERIOD_TICKS * TICK) ~= 833
 *
 * See neighbor_protocol.h for neighbor transmission periods.
 */

static const unsigned DELAY_PRESCALER = 833;
// no delay for first sample - want to capture initial value immediately
static unsigned delayPrescaleCounter = DELAY_PRESCALER;

/*
 * Max discharge time is 3ms.
 * To configure our timer at this resolution, we need to use the maximum
 * period (0xffff), and select a prescaler that gets us as close as possible.
 *
 * 0.05 / ((1 / 36000000) * 0xffff) == 27.466
 */
static const unsigned DISCHARGE_PRESCALER = 6;



void init()
{
    /*
     * Setting lastReading for comparison on startup
     */
    lastReading = UNINITIALIZED;
    lastVsysReading = UNINITIALIZED;

    /*
     * Take a VBatt sample first, since its charge time is much longer than
     * the VSys charge time.
     */
    currentState = VBattCapture;

    /*
     * BATT_MEAS can always remain configured as an input.
     * To charge (default state), BATT_MEAS_GND => input/float.
     * To discharge, BATT_MEAS_GND => driven low and timer capture on BATT_MEAS.
     */

    BATT_MEAS_GPIO.setControl(GPIOPin::IN_FLOAT);

    GPIOPin battMeasGnd = BATT_MEAS_GND_GPIO;
    battMeasGnd.setControl(GPIOPin::IN_FLOAT);
    battMeasGnd.setLow();

    /*
     * We share this timer with NeighborTX - its alrady been init'd there.
     * Just configure our channel, but don't enable it's ISR yet.
     */

    HwTimer timer(&BATT_LVL_TIM);
    timer.configureChannelAsInput(BATT_LVL_CHAN, HwTimer::FallingEdge);
    timer.enableChannel(BATT_LVL_CHAN);
}

unsigned raw()
{
    return lastReading;
}

unsigned vsys()
{
    return lastVsysReading;
}

unsigned scaled()
{
    /*
     * We assume a linear profile
     * Must calibrate on the assembly line to find the range
     * It is hardcoded for now.
     */
    const unsigned RANGE = 3250;
    unsigned minL, maxL;
    if (PowerManager::state() == PowerManager::BatteryPwr) {
        minL = lastVsysReading;
        maxL = MAX(minL + RANGE, minL);
    } else {
        // We are on usb power
        maxL = lastVsysReading;
        minL = MIN(maxL - RANGE, maxL);
    }

    unsigned clamped = clamp(lastReading, minL, maxL);
    return (clamped - minL) * _SYS_BATTERY_MAX / RANGE;
}

void beginCapture()
{
    /*
     * An opportunity to take a new sample.
     *
     * First, detect whether a battery is connected: BATT_MEAS will be high.
     *
     * BATT_LVL_TIM is shared with neighbor transmit
     */

    if (currentState == VSysCapture || delayPrescaleCounter++ == DELAY_PRESCALER) {

        delayPrescaleCounter = 0;

        NeighborTX::pause();

        HwTimer timer(&BATT_LVL_TIM);
        timer.setPeriod(0xffff, DISCHARGE_PRESCALER);
        /*
         * We need to generate an update event in order to latch in the new
         * prescaler - otherwise it would only be applied at the next
         * update event.
         *
         * This also has the nice side effect of resetting the counter,
         * so we don't need to track the start time.
         */
        timer.generateEvent(HwTimer::UpdateEvent);

        BATT_MEAS_GND_GPIO.setControl(GPIOPin::OUT_2MHZ);
        BATT_MEAS_GND_GPIO.setLow();

        /*
         * If the current state is the calibration state
         * then BATT_MEAS_GPIO was set as an output
         * in the captureISR
         */
        if (currentState == VSysCapture) {
            BATT_MEAS_GPIO.setControl(GPIOPin::IN_FLOAT);
        }

        /*
         * If the battery is too low (< min(Vih)) or not present,
         * we dont bother to run the timer and set input capture
         * to zero (as though the capacitor drained out immediately)
         */
        if (BATT_MEAS_GPIO.isLow()) {
            process(0);
        } else {
            timer.enableCompareCaptureIsr(BATT_LVL_CHAN);
        }
    }
}

void captureIsr()
{
    /*
     * Called from within NeighborTX, where the interrupts for the
     * timer we're on are handled.
     *
     * Once our discharge has completed, we can resume normal neighbor transmission.
     */
    HwTimer timer(&BATT_LVL_TIM);
    timer.disableCompareCaptureIsr(BATT_LVL_CHAN);

    unsigned capture = timer.lastCapture(BATT_LVL_CHAN);
    process(capture);
}


void process(unsigned capture)
{
    /*
     * We alternately sample VSYS and VBATT in order to establish a consistent
     * baseline - store the capture appropriately.
     */

    BATT_MEAS_GND_GPIO.setControl(GPIOPin::IN_FLOAT);

    if (currentState == VBattCapture) {

        lastReading = capture;

        BATT_MEAS_GPIO.setControl(GPIOPin::OUT_2MHZ);
        BATT_MEAS_GPIO.setHigh();

        currentState = VSysCapture;

    } else if (currentState == VSysCapture) {

        lastVsysReading = capture;

        /*
         * Check and take action if we are on battery power
         */
        if (PowerManager::state() == PowerManager::BatteryPwr) {
            PowerManager::shutdownIfVBattIsCritical(lastReading, lastVsysReading);
            BatteryLevel::onCapture(); // see in common folder
        }

        currentState = VBattCapture;
    }

    NeighborTX::resume();
}

} // namespace BatteryLevel

#endif // USE_RC_BATT_MEAS
