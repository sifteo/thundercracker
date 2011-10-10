/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * Thundercracker cube firmware
 *
 * M. Elizabeth Scott <beth@sifteo.com>
 * Copyright <c> 2011 Sifteo, Inc. All rights reserved.
 */

#ifndef _SENSORS_H
#define _SENSORS_H

#include "hardware.h"

void sensors_init();

void spi_i2c_isr(void) __interrupt(VECTOR_SPI_I2C) __naked;
void tf0_isr(void) __interrupt(VECTOR_TF0) __naked;
void tf1_isr(void) __interrupt(VECTOR_TF1) __naked;
void tf2_isr(void) __interrupt(VECTOR_TF2) __naked;

#endif
