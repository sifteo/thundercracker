/*
 * Thundercracker Firmware -- Confidential, not for redistribution.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#include "rctimer.h"

/*
    Set our hardware timer up as an input - period doesn't much matter since
    we only want to interrupt on capture.
*/
void RCTimer::init()
{
    pin.setControl(GPIOPin::OUT_2MHZ);
    pin.setLow();

    timer.init(0xFFF, 0);

    timer.disableChannel(timerChan);
    timer.configureChannelAsInput(timerChan, HwTimer::FallingEdge);
    timer.enableCompareCaptureIsr(timerChan);
    timer.enableChannel(timerChan);
}

/*
    Set the pin high, and then measure how long it takes to generate a
    falling edge.
*/
void RCTimer::startSample()
{
    // hold high during charge time
    pin.setControl(GPIOPin::OUT_2MHZ);
    pin.setHigh();
    // TODO: no busy waiting! should probably set timer duration, and start
    // counting from within the interrupt on update
    for (volatile unsigned i = 0; i < 250; ++i) {
        ;
    }

    pin.setControl(GPIOPin::IN_FLOAT);
    startTime = timer.count();
}

void RCTimer::isr()
{
    timer.clearStatus();

    // XXX: running average, or other smoothing?
    reading = timer.lastCapture(timerChan) - startTime;
}
