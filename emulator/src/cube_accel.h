/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * Sifteo Thundercracker simulator
 * M. Elizabeth Scott <beth@sifteo.com>
 *
 * Copyright <c> 2011 Sifteo, Inc. All rights reserved.
 */

/*
 * Simulated I2C accelerometer: ST LIS3DH.
 *
 * XXX: This is a very featureful accelerometer! Currently we only simulate
 *      a tiny bit of its functionality.
 */

#ifndef _CUBE_ACCEL_H
#define _CUBE_ACCEL_H

#include <stdint.h>

namespace Cube {


class I2CAccelerometer {
 public:

    void setVector(int16_t x, int16_t y, int16_t z) {
        regs.out_x = x;
        regs.out_y = y;
        regs.out_z = z;
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
            // XXX: MSB enables/disables auto-increment.
            regAddress = byte & 0x7F;
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
    static const uint8_t deviceAddress = 0x30;
    uint8_t regAddress;

    // Matches device register layout. Assumes little-endian.
    union {
        uint8_t bytes[0x31];
        struct {
            uint8_t reserved0[7];       // 00
            uint8_t status_reg;         // 07
            uint16_t out_adc1;          // 08
            uint16_t out_adc2;          // 0a
            uint16_t out_adc3;          // 0c
            uint8_t int_counter_reg;    // 0e
            uint8_t who_am_i;           // 0f
            uint8_t reserved1[15];      // 10
            uint8_t temp_cfg_reg;       // 1f
            uint8_t ctrl_reg[6];        // 20
            uint8_t reference;          // 26
            uint8_t status_reg2;        // 27
            int16_t out_x;              // 28
            int16_t out_y;              // 2a
            int16_t out_z;              // 2c
            uint8_t fifo_ctrl_reg;      // 2e
            uint8_t fifo_src_reg;       // 2f
            uint8_t int1_cfg;           // 30
            uint8_t int1_source;        // 31
            uint8_t int1_ths;           // 32
            uint8_t int1_duration;      // 33
            uint8_t reserved2[4];       // 34
            uint8_t click_cfg;          // 38
            uint8_t click_src;          // 39
            uint8_t click_ths;          // 3a
            uint8_t time_limit;         // 3b
            uint8_t time_latency;       // 3c
            uint8_t time_window;        // 3d
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
