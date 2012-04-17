/*
 * Thundercracker Firmware -- Confidential, not for redistribution.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#include "rctimer.h"

/*
 * Set our hardware timer up as an input - period doesn't much matter since
 * we only want to interrupt on capture.
*/
void RCTimer::init()
{
    pin.setControl(GPIOPin::OUT_2MHZ);
    pin.setLow();

    timer.init(0xFFFF, 0);

    timer.disableChannel(timerChan);
    timer.configureChannelAsInput(timerChan, HwTimer::FallingEdge);
    timer.enableCompareCaptureIsr(timerChan);
    timer.enableChannel(timerChan);
}

/*
 * Set the pin high, and then measure how long it takes to generate a
 * falling edge.
*/
void RCTimer::startSample()
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

/*
 * Handle timer interrupt.
 * Should only ever be getting called here on capture, but return true to
 * confirm that we got an updated reading as a result of this ISR.
 */
bool RCTimer::isr()
{
    uint32_t status = timer.status();
    timer.clearStatus();

    // capture event
    if (status & (1 << timerChan)) {
        reading = timer.lastCapture(timerChan) - startTime;

        // keep this pin low between samples to avoid power draw
        pin.setLow();
        pin.setControl(GPIOPin::OUT_2MHZ);
        return true;
    }

    return false;
}
