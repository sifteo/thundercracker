/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * Sifteo Thundercracker simulator
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

#ifndef _CUBE_I2C_H
#define _CUBE_I2C_H

#include "vtime.h"
#include "cube_cpu.h"
#include "cube_accel.h"

namespace Cube {


class I2CBus {
 public:

    // Peripherals on this bus
    I2CAccelerometer accel;

    void init() {
        timer = 0;
        state = I2C_IDLE;
        iex3 = false;
        next_ack_status = false;
        tx_buffer_full = false;
        rx_buffer_full = false;
    }

    ALWAYS_INLINE void tick(TickDeadline &deadline, CPU::em8051 *cpu) {
        uint8_t w2con0 = cpu->mSFR[REG_W2CON0];
        uint8_t w2con1 = cpu->mSFR[REG_W2CON1];
    
        if (!(w2con0 & W2CON0_ENABLE)) {
            // Hardware disabled, reset state
            
            if (state != I2C_IDLE && cpu->isTracing)
                fprintf(cpu->traceFile, "[%2d] I2C: State reset\n", cpu->id);
            
            timer = 0;
            state = I2C_IDLE;
            tx_buffer_full = false;
            rx_buffer_full = false;
        
        } else if (LIKELY(timer)) {
            // Still busy

            if (deadline.hasPassed(timer)) {
                /*
                 * Write/read finished. Emulate reads at the end of their
                 * time window, so that the CPU has time to set a stop
                 * condition if necessary.
                 */

                if (cpu->isTracing)
                    fprintf(cpu->traceFile, "[%2d] I2C: timer fired\n", cpu->id);

                timer = 0;

                if (state == I2C_READING) {
                    uint8_t stop = w2con0 & W2CON0_STOP;

                    if (rx_buffer_full)
                        CPU::except(cpu, CPU::EXCEPTION_I2C);

                    rx_buffer = busRead(cpu, !stop);
                    rx_buffer_full = 1;
            
                    if (stop) {
                        busStop(cpu);
                        state = I2C_IDLE;
                        cpu->mSFR[REG_W2CON0] = w2con0 &= ~W2CON0_STOP;
                    } else {
                        timerSet(deadline, cpu, 9);  // Data byte, ACK
                    }
                }
                
                if (next_ack_status)
                    w2con1 &= ~W2CON1_ACKN;
                else
                    w2con1 |= W2CON1_ACKN;
                
                w2con1 |= W2CON1_READY;
                cpu->mSFR[REG_W2CON1] = w2con1;

            } else {
                // Still waiting
                deadline.set(timer);
            }
            
        } else {
            // I2C state machine can run
            
            if ((state == I2C_IDLE && tx_buffer_full)
                || (w2con0 & W2CON0_START)) {
                
                /*
                 * Explicit or implied start condition, and address byte
                 */
                
                if (tx_buffer_full) {
                    busStart(cpu);
                    next_ack_status = busWrite(cpu, tx_buffer);
                    state = (tx_buffer & 1) ? I2C_WR_READ_ADDR : I2C_WRITING;
                    cpu->mSFR[REG_W2CON0] = w2con0 &= ~W2CON0_START;
                    tx_buffer_full = 0;
                    timerSet(deadline, cpu, 10);      // Start, data byte, ACK
                }
                
            } else if (state == I2C_WRITING) {
                /*
                 * Emulate writes at the beginning of their time window
                 */
                
                if (tx_buffer_full) {
                    next_ack_status = busWrite(cpu, tx_buffer);
                    tx_buffer_full = 0;
                    timerSet(deadline, cpu, 9);       // Data byte, ACK
                    
                } else if (w2con0 & W2CON0_STOP) {
                    busStop(cpu);
                    state = I2C_IDLE;
                    cpu->mSFR[REG_W2CON0] = w2con0 &= ~W2CON0_STOP;
                }
                
            } else if (state == I2C_WR_READ_ADDR) {
                state = I2C_READING;
                timerSet(deadline, cpu, 9);       // Data byte, ACK
            }
        }
        
        /*
         * We return a level-triggered interrupt, which is then fed
         * into the core's IEX3 line which has edge-triggering.
         */
 
        bool nextIEX3 = ( (w2con0 & W2CON0_ENABLE) &&
                          (w2con1 & W2CON1_READY) &&
                          !(w2con1 & W2CON1_MASKIRQ) &&
                          (cpu->mSFR[REG_INTEXP] & 0x04) );
       
        if (UNLIKELY(nextIEX3 != iex3)) {
            if (cpu->isTracing)
                fprintf(cpu->traceFile, "[%2d] I2C: IEX3 level %d -> %d\n", cpu->id, iex3, nextIEX3);

            if (cpu->mSFR[REG_T2CON] & 0x40) {
                // Rising edge
                if (nextIEX3 && !iex3) {
                    cpu->mSFR[REG_IRCON] |= IRCON_SPI;
                    cpu->needInterruptDispatch = true;
                }
            } else {
                // Falling edge
                if (!nextIEX3 && iex3) {
                    cpu->mSFR[REG_IRCON] |= IRCON_SPI;
                    cpu->needInterruptDispatch = true;
                }
            }
            iex3 = nextIEX3;
        }        
    }
    
    void writeData(CPU::em8051 *cpu, uint8_t data) {
        if (cpu->isTracing)
            fprintf(cpu->traceFile, "[%2d] I2C: write %02x\n", cpu->id, data);

        if (tx_buffer_full) {
            CPU::except(cpu, CPU::EXCEPTION_I2C);
        } else {
            tx_buffer = data;
            tx_buffer_full = 1;
        }
    }

    uint8_t readData(CPU::em8051 *cpu) {
        if (!rx_buffer_full)
            CPU::except(cpu, CPU::EXCEPTION_I2C);

        if (cpu->isTracing)
            fprintf(cpu->traceFile, "[%2d] I2C: read %02x\n", cpu->id, rx_buffer);
        
        rx_buffer_full = 0;
        return rx_buffer;
    }

    uint8_t readCON1(CPU::em8051 *cpu) {
        uint8_t value = cpu->mSFR[REG_W2CON1];
        
        // Reset READY bit after each read
        cpu->mSFR[REG_W2CON1] = value & ~W2CON1_READY;

        if (cpu->isTracing)
            fprintf(cpu->traceFile, "[%2d] I2C: con1 -> %02x\n", cpu->id, value);
        
        return value;
    }

 private:
    void timerSet(TickDeadline &deadline, CPU::em8051 *cpu, int bits) {
        /*
         * Wake after 'bits' bit periods, and set READY.
         */

        if (cpu->isTracing)
            fprintf(cpu->traceFile, "[%2d] I2C: timer started, %d bits\n", cpu->id, bits);
        
        uint8_t w2con0 = cpu->mSFR[REG_W2CON0];
        switch (w2con0 & W2CON0_SPEED) {
        case W2CON0_400KHZ: timer = deadline.setRelative(VirtualTime::hz(400000) * bits); break;
        case W2CON0_100KHZ: timer = deadline.setRelative(VirtualTime::hz(100000) * bits); break;
        default: CPU::except(cpu, CPU::EXCEPTION_I2C);
        }
    }

    void busStart(CPU::em8051 *cpu) {
        if (cpu->isTracing)
            fprintf(cpu->traceFile, "[%2d] I2C: BUS start\n", cpu->id); 

        accel.i2cStart();
    }

    void busStop(CPU::em8051 *cpu) {
        if (cpu->isTracing)
            fprintf(cpu->traceFile, "[%2d] I2C: BUS stop\n", cpu->id); 

        accel.i2cStop();
    }
    
    uint8_t busWrite(CPU::em8051 *cpu, uint8_t byte) {
        if (cpu->isTracing)
            fprintf(cpu->traceFile, "[%2d] I2C: BUS write %02x\n", cpu->id, byte); 

        return accel.i2cWrite(byte);
    }

    uint8_t busRead(CPU::em8051 *cpu, uint8_t ack) {
        uint8_t result = accel.i2cRead(ack);

        if (cpu->isTracing)
            fprintf(cpu->traceFile, "[%2d] I2C: BUS read(%d) %02x\n", cpu->id, ack, result);

        return result;
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

    uint64_t timer;         // Cycle count at which we're not busy
    enum i2c_state state;

    bool iex3;
    uint8_t next_ack_status;
    uint8_t tx_buffer;
    uint8_t tx_buffer_full;
    uint8_t rx_buffer;
    uint8_t rx_buffer_full;
};


};  // namespace Cube
 
#endif
