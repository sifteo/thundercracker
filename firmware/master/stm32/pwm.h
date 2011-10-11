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

    enum DmaMode {
        DmaDisabled,
        DmaEnabled
    };

    void init(int freq, int period);
    void end();

    void enableChannel(int ch, enum Polarity p, int duty, enum DmaMode dmamode = DmaDisabled);
    void disableChannel(int ch);

    int period() const;
    void setPeriod(int period);

    void setDuty(int ch, int duty);
    void setDutyDma(int ch, const uint16_t *data, uint16_t len);

private:
    volatile TIM2_5_t *tim;
    GPIOPin output;

    static void staticDmaHandler(void *p, uint32_t flags);
    void dmaHandler(uint32_t flags);
    bool getDmaDetails(int pwmchan, volatile DMA_t **dma, int *dmaChannel);
};

#endif // _STM32_TIMER_H
