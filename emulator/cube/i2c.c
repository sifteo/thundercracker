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

#define W2CON0_STOP     0x20
#define W2CON0_START    0x10
#define W2CON0_400KHZ   0x08
#define W2CON0_100KHZ   0x04
#define W2CON0_SPEED    0x0C
#define W2CON0_MASTER   0x02
#define W2CON0_ENABLE   0x01

#define W2CON1_MASKIRQ  0x20
#define W2CON1_ACKN     0x02
#define W2CON1_READY    0x01

enum i2c_state {
    I2C_IDLE = 0,       // Not in a transfer yet
    I2C_WRITING,        // TX mode
    I2C_READING,        // RX mode
    I2C_READ_NAK,       // Last byte of a read, which we'll fail to ACK
};

struct {
    uint32_t timer;     // Cycles until we're not busy
    enum i2c_state state;

    uint8_t tx_buffer;
    uint8_t tx_buffer_full;
    uint8_t rx_buffer;
    uint8_t rx_buffer_full;
} i2c;


void i2c_init()
{
    i2c.timer = 0;
}

static void i2c_timer_set(struct em8051 *cpu, int bits)
{
    /*
     * Wake after 'bits' bit periods, and set READY.
     */

    uint8_t w2con0 = cpu->mSFR[REG_W2CON0];
    switch (w2con0 & W2CON0_SPEED) {
    case W2CON0_400KHZ: i2c.timer = HZ_TO_CYCLES(400000) * bits; break;
    case W2CON0_100KHZ: i2c.timer = HZ_TO_CYCLES(100000) * bits; break;
    default: cpu->except(cpu, EXCEPTION_I2C);
    }
}

int i2c_tick(struct em8051 *cpu)
{
    uint8_t w2con0 = cpu->mSFR[REG_W2CON0];
    uint8_t w2con1 = cpu->mSFR[REG_W2CON1];

    if (i2c.timer) {
        // Still busy

        if (!--i2c.timer)
            cpu->mSFR[REG_W2CON1] = w2con1 |= W2CON1_READY;

    } else {
        // I2C state machine can run

        if (i2c.state == I2C_IDLE || (w2con0 & W2CON0_START)) {
            /*
             * Explicit or implied start condition, and address byte
             */

            if (i2c.tx_buffer_full) {
                i2cbus_start();
                i2cbus_write(i2c.tx_buffer);
                i2c.state = (i2c.tx_buffer & 1) ? I2C_READING : I2C_WRITING;
                cpu->mSFR[REG_W2CON0] = w2con0 &= ~W2CON0_START;
                i2c.tx_buffer_full = 0;
                i2c_timer_set(cpu, 10);      // Start, data byte, ACK
            }

        } else if (i2c.state == I2C_WRITING) {
            
            if (i2c.tx_buffer_full) {
                i2cbus_write(i2c.tx_buffer);
                i2c.tx_buffer_full = 0;
                i2c_timer_set(cpu, 9);       // Data byte, ACK

            } else if (w2con0 & W2CON0_STOP) {
                i2cbus_stop();
                i2c.state = I2C_IDLE;
                cpu->mSFR[REG_W2CON0] = w2con0 &= ~W2CON0_STOP;
            }

        } else {
            /*
             * Reading (with/without ACK)
             */

            if (i2c.rx_buffer_full)
                cpu->except(cpu, EXCEPTION_I2C);
 
            i2c.rx_buffer = i2cbus_read(i2c.state == I2C_READING);
            i2c.rx_buffer_full = 1;
            
            if (i2c.state == I2C_READ_NAK) {
                // This was the last byte.
                i2cbus_stop();
                i2c.state = I2C_IDLE;

                i2c_timer_set(cpu, 10);  // Data byte, ACK, stop
            } else {
                i2c_timer_set(cpu, 9);  // Data byte, ACK
            }

            if (w2con0 & W2CON0_STOP) {
                // The next byte is the last
                i2c.state = I2C_READ_NAK;
                cpu->mSFR[REG_W2CON0] = w2con0 &= ~W2CON0_STOP;
            }
        }
    }
    
    /*
     * We return a level-triggered interrupt (Edge triggering is
     * handled externally, by the core's IEX3 interrupt input)
     */

    return (w2con0 & W2CON0_ENABLE) && (w2con1 & W2CON1_READY) && !(w2con1 & W2CON1_MASKIRQ);
}

void i2c_write_data(struct em8051 *cpu, uint8_t data)
{
    if (i2c.tx_buffer_full) {
        cpu->except(cpu, EXCEPTION_I2C);
    } else {
        i2c.tx_buffer = data;
        i2c.tx_buffer_full = 1;
    }
}

uint8_t i2c_read_data(struct em8051 *cpu)
{
    if (!i2c.rx_buffer_full)
        cpu->except(cpu, EXCEPTION_I2C);

    i2c.rx_buffer_full = 0;
    return i2c.rx_buffer;
}

uint8_t i2c_read_con1(struct em8051 *cpu)
{
    uint8_t value = cpu->mSFR[REG_W2CON1];

    // Reset READY bit after each read
    cpu->mSFR[REG_W2CON1] = value & ~W2CON1_READY;

    return value;
}
