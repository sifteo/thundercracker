/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * Sifteo prototype simulator
 * M. Elizabeth Scott <beth@sifteo.com>
 *
 * Copyright <c> 2011 Sifteo, Inc. All rights reserved.
 */

/*
 * This file implements a single I2C master. It's simpler than the SPI
 * master, since our I2C peripheral only runs at 100 kHz. There is no
 * multi-level transmit/receive FIFO, we just know how to write
 * individual bytes, start/stop conditions, and fire interrupts.
 *
 * Our interface to the I2C peripheral device is via global functions
 * i2cbus_*, currently defined directly by our accelerometer
 * emulation, since we have no other devices on the bus.
 */

#ifndef _I2C_H
#define _I2C_H

#include "emu8051.h"

void i2c_init();
int i2c_tick(struct em8051 *cpu);
void i2c_write_data(struct em8051 *cpu, uint8_t data);
uint8_t i2c_read_data(struct em8051 *cpu);
uint8_t i2c_read_con1(struct em8051 *cpu);
uint8_t i2c_trace();

void i2cbus_start();
void i2cbus_stop();
void i2cbus_write(uint8_t byte);
uint8_t i2cbus_read(uint8_t ack);
 
#endif
