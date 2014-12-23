/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * Thundercracker firmware
 *
 * Copyright <c> 2012 Sifteo, Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#ifndef HWTIMER_H_
#define HWTIMER_H_

#include "hardware.h"
#include "macros.h"

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

    enum Event {
        UpdateEvent     = 1 << 0,
        CC1GEvent       = 1 << 1,
        CC2GEvent       = 1 << 2,
        CC3GEvent       = 1 << 3,
        CC4GEvent       = 1 << 4,
        TriggerEvent    = 1 << 6
    };

    enum MasterModeSelect {
        ResetTrigger    = 0 << 4,
        EnableTrigger   = 1 << 4,
        UpdateTrigger   = 2 << 4,
    };

    ALWAYS_INLINE HwTimer(volatile TIM_t *_hw) :
        tim(_hw) {}

    void init(uint16_t period, uint16_t prescaler) const;
    void deinit() const;

    uint16_t ALWAYS_INLINE status() const {
        return (tim->SR & tim->DIER);
    }
    void ALWAYS_INLINE clearStatus() const {
        tim->SR = 0;
    }

    void configureChannelAsOutput(int ch, Polarity p, TimerMode timmode, OutputMode outmode = SingleOutput, DmaMode dmamode = DmaDisabled) const;
    void configureChannelAsInput(int ch, InputCaptureEdge edge, uint8_t filterFreq = 0, uint8_t prescaler = 0) const;

    void ALWAYS_INLINE configureTriggerOutput(MasterModeSelect mms = UpdateTrigger) const {
        tim->CR2 = mms;
    }

    void ALWAYS_INLINE enableChannel(int ch) const {
        tim->SR &= ~(1 << ch);  // CCxIF bits start at 1, so no need to subtract from 1-based channel num
        tim->CCER |= 1 << ((ch - 1) * 4);
    }

    void ALWAYS_INLINE disableChannel(int ch) const {
        tim->CCER &= ~(0x1 << ((ch - 1) * 4));
    }

    bool ALWAYS_INLINE channelIsEnabled(int ch) const {
        return (tim->CCER & (1 << ((ch - 1) * 4))) != 0;
    }

    /*
     * NOTE: the complementary output control routines below do not work for
     * channel 4, since its layout in CCER is irregular. Special case it
     * if we need it.
     */
    void ALWAYS_INLINE enableComplementaryOutput(int ch) const {
        tim->CCER |= (1 << ((ch-1 * 4) + 2));
    }
    void ALWAYS_INLINE disableComplementaryOutput(int ch) const {
        tim->CCER &= ~(1 << ((ch-1 * 4) + 2));
    }
    void ALWAYS_INLINE invertComplementaryOutput(int ch) const {
        tim->CCER ^= (1 << ((ch-1 * 4) + 3));
    }

    void ALWAYS_INLINE enableCompareCaptureIsr(int ch) const {
        tim->SR &= ~(1 << ch);  // clear pending ISR status
        tim->DIER |= (1 << ch);
    }
    void ALWAYS_INLINE disableCompareCaptureIsr(int ch) const {
        tim->DIER &= ~(1 << ch);
    }
    void ALWAYS_INLINE enableUpdateIsr() const {
        tim->SR &= ~(1 << 0);   // clear pending ISR status
        tim->DIER |= (1 << 0);
    }
    void ALWAYS_INLINE disableUpdateIsr() const {
        tim->DIER &= ~(1 << 0);
    }

    uint16_t ALWAYS_INLINE lastCapture(int ch) const {
        return tim->compareCapRegs[ch - 1].CCR;
    }

    uint16_t ALWAYS_INLINE count() const {
        return tim->CNT;
    }

    void ALWAYS_INLINE setCount(uint16_t c) const {
        tim->CNT = c;
    }

    uint16_t ALWAYS_INLINE period() const {
        return tim->ARR;
    }

    void ALWAYS_INLINE setPeriod(uint16_t period, uint16_t prescaler) const {
        tim->ARR = period;
        tim->PSC = prescaler;
    }

    void ALWAYS_INLINE setDuty(int ch, uint16_t duty) const {
        tim->compareCapRegs[ch - 1].CCR = duty;
    }

    void ALWAYS_INLINE generateEvent(uint16_t mask) const {
        tim->EGR = mask;
    }

    void setDutyDma(int ch, const uint16_t *data, uint16_t len) const;

private:
    volatile TIM_t *tim;
};

#endif /* HWTIMER_H_ */
