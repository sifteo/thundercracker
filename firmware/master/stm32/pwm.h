/*
 * This file is part of the internal implementation of the Sifteo SDK.
 * Confidential, not for redistribution.
 *
 * Copyright <c> 2011 Sifteo, Inc. All rights reserved.
 */

#ifndef _STM32_TIMER_H
#define _STM32_TIMER_H

#include "hardware.h"
#include "gpio.h"

class Pwm {
public:
    Pwm(volatile TIM2_5_t *_hw, GPIOPin _output);

    enum Polarity {
        ActiveHigh,
        ActiveLow
    };

    void init(int freq, int period);
    void end();

    void enableChannel(int ch, enum Polarity p, int duty);
    void disableChannel(int ch);

    int period() const;
    void setPeriod(int period);

    void setDuty(int ch, int duty);

private:
    volatile TIM2_5_t *tim;
    GPIOPin output;
};

extern Pwm testTmr;

#endif // _STM32_TIMER_H
