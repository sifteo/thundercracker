/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * Thundercracker cube firmware
 *
 * Micah Elizabeth Scott <micah@misc.name>
 * Copyright <c> 2012 Sifteo, Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#ifndef _SENSORS_H
#define _SENSORS_H

#include "cube_hardware.h"

void sensors_init();

/*
 * We export a global tick counter, which can be used by other modules
 * who need a low-frequency timebase.
 */
extern volatile uint8_t sensor_tick_counter;
extern volatile uint8_t sensor_tick_counter_high;

/*
 * Interrupt declarations must be visible in main.c
 */
void spi_i2c_isr(void) __interrupt(VECTOR_SPI_I2C) __naked;
void tf0_isr(void) __interrupt(VECTOR_TF0) __naked;
void tf1_isr(void) __interrupt(VECTOR_TF1) __naked;
void tf2_isr(void) __interrupt(VECTOR_TF2) __naked;

// To change A21, set the target and call i2c_a21_wait().
extern __bit i2c_a21_target;
void i2c_a21_wait(void) __naked;


#endif
