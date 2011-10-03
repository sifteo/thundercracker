/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * Sifteo prototype simulator
 * M. Elizabeth Scott <beth@sifteo.com>
 *
 * Copyright <c> 2011 Sifteo, Inc. All rights reserved.
 */

#include <stdint.h>
#include "i2c.h"
#include "emu8051.h"
#include "emulator.h"

/*
 * Simulated I2C accelerometer: MEMSIC DTOS v005
 */

struct {
    union {
        uint8_t bytes[11];
        struct {
            int8_t x;
            int8_t y;
            uint8_t status;
            uint8_t _res0;
            uint8_t detection;
            uint8_t _res1[3];
            uint8_t id[2];
            uint8_t address;
        };
    } regs;
} accel;


void accel_set_vector(int8_t x, int8_t y)
{
    accel.regs.x = x;
    accel.regs.y = y;
}
 
void i2cbus_start()
{
    printf("I2C Start\n");
}

void i2cbus_stop()
{
    printf("I2C Stop\n");
}

void i2cbus_write(uint8_t byte)
{
    printf("I2C Write 0x%02x\n", byte);
}

uint8_t i2cbus_read(uint8_t ack)
{
    printf("I2C Read (%d)\n", ack);
    return 0xaa;
}

