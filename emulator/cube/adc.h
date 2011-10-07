/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * Sifteo prototype simulator
 * M. Elizabeth Scott <beth@sifteo.com>
 *
 * Copyright <c> 2011 Sifteo, Inc. All rights reserved.
 */

#ifndef _ADC_H
#define _ADC_H

#include <stdint.h>

void adc_init(void);
void adc_start(void);
int adc_tick(uint8_t *regs);

void adc_set_input(int index, uint16_t value16);
 
#endif
