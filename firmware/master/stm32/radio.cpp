/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * This file is part of the internal implementation of the Sifteo SDK.
 * Confidential, not for redistribution.
 *
 * Copyright <c> 2011 Sifteo, Inc. All rights reserved.
 */

/*
 * High-level radio interface. This is the glue between our
 * cross-platform code and the low-level nRF radio driver.
 */

#include "radio.h"
#include "nrf24l01.h"

static NRF24L01 NordicRadio(GPIOPin(&GPIOB, 10),		// CE
			    SPIMaster(&SPI2,			// Bus:
				      GPIOPin(&GPIOB, 12),	//   CSN
				      GPIOPin(&GPIOB, 13),	//   SCK
				      GPIOPin(&GPIOB, 14),	//   MISO
				      GPIOPin(&GPIOB, 15)));	//   MOSI

void Radio::open()
{
    NordicRadio.init();
    NordicRadio.ptxMode();
}

void Radio::halt()
{
    // Wait for any interrupt
    __asm__ __volatile__ ("wfi");
}
