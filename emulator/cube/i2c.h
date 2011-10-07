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
 */

#ifndef _I2C_H
#define _I2C_H

#include "emu8051.h"
#include "accel.h"

class I2CMaster {
 public:

    // Peripherals on this bus
    I2CAccelerometer accel;

    void init() {
        timer = 0;
    }

    int tick(struct em8051 *cpu) {
        uint8_t w2con0 = cpu->mSFR[REG_W2CON0];
        uint8_t w2con1 = cpu->mSFR[REG_W2CON1];
    
        if (timer) {
            // Still busy

            if (!--timer) {
                /*
                 * Write/read finished. Emulate reads at the end of their
                 * time window, so that the CPU has time to set a stop
                 * condition if necessary.
                 */
                
                if (state == I2C_READING) {
                    uint8_t stop = w2con0 & W2CON0_STOP;

                    if (rx_buffer_full)
                        cpu->except(cpu, EXCEPTION_I2C);
 
                    rx_buffer = accel.i2cRead(!stop);
                    rx_buffer_full = 1;
            
                    if (stop) {
                        accel.i2cStop();
                        state = I2C_IDLE;
                        cpu->mSFR[REG_W2CON0] = w2con0 &= ~W2CON0_STOP;
                    } else {
                        timerSet(cpu, 9);  // Data byte, ACK
                    }
                }
                
                if (next_ack_status)
                    w2con1 &= ~W2CON1_ACKN;
                else
                    w2con1 |= W2CON1_ACKN;
                
                w2con1 |= W2CON1_READY;
                cpu->mSFR[REG_W2CON1] = w2con1;
            }
            
        } else {
            // I2C state machine can run
            
            if ((state == I2C_IDLE && tx_buffer_full)
                || (w2con0 & W2CON0_START)) {
                
                /*
                 * Explicit or implied start condition, and address byte
                 */
                
                if (tx_buffer_full) {
                    accel.i2cStart();
                    next_ack_status = accel.i2cWrite(tx_buffer);
                    state = (tx_buffer & 1) ? I2C_WR_READ_ADDR : I2C_WRITING;
                    cpu->mSFR[REG_W2CON0] = w2con0 &= ~W2CON0_START;
                    tx_buffer_full = 0;
                    timerSet(cpu, 10);      // Start, data byte, ACK
                }
                
            } else if (state == I2C_WRITING) {
                /*
                 * Emulate writes at the beginning of their time window
                 */
                
                if (tx_buffer_full) {
                    next_ack_status = accel.i2cWrite(tx_buffer);
                    tx_buffer_full = 0;
                    timerSet(cpu, 9);       // Data byte, ACK
                    
                } else if (w2con0 & W2CON0_STOP) {
                    accel.i2cStop();
                    state = I2C_IDLE;
                    cpu->mSFR[REG_W2CON0] = w2con0 &= ~W2CON0_STOP;
                }
                
            } else if (state == I2C_WR_READ_ADDR) {
                state = I2C_READING;
                timerSet(cpu, 9);       // Data byte, ACK
            }
        }
        
        /*
         * We return a level-triggered interrupt (Edge triggering is
         * handled externally, by the core's IEX3 interrupt input)
         */
        
        return (w2con0 & W2CON0_ENABLE) && (w2con1 & W2CON1_READY) && !(w2con1 & W2CON1_MASKIRQ);
    }
    
    void writeData(struct em8051 *cpu, uint8_t data) {
        if (tx_buffer_full) {
            cpu->except(cpu, EXCEPTION_I2C);
        } else {
            tx_buffer = data;
            tx_buffer_full = 1;
        }
    }

    uint8_t readData(struct em8051 *cpu) {
        if (!rx_buffer_full)
            cpu->except(cpu, EXCEPTION_I2C);
        
        rx_buffer_full = 0;
        return rx_buffer;
    }

    uint8_t readCON1(struct em8051 *cpu) {
        uint8_t value = cpu->mSFR[REG_W2CON1];
        
        // Reset READY bit after each read
        cpu->mSFR[REG_W2CON1] = value & ~W2CON1_READY;
        
        return value;
    }

 private:
    void timerSet(struct em8051 *cpu, int bits) {
        /*
         * Wake after 'bits' bit periods, and set READY.
         */
        
        uint8_t w2con0 = cpu->mSFR[REG_W2CON0];
        switch (w2con0 & W2CON0_SPEED) {
        case W2CON0_400KHZ: timer = HZ_TO_CYCLES(400000) * bits; break;
        case W2CON0_100KHZ: timer = HZ_TO_CYCLES(100000) * bits; break;
        default: cpu->except(cpu, EXCEPTION_I2C);
        }
    }

    static const uint8_t W2CON0_STOP    = 0x20;
    static const uint8_t W2CON0_START   = 0x10;
    static const uint8_t W2CON0_400KHZ  = 0x08;
    static const uint8_t W2CON0_100KHZ  = 0x04;
    static const uint8_t W2CON0_SPEED   = 0x0C;
    static const uint8_t W2CON0_MASTER  = 0x02;
    static const uint8_t W2CON0_ENABLE  = 0x01;
    
    static const uint8_t W2CON1_MASKIRQ = 0x20;
    static const uint8_t W2CON1_ACKN    = 0x02;
    static const uint8_t W2CON1_READY   = 0x01;

    enum i2c_state {
        I2C_IDLE = 0,       // Not in a transfer yet
        I2C_WRITING,        // TX mode
        I2C_WR_READ_ADDR,   // Writing an address for RX
        I2C_READING,        // RX mode
    };

    uint32_t timer;     // Cycles until we're not busy
    enum i2c_state state;

    uint8_t next_ack_status;
    uint8_t tx_buffer;
    uint8_t tx_buffer_full;
    uint8_t rx_buffer;
    uint8_t rx_buffer_full;
};
 
#endif
