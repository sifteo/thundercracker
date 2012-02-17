/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * This file is part of the internal implementation of the Sifteo SDK.
 * Confidential, not for redistribution.
 *
 * Copyright <c> 2011 Sifteo, Inc. All rights reserved.
 */

#ifndef _STM32_GPIO_H
#define _STM32_GPIO_H

#include <stdint.h>
#include "hardware.h"


class GPIOPin {
 public:
    enum Control {
        IN_ANALOG = 0,
        OUT_10MHZ = 1,
        OUT_2MHZ = 2,
        OUT_50MHZ = 3,
        IN_FLOAT = 4,
        OUT_OPEN_10MHZ = 5,
        OUT_OPEN_2MHZ = 6,
        OUT_OPEN_50MHZ = 7,
        IN_PULL = 8,
        OUT_ALT_10MHZ = 9,
        OUT_ALT_2MHZ = 10,
        OUT_ALT_50MHZ = 11,
        IN_RESERVED = 12,
        OUT_ALT_OPEN_10MHZ = 13,
        OUT_ALT_OPEN_2MHZ = 14,
        OUT_ALT_OPEN_50MHZ = 15,
    };

    GPIOPin(volatile GPIO_t *port, unsigned bit)
        : value(bit + (uintptr_t)port) {}

    void setHigh() const {
        port()->ODR |= bit();
    }

    void setLow() const {
        port()->ODR &= ~bit();
    }

    /*
        Pull up and pull down are controlled via ODR when the pin
        is configured as an input.
    */
    void pullup() const {
        port()->ODR |= bit();
    }

    void pulldown() const {
        port()->ODR &= ~bit();
    }

    void toggle() const {
        port()->ODR ^= bit();
    }

    bool isHigh() const {
        return port()->IDR & bit();
    }

    bool isLow() const {
        return (port()->IDR & bit()) == 0;
    }

    void setControl(Control c) const;
    void irqInit() const;

    void irqSetRisingEdge() const {
        EXTI.RTSR |= bit();
    }

    void irqSetFallingEdge() const {
        EXTI.FTSR |= bit();
    }

    void irqEnable() const {
        EXTI.IMR |= bit();
    }

    void irqDisable() const {
        EXTI.IMR &= ~bit();
    }   

    void irqAcknowledge() const {
        // Write 1 to clear
        EXTI.PR = bit();
    }

    volatile GPIO_t *port() const {
        return (volatile GPIO_t*)(0xFFFFFF00 & value);
    }

    unsigned portIndex() const {
        // PA=0, PB=1, ...
        return (value - (uintptr_t)&GPIOA) >> 10;
    }

    unsigned pin() const {
        return 15 & value;
    }

    unsigned bit() const {
        return 1 << pin();
    }

 private:
    uintptr_t value;
};

#endif
