/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * Thundercracker cube firmware
 *
 * M. Elizabeth Scott <beth@sifteo.com>
 * Copyright <c> 2011 Sifteo, Inc. All rights reserved.
 */

#ifndef _ADC_H
#define _ADC_H

#include "hardware.h"

void adc_isr(void) __interrupt(VECTOR_MISC);

extern volatile uint8_t debug_touch;

#endif
