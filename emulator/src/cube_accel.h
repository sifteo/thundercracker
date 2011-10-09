/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * Sifteo Thundercracker simulator
 * M. Elizabeth Scott <beth@sifteo.com>
 *
 * Copyright <c> 2011 Sifteo, Inc. All rights reserved.
 */

/*
 * Simulated I2C accelerometer: MEMSIC DTOS v005
 */

#ifndef _CUBE_ACCEL_H
#define _CUBE_ACCEL_H

#include <stdint.h>

namespace Cube {


class I2CAccelerometer {
 public:

    void setVector(int8_t x, int8_t y) {
        /* XXX: Axes swapped on our prototype hardware */
        regs.x = y;
        regs.y = x;
    }

    void i2cStart() {
        state = S_I2C_ADDRESS;
    }
    
    void i2cStop() {
        state = S_IDLE;
    }

    uint8_t i2cWrite(uint8_t byte) {
        switch (state) {

        case S_I2C_ADDRESS:
            if ((byte & 0xFE) == deviceAddress) {
                if (byte & 1) {
                    // Read
                    state = S_DATA;
                } else {
                    // Write
                    state = S_REG_ADDRESS;
                }
            } else {
                // Not us
                state = S_IDLE;
            }
            break;
            
        case S_REG_ADDRESS:
            regAddress = byte;
            state = S_DATA;
            break;

        default:
            break;
        }

        return state != S_IDLE;
    }

    uint8_t i2cRead(uint8_t ack) {
        uint8_t result = 0xff;

        switch (state) {

        case S_DATA:
            if (regAddress < sizeof regs)
                result = regs.bytes[regAddress++];
            break;

        default:
            break;
            
        }
        
        return result;
    }

 private:
    static const uint8_t deviceAddress = 0x2A;
    uint8_t regAddress;

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

};


};  // namespace Cube

#endif
