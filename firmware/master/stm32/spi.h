/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * This file is part of the internal implementation of the Sifteo SDK.
 * Confidential, not for redistribution.
 *
 * Copyright <c> 2011 Sifteo, Inc. All rights reserved.
 */

#ifndef _STM32_SPI_H
#define _STM32_SPI_H

#include <stdint.h>
#include "hardware.h"
#include "gpio.h"


class SPIMaster {
 public:
    SPIMaster(volatile SPI_t *_hw,
              GPIOPin _csn,
              GPIOPin _sck,
              GPIOPin _miso,
              GPIOPin _mosi)
        : hw(_hw), csn(_csn), sck(_sck), miso(_miso), mosi(_mosi) {}

    void init();
    void begin();
    void end();

    uint8_t transfer(uint8_t b);
    void transferTable(const uint8_t *table);
  
 private:
    volatile SPI_t *hw;
    GPIOPin csn;
    GPIOPin sck;
    GPIOPin miso;
    GPIOPin mosi;
};

#endif
