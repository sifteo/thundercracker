/*
 * Thundercracker Firmware -- Confidential, not for redistribution.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#ifndef _STM32_GPIO_H
#define _STM32_GPIO_H

#include "macros.h"
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

    ALWAYS_INLINE GPIOPin(volatile GPIO_t *port, unsigned bit)
        : value(bit | (uintptr_t)port) {}

    ALWAYS_INLINE void setHigh() const {
        port()->BSRR = bit();
    }

    ALWAYS_INLINE void setLow() const {
        port()->BRR = bit();
    }

    /*
        Pull up and pull down are controlled via ODR when the pin
        is configured as an input.
    */
    ALWAYS_INLINE void pullup() const {
        port()->ODR |= bit();
    }

    ALWAYS_INLINE void pulldown() const {
        port()->ODR &= ~bit();
    }

    ALWAYS_INLINE void toggle() const {
        port()->ODR ^= bit();
    }

    ALWAYS_INLINE bool isHigh() const {
        return port()->IDR & bit();
    }

    ALWAYS_INLINE bool isLow() const {
        return (port()->IDR & bit()) == 0;
    }

    void setControl(Control c) const;
    void irqInit() const;

    ALWAYS_INLINE void irqSetRisingEdge() const {
        EXTI.RTSR |= bit();
    }

    ALWAYS_INLINE void irqSetFallingEdge() const {
        EXTI.FTSR |= bit();
    }

    ALWAYS_INLINE void irqEnable() const {
        EXTI.IMR |= bit();
    }

    ALWAYS_INLINE void irqDisable() const {
        EXTI.IMR &= ~bit();
    }   

    ALWAYS_INLINE void irqAcknowledge() const {
        // Write 1 to clear
        EXTI.PR = bit();
    }

    ALWAYS_INLINE bool irqPending() const {
        return (EXTI.PR & bit()) != 0;
    }

    ALWAYS_INLINE void softwareInterrupt() const {
        EXTI.SWIER |= bit();
    }

    ALWAYS_INLINE volatile GPIO_t *port() const {
        return (volatile GPIO_t*)(0xFFFFFF00 & value);
    }

    ALWAYS_INLINE unsigned portIndex() const {
        // PA=0, PB=1, ...
        return (value - (uintptr_t)&GPIOA) >> 10;
    }

    ALWAYS_INLINE unsigned pin() const {
        return 15 & value;
    }

    ALWAYS_INLINE unsigned bit() const {
        return 1 << pin();
    }

    ALWAYS_INLINE static void setControl(volatile GPIO_t *port, unsigned bit, Control c)
    {
        /*
         * Change this pin's control nybble
         */

        volatile uint32_t *ptr = &port->CRL;

        if (bit >= 8) {
            bit -= 8;
            ptr++;
        }

        unsigned shift = bit << 2;
        unsigned mask = 0xF << shift;

        *ptr = (*ptr & ~mask) | (c << shift);
    }

 private:
    const uintptr_t value;
};

#endif
