/*
 * Thundercracker Firmware -- Confidential, not for redistribution.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
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

    enum InputCaptureEdge {
        RisingEdge          = 0,
        FallingEdge         = 1 << 1
    };

    HwTimer(volatile TIM_t *_hw) :
        tim(_hw) {}

    void init(uint16_t period, uint16_t prescaler);
    void deinit();

    uint16_t status() const {
        return tim->SR;
    }
    void clearStatus() {
        tim->SR = 0;
    }

    void configureChannelAsOutput(int ch, Polarity p, TimerMode timmode, OutputMode outmode = SingleOutput, DmaMode dmamode = DmaDisabled);
    void configureChannelAsInput(int ch, InputCaptureEdge edge, uint8_t filterFreq = 0, uint8_t prescaler = 0);

    void enableChannel(int ch) {
        tim->SR &= ~(1 << ch);  // CCxIF bits start at 1, so no need to subtract from 1-based channel num
        tim->CCER |= 1 << ((ch - 1) * 4);
    }

    void disableChannel(int ch) {
        tim->CCER &= ~(0x1 << ((ch - 1) * 4));
    }

    bool channelIsEnabled(int ch) {
        return (tim->CCER & (1 << ((ch - 1) * 4))) != 0;
    }

    void enableCompareCaptureIsr(int ch) {
        tim->SR &= ~(1 << ch);  // clear pending ISR status
        tim->DIER |= (1 << ch);
    }
    void disableCompareCaptureIsr(int ch) {
        tim->DIER &= ~(1 << ch);
    }
    void enableUpdateIsr() {
        tim->SR &= ~(1 << 0);   // clear pending ISR status
        tim->DIER |= (1 << 0);
    }
    void disableUpdateIsr() {
        tim->DIER &= ~(1 << 0);
    }

    uint16_t lastCapture(int ch) const {
        return tim->CCR[ch - 1];
    }

    uint16_t count() const {
        return tim->CNT;
    }

    void setCount(uint16_t c) {
        tim->CNT = c;
    }

    uint16_t period() const {
        return tim->ARR;
    }

    void setPeriod(uint16_t period, uint16_t prescaler) {
        tim->ARR = period;
        tim->PSC = prescaler;
    }

    void setDuty(int ch, uint16_t duty) {
        tim->CCR[ch - 1] = duty;
    }

    void setDutyDma(int ch, const uint16_t *data, uint16_t len);

private:
    volatile TIM_t *tim;
};

#endif /* HWTIMER_H_ */
