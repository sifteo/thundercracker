/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * Thundercracker cube firmware
 *
 * Hakim Raja <huckym@sifteo.com>
 * Copyright <c> 2011 Sifteo, Inc. All rights reserved.
 */

#ifndef _TOUCH_H
#define _TOUCH_H

#include "hardware.h"

void touch_init();

void timer_isr(void) __interrupt(VECTOR_TF0);
void adc_isr(void) __interrupt(VECTOR_MISC);

#endif
