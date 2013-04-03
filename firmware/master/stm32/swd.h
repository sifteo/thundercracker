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

    bool enableRequest();
    bool readRequest(uint8_t);
    bool writeRequest(uint8_t, uint32_t);

    void isr();

    uint8_t header;
    uint32_t payloadIn;
    uint32_t payloadOut;

    SWDMaster(volatile TIM_t *_tim,
              GPIOPin _swdclk,
              GPIOPin _swdio)
        : swdTimer(_tim), swdclk(_swdclk), swdio(_swdio){}
private:
    HwTimer swdTimer;
    GPIOPin swdclk;
    GPIOPin swdio;

    bool busyFlag;

    bool ALWAYS_INLINE isDAPRead() {
        return (header & (1<<7));
    }

    void ALWAYS_INLINE sendbit(uint32_t& b) {
        if ((b & (1<<31))) {
            swdio.setHigh();
        } else {
            swdio.setLow();
        }
        b <<= 1;
    }

    void ALWAYS_INLINE receivebit(uint32_t& b) {
        // LSB first
        b >>= 1;
        b |= (swdio.isHigh()<<31);
    }

    bool ALWAYS_INLINE isRisingEdge() {
        if (swdclk.isLow()) {
            swdclk.setHigh();
            return true;
        }
        return false;
    }

    void ALWAYS_INLINE fallingEdge() {
        swdclk.setLow();
    }

    void startTimer();
    void stopTimer();

    typedef void (SWDMaster::*ControllerCallback)();

    ControllerCallback controllerCB;
    void txnControllerCB();
    void wkpControllerCB();
};

#endif // _STM32_SWD_H
