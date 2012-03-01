/*
 * Thundercracker Firmware -- Confidential, not for redistribution.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#ifndef RCTIMER_H
#define RCTIMER_H

#include "gpio.h"
#include "hwtimer.h"

class RCTimer
{
public:
    RCTimer(HwTimer hwtimer, int channel, GPIOPin gpio) :
        timer(hwtimer),
        timerChan(channel),
        pin(gpio),
        startTime(0),
        reading(0)
    {}

    void init();

    void startSample();
    void isr();
    uint16_t lastReading() const {
        return reading;
    }

private:
    HwTimer timer;
    int timerChan;
    GPIOPin pin;
    uint16_t startTime;
    uint16_t reading;
};

#endif // RCTIMER_H
