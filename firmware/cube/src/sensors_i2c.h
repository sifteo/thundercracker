/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * Thundercracker cube firmware
 *
 * M. Elizabeth Scott <beth@sifteo.com>
 * Copyright <c> 2011 Sifteo, Inc.
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

/*
 * Internal declarations for I2C-attached sensors
 * (Accelerometer, factory test)
 */

#ifndef _SENSORS_I2C_H
#define _SENSORS_I2C_H

/*
 * LIS3DH accelerometer.
 */

extern static void i2c_accel_tx(const __code uint8_t *);

#if !( defined(USE_LIS3DE)  || \
       defined(USE_LIS3DH)  )
    #error No accelerometer selected
#endif

#if defined(USE_LIS3DE)
    //using LIS3DE
    #define ACCEL_ADDR_TX       0x52    // 01010010 - SDO is pulled HIGH (internally)
    #define ACCEL_ADDR_RX       0x53    // 01010011 - SDO is pulled HIGH (internally)
#endif

#if defined(USE_LIS3DH)
#if HWREV >= 4
    //using LIS3DH
    #define ACCEL_ADDR_TX       0x32    // 00110010 - SDO is pulled HIGH (internally)
    #define ACCEL_ADDR_RX       0x33    // 00110011 - SDO is pulled HIGH (internally)
#else
    //using LIS3DH with pulldown on SDO
    #define ACCEL_ADDR_TX       0x30    // 00110000 - SDO is tied LOW
    #define ACCEL_ADDR_RX       0x31    // 00110001 - SDO is tied LOW
#endif
#endif

#define ACCEL_CTRL_REG1         0x20
#define ACCEL_CTRL_REG2         0x21
#define ACCEL_CTRL_REG4         0x23
#define ACCEL_CTRL_REG6         0x25
#define ACCEL_INT1_CFG          0x30
#define ACCEL_INT1_THS          0x32
#define ACCEL_INT1_DUR          0x33
#define ACCEL_TEMP_CFG_REG      0x1f    // Temperature and ADC config

#define ACCEL_START_READ_X      0xA8    // (AUTO_INC_BIT | OUT_X_L)
#define ACCEL_START_READ_BAT    0x88    // (AUTO_INC_BIT | OUT_1_L)

/*
 * Initial accelerometer register values
 */

#define ACCEL_REG1_NORMAL       0x67    // 200 Hz, normal power, x/y/z axes enabled
#define ACCEL_REG4_NORMAL       0x80    // block update, 2g full-scale, little endian

#define ACCEL_TEMP_CFG_NORMAL   0x80    // ADC on, Temperature sensor off

#define ACCEL_IO_00             0x00    // I/O low  (int disabled, active-high)
#define ACCEL_IO_11             0x02    // I/O high (int disabled, active-low)

/*
 * Accelerometer settings for wakeup
 */

#define ACCEL_REG1_WAKE         0x4F    // 50 Hz, low power, x/y/z axes enabled
#define ACCEL_REG2_WAKE         0x01    // HPF on AOI for INT1
#define ACCEL_REG6_WAKE         0x40    // AOI interrupt on INT2 (active high)
#define ACCEL_TEMP_CFG_WAKE     0x00    // ADC off

#define ACCEL_CFG_WAKE          0x2A    // XHIE || YHIE || ZHIE (> THS)
#define ACCEL_THS_WAKE          0x2A    // threshold (LSB = 16mg)
#define ACCEL_DUR_WAKE          0x0A    // duration (@50HZ ODR -> LSB = 20ms)

/*
 * Factory test hardware
 */

#define FACTORY_ADDR_TX         0xAA
#define FACTORY_ADDR_RX         0xAB

/*
 * I2C initiation.
 *
 * The I2C state machine responds to completion interrupts. We need an external
 * event to initiate each transaction. This could come in multiple places: From TF2
 * if we're just finishing up neighbor transmit, or from TF0 if we're skipping
 * neighbor transmit.
 *
 * This is a small macro, since we don't want to spend the stack space on a function
 * call from these call sites.
 */

#pragma sdcc_hash +

#define I2C_PULSE()                                                                                  __endasm; \
    __asm anl     _MISC_DIR, #~MISC_I2C           ; Output drivers enabled                           __endasm; \
    __asm xrl     _MISC_PORT, #MISC_I2C           ; Now pulse I2C lines low                          __endasm; \
    __asm orl     _MISC_PORT, #MISC_I2C           ; Drive pins high - This delivers a 250 ns pulse   __endasm; \
    __asm

#define I2C_RESET()                                                                                  __endasm; \
    __asm mov     _W2CON0, #0                     ; Reset I2C master                                 __endasm; \
    __asm I2C_PULSE()                             ; Drive pins high - This delivers a 250 ns pulse   __endasm; \
    __asm mov     _W2CON0, #1                     ; Turn on I2C controller                           __endasm; \
    __asm mov     _W2CON0, #7                     ; Master mode, 100 kHz.                            __endasm; \
    __asm

#define I2C_INITIATE()                                                                               __endasm; \
    __asm mov     _i2c_state, #0                  ; Reset our state machine                          __endasm; \
    __asm I2C_RESET()                             ; Reset I2C master                                 __endasm; \
    __asm mov     _W2DAT, #ACCEL_ADDR_TX          ; Trigger the next I2C transaction                 __endasm; \
    __asm

#endif
