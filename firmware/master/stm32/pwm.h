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
    Pwm(volatile TIM_t *_hw, GPIOPin _output, GPIOPin _complementaryOutput) :
        tim(_hw), output(_output), complementaryOutput(_complementaryOutput)
    {}

    enum Polarity {
        ActiveHigh,
        ActiveLow
    };

    enum DmaMode {
        DmaDisabled,
        DmaEnabled
    };

    enum OutputMode {
        SingleOutput,
        ComplementaryOutput
    };

    void init(int period, int prescaler);
    void end();

    void enableChannel(int ch, enum Polarity p, enum OutputMode outmode = SingleOutput, enum DmaMode dmamode = DmaDisabled);
    void disableChannel(int ch);

    int period() const;
    void setPeriod(int period);

    void setDuty(int ch, int duty);
    void setDutyDma(int ch, const uint16_t *data, uint16_t len);

private:
    volatile TIM_t *tim;
    GPIOPin output;
    GPIOPin complementaryOutput;

    static void staticDmaHandler(void *p, uint32_t flags);
    void dmaHandler(uint32_t flags);
    bool getDmaDetails(int pwmchan, volatile DMA_t **dma, int *dmaChannel);
};

#endif // _STM32_TIMER_H
