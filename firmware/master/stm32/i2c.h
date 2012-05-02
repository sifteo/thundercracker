/*
 * Thundercracker Firmware -- Confidential, not for redistribution.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#ifndef _STM32_I2C_H
#define _STM32_I2C_H

#include <stdint.h>
#include "hardware.h"
#include "gpio.h"

class I2CSlave {
public:
    I2CSlave (volatile I2C_t *_hw) :
        hw(_hw) {}

    enum StatusFlags {
        OverUnderRun    = (1 << 11),
        TxEmpty         = (1 << 7),
        RxNotEmpty      = (1 << 6),
        StopBit         = (1 << 4),
        AddressMatch    = (1 << 1)
    };

    void init(GPIOPin scl, GPIOPin sda, uint8_t address);

    // return a StatusFlags compatible mask
    uint16_t irqEvStatus() const {
        return hw->SR1;
    }

    void isrEV(uint8_t *byte);
    void isrER();

private:
    volatile I2C_t *hw;
};

#endif
