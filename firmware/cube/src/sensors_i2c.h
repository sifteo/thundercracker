/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * Thundercracker cube firmware
 *
 * M. Elizabeth Scott <beth@sifteo.com>
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

/*
 * Internal declarations for I2C-attached sensors
 * (Accelerometer, factory test)
 */

#ifndef _SENSORS_I2C_H
#define _SENSORS_I2C_H

/*
 * LIS3DH accelerometer.
 */

#define ACCEL_ADDR_TX           0x30    // 00110010 - SDO is tied LOW
#define ACCEL_ADDR_RX           0x31    // 00110011 - SDO is tied LOW

#define ACCEL_CTRL_REG1         0x20
#define ACCEL_REG1_INIT         0x4F    // 50 Hz, low power, x/y/z axes enabled

#define ACCEL_CTRL_REG4         0x23
#define ACCEL_REG4_INIT         0x80    // block update, 2g full-scale, little endian

#define ACCEL_CTRL_REG6         0x25
#define ACCEL_IO_00             0x00
#define ACCEL_IO_11             0x02

#define ACCEL_START_READ_X      0xA8    // (AUTO_INC_BIT | OUT_X_L)

/*
 * Factory test hardware
 */

#define FACTORY_ADDR_TX         0xAA
#define FACTORY_ADDR_RX         0xAB

#endif
