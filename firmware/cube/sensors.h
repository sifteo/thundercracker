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

extern uint8_t accel_state;

#define ACCEL_I2C_ADDR  0x2A

/*
 * Assembly macro: Trigger an accelerometer read by writing the
 * first byte, and resetting the accelerometer state machine. Only
 * do this if the accelerometer state machine is idle.
 */

#pragma sdcc_hash +
#define ACCEL_BEGIN_POLL(lbl)                           __endasm ; \
        __asm   mov     a, _accel_state                 __endasm ; \
        __asm   jnz     lbl                             __endasm ; \
        __asm   mov     _accel_state, #0                __endasm ; \
        __asm   mov     _W2DAT, #ACCEL_I2C_ADDR         __endasm ; \
        __asm   lbl:                                    __endasm ; \
        __asm

#endif
