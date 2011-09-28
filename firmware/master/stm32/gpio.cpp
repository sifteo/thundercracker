/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * This file is part of the internal implementation of the Sifteo SDK.
 * Confidential, not for redistribution.
 *
 * Copyright <c> 2011 Sifteo, Inc. All rights reserved.
 */

#include "gpio.h"

void GPIOPin::setControl(Control c) const {
    unsigned p = pin();
    volatile uint32_t *ptr = &port()->CRL;
    
    if (p >= 8) {
	p -= 8;
	ptr++;
    }

    unsigned shift = p << 2;
    unsigned mask = 0xF << shift;

    *ptr = (*ptr & ~mask) | (c << shift);
}
