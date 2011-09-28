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


class SPIMaster {
 public:
    SPIMaster(volatile SPI_t *_hw,
	      GPIOPin _csn,
	      GPIOPin _sck,
	      GPIOPin _miso,
	      GPIOPin _mosi)
	: hw(_hw), csn(_csn), sck(_sck), miso(_miso), mosi(_mosi) {}

    void init() {
	csn.setHigh();
	csn.setControl(GPIOPin::OUT_10MHZ);
	sck.setControl(GPIOPin::OUT_ALT_10MHZ);
	miso.setControl(GPIOPin::IN_FLOAT);    
	mosi.setControl(GPIOPin::OUT_ALT_10MHZ);

	hw->CR1 = 0x0004;	// Master mode
	hw->CR2 = 0x0004;	// SS output enable
	hw->CR1 |= 0x0040;	// Peripheral enable
    }

    void begin() {
	csn.setLow();
    }

    void end() {
	csn.setHigh();
    }

    uint8_t transfer(uint8_t b) {
	/*
	 * XXX: This is slow, ugly, and power hungry. We should be
	 *      doing DMA, and keeping the FIFOs full! And NOT
	 *      busy-looping ever!
	 */
	hw->DR = b;
	while (!(hw->SR & 1));	// Wait for RX-not-empty
	return hw->DR;
    }
    
 private:
    volatile SPI_t *hw;
    GPIOPin csn;
    GPIOPin sck;
    GPIOPin miso;
    GPIOPin mosi;
};

#endif
