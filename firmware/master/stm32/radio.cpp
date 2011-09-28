/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * This file is part of the internal implementation of the Sifteo SDK.
 * Confidential, not for redistribution.
 *
 * Copyright <c> 2011 Sifteo, Inc. All rights reserved.
 */

#include "radio.h"
#include "hardware.h"

void Radio::open()
{
}

void Radio::halt()
{
    GPIOA.ODR ^= (1 << 4);
}
