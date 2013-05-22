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
#include <algorithm>

namespace Cube {


class I2CAccelerometer {
 public:

    void setVector(int16_t x, int16_t y, int16_t z)
    {
        /*
         * Store the latest sample, and update interrupt state.
         * This would normally be done at the rate configured by the
         * ODR registers, but we currently get this data at whatever rate
         * the physics simulation is running at, and we don't resample it.
         */

        const int kFilterLog2 = 8;
        const int kHardwareTicksPerSimulatedTick = 4;

        // Store latest samples
        regs.out_x = x;
        regs.out_y = y;
        regs.out_z = z;

        // Cheesy high-pass filter, probably not at all accurate
        lpX += x - (lpX >> kFilterLog2);
        lpY += y - (lpY >> kFilterLog2);
        lpZ += z - (lpZ >> kFilterLog2);
        if (regs.ctrl_reg[1] & 0x01) {
            x = std::min(0x7fff, std::max(-0x7fff, int(x) - (lpX >> kFilterLog2)));
            y = std::min(0x7fff, std::max(-0x7fff, int(y) - (lpY >> kFilterLog2)));
            z = std::min(0x7fff, std::max(-0x7fff, int(z) - (lpZ >> kFilterLog2)));
        }

        // Test against high/low interrupt thresholds, create a 6-bit source mask
        int16_t ths = int(regs.int1_ths) << 8;
        uint8_t src = 0;
        if (x < -ths) src |= 0x01; 
        if (x >  ths) src |= 0x02; 
        if (y < -ths) src |= 0x04; 
        if (y >  ths) src |= 0x08; 
        if (z < -ths) src |= 0x10; 
        if (z >  ths) src |= 0x20; 

        // Test the bit mask against INT1_CFG, to check our interrupt state.
        uint8_t cfg = regs.int1_cfg;
        bool nextInt;
        switch (cfg & 0xC0) {
            case 0x00:  // OR combination
                nextInt = (0 != (cfg & src));
                break;
            case 0x40:  // 6-direction edge
                nextInt = (0 == ((cfg ^ src) & 0x3F) && 0 == intDuration);
                break;
            case 0x80:  // AND combination
                nextInt = ((cfg & src) == (cfg & 0x3F));
                break;
            case 0xC0:  // 6-direction position
                nextInt = (0 == ((cfg ^ src) & 0x3F));
                break;
        }

        // Increment/reset interrupt duration
        if (nextInt)
            intDuration += kHardwareTicksPerSimulatedTick;
        else
            intDuration = 0;

        // Interrupt is active if the test has passed continuously for the set duration
        // This goes into bit 6 of 'src'
        if (intDuration > regs.int1_duration)
            src |= 0x40;

        regs.int1_src = src;
    }

    void setADC1(uint16_t value16) {
        regs.out_adc1 = value16;
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

            case S_DATA:
                if (regAddress < sizeof regs)
                    regs.bytes[regAddress++] = byte;
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

    bool intPin(unsigned index)
    {
        // Interrupt Active for INT1?
        ASSERT(index == 1);
        uint8_t reg6 = regs.ctrl_reg[5];
        bool source = ((reg6 & regs.int1_src) >> 6) & 1;
        
        // Both interrupts share the same active-high/active-low control pin
        return source ^ ((reg6 >> 1) & 1);
    }

 private:
    static const uint8_t deviceAddress = 0x32;
    uint8_t regAddress;
    unsigned intDuration;
    int lpX, lpY, lpZ;

    // Matches device register layout. Assumes little-endian.
    union {
        uint8_t bytes[0x3e];
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
            uint8_t int1_src;           // 31
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
