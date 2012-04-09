/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * This file is part of the internal implementation of the Sifteo SDK.
 * Confidential, not for redistribution.
 *
 * Copyright <c> 2011 Sifteo, Inc. All rights reserved.
 */

#ifndef _STM32_I2C_H
#define _STM32_I2C_H

#include <stdint.h>
#include "hardware.h"
#include "gpio.h"

class I2CSlave {
public:
    I2CSlave (volatile I2C_t *_hw)
        : hw(_hw) {}

    void init(GPIOPin scl, GPIOPin sda);

    void isr_EV();
    void isr_ER();

    void setNextTest(uint8_t test);

private:
    volatile I2C_t *hw;

    uint8_t nextTest;
    uint8_t Address;
    uint32_t I2CDirection;
};

#endif
