/*
 * Thundercracker Firmware -- Confidential, not for redistribution.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#ifndef _STM32_SWD_H
#define _STM32_SWD_H

#include "hwtimer.h"
#include "gpio.h"

class SWDMaster {
public:
    void init();
    void start();
    void stop();

    void isr();
    void controller();

    SWDMaster(volatile TIM_t *_tim,
              GPIOPin _swdclk,
              GPIOPin _swdio)
        : swdTimer(_tim), swdclk(_swdclk), swdio(_swdio){}
private:
    HwTimer swdTimer;
    GPIOPin swdclk;
    GPIOPin swdio;

    enum swdState {
        rtz = 0,
        idle,
        wkp,
        req,
        ack,
        dat
    };
    swdState state;

    void ALWAYS_INLINE clk_toggle();
};

#endif // _STM32_SWD_H
