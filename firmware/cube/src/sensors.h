/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * Thundercracker cube firmware
 *
 * M. Elizabeth Scott <beth@sifteo.com>
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#ifndef _SENSORS_H
#define _SENSORS_H

#include "cube_hardware.h"

void sensors_init();

/*
 * We export a global tick counter, which can be used by other modules
 * who need a low-frequency timebase.
 *
 * This is currently used by battery voltage polling, and LCD delays.
 */
extern volatile uint8_t sensor_tick_counter;
extern volatile uint8_t sensor_tick_counter_high;

#ifdef DEBUG_NBR
	extern uint8_t __idata nbr_data[4];
	extern uint8_t	nbr_temp;
	extern uint8_t __idata nbr_data_valid[2];
	extern uint8_t __idata nbr_data_invalid[2];
#endif

extern __bit touch;
#ifdef DEBUG_TOUCH
    extern uint8_t touch_count;
#endif

/*
 * Interrupt declarations must be visible in main.c
 */
void spi_i2c_isr(void) __interrupt(VECTOR_SPI_I2C) __naked;
void tf0_isr(void) __interrupt(VECTOR_TF0) __naked;
void tf1_isr(void) __interrupt(VECTOR_TF1) __naked;
void tf2_isr(void) __interrupt(VECTOR_TF2) __naked;

// To change A21, set the target and call i2c_a21_wait().
extern __bit i2c_a21_target;
void i2c_a21_wait(void);


#endif
