/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * Thundercracker cube firmware
 *
 * M. Elizabeth Scott <beth@sifteo.com>
 * Copyright <c> 2011 Sifteo, Inc. All rights reserved.
 */

#ifndef _BATTERY_H
#define _BATTERY_H

#include "hardware.h"

// Battery poller is trying to acquire exclusive access to the A/D converter
extern volatile __bit battery_adc_lock;

void battery_poll();

#endif
