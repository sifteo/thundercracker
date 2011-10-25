/*
 * This file is part of the internal implementation of the Sifteo SDK.
 * Confidential, not for redistribution.
 *
 * Copyright <c> 2011 Sifteo, Inc. All rights reserved.
 */

#ifndef HWTIMER_H_
#define HWTIMER_H_

#include "hardware.h"

class HwTimer
{
public:
    enum Polarity {
        ActiveHigh,
        ActiveLow
    };

    // see TIMx_CCMR1->OCxM
    enum TimerMode {
        Frozen,
        ActiveOnMatch,
        InactiveOnMatch,
        ToggleOnMatch,
        ForceActive,
        ForceInactive,
        Pwm1,
        Pwm2
    };

    enum DmaMode {
        DmaDisabled,
        DmaEnabled
    };

    enum OutputMode {
        SingleOutput,
        ComplementaryOutput
    };

    HwTimer(volatile TIM_t *_hw) :
        tim(_hw) {}

    void init(int period, int prescaler);
    void setUpdateIsrEnabled(bool enabled);
    void end();

    void configureChannel(int ch, Polarity p, TimerMode timmode, OutputMode outmode = SingleOutput, DmaMode dmamode = DmaDisabled);
    void enableChannel(int ch);
    void disableChannel(int ch);
    bool channelIsEnabled(int ch);

    int period() const;
    void setPeriod(int period, int prescaler);

    void setDuty(int ch, int duty);
    void setDutyDma(int ch, const uint16_t *data, uint16_t len);

private:
    volatile TIM_t *tim;
};

#endif /* HWTIMER_H_ */
