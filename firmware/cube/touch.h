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

#define my_idata

union ts_word {
	uint16_t val;
	struct {
		uint8_t valL, valH;
	};
};

my_idata extern volatile union ts_word ts_prev_touchval;
my_idata extern volatile union ts_word ts_touchval;
extern volatile __bit ts_dataReady;

void touch_init();

#ifdef TOUCH_STANDALONE
void timer_isr(void) __interrupt(VECTOR_TF0);
#endif

void adc_isr(void) __interrupt(VECTOR_MISC);

#endif
