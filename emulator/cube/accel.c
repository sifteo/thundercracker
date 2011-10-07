/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * Sifteo prototype simulator
 * M. Elizabeth Scott <beth@sifteo.com>
 *
 * Copyright <c> 2011 Sifteo, Inc. All rights reserved.
 */

/*
 * Simulated I2C accelerometer: MEMSIC DTOS v005
 */

#include <stdint.h>
#include "i2c.h"
#include "emu8051.h"
#include "emulator.h"

#ifdef DEBUG
#define I2C_DEBUG(_x)   printf _x
#else
#define I2C_DEBUG(_x)
#endif

#define ACCEL_ADDRESS  0x2A

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

    enum {
        S_IDLE,
        S_I2C_ADDRESS,
        S_REG_ADDRESS,
        S_DATA,
    } state;

    uint8_t address;
} accel;


void accel_set_vector(int8_t x, int8_t y)
{
    accel.regs.x = x;
    accel.regs.y = y;
}
 
void i2cbus_start()
{
    I2C_DEBUG(("I2C Start\n"));
    accel.state = S_I2C_ADDRESS;
}

void i2cbus_stop()
{
    I2C_DEBUG(("I2C Stop\n"));
    accel.state = S_IDLE;
}

uint8_t i2cbus_write(uint8_t byte)
{
    switch (accel.state) {

    case S_I2C_ADDRESS:
        if ((byte & 0xFE) == ACCEL_ADDRESS) {
            if (byte & 1) {
                // Read
                accel.state = S_DATA;
            } else {
                // Write
                accel.state = S_REG_ADDRESS;
            }
        } else {
            // Not us
            accel.state = S_IDLE;
        }
        break;

    case S_REG_ADDRESS:
        accel.address = byte;
        accel.state = S_DATA;
        break;

    default:
        break;
    }        

    I2C_DEBUG(("I2C Write 0x%02x, state=%d\n", byte, accel.state));

    return accel.state != S_IDLE;
}

uint8_t i2cbus_read(uint8_t ack)
{
    uint8_t result = 0xff;

    switch (accel.state) {

    case S_DATA:
        if (accel.address < sizeof accel.regs)
            result = accel.regs.bytes[accel.address++];
        break;

    default:
        break;

    }

    I2C_DEBUG(("I2C Read (ack=%d, state=%d) -> 0x%02x\n", ack, accel.state, result));
    return result;
}

