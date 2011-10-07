/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * Sifteo prototype simulator
 * M. Elizabeth Scott <beth@sifteo.com>
 *
 * Copyright <c> 2011 Sifteo, Inc. All rights reserved.
 */

/*
 * This file implements a single SPI master controller.  The nRF24LE1
 * has two of these, one for the on-chip SPI bus used by the radio,
 * and one for the external SPI bus.
 *
 * It's a simple TX/RX FIFO with Interrupt support. We implement the
 * controller itself, using a supplied callback for emulating one byte
 * of data transfer with the actual SPI peripheral.
 *
 * The callback is invoked after the last bit of every transferred
 * byte. Its argument is the full byte that was sent via MOSI, and its
 * return value is the byte that the slave would have been writing to
 * MISO concurrently with that output byte.
 *
 * (This means that theoretically a simulated peripheral could "cheat"
 * and return a response that depended on the value of the byte
 * written over MOSI. Don't do that. Real hardware would have been
 * clocking out that response byte before it knew what the command
 * byte was.)
 */

/*
 * This makes a few big assumptions about the SPI hardware, based on
 * my reading of the data sheet and my best guess as to how the
 * hardware is implemented. So far, these seem to match what we've
 * seen on real hardware.
 *
 * 1. There are no hidden wait states between register writes and
 *    starting a transfer.
 *
 * 2. There is no additional interrupt latency, beyond the
 *    already-assumed LCALL latency.
 *
 * 3. The SPI clock is started when a transfer starts. It is not a
 *    global free-running clock.
 *
 * 4. The two-level FIFO queue does NOT include the actual I/O shift
 *    register, which is in a separate hardware register. In other
 *    words, it is possible to have up to two bytes queued, plus a
 *    third byte that is partially transmitted.
 */

#ifndef _SPI_H
#define _SPI_H

class SPIBus {
 public:

    // Peripheral devices
    Radio radio;

    void init(struct em8051 *_cpu) {
        cpu = _cpu;
        tx_count = 0;
        rx_count = 0;
        timer = 0;
        irq_state = 0;
        status_dirty = 1;
    }

    void writeData(uint8_t mosi) {
        if (tx_count < SPI_FIFO_SIZE) {
            memmove(tx_fifo + 1, tx_fifo, SPI_FIFO_SIZE - 1);
            tx_fifo[0] = mosi;
            tx_count++;
            status_dirty = 1;
        } else {
            cpu->except(cpu, EXCEPTION_SPI_XRUN);
        }
    }

    uint8_t readData() {
        uint8_t miso = rx_fifo[0];

        if (rx_count > 0)  {
            memmove(rx_fifo, rx_fifo + 1, SPI_FIFO_SIZE - 1);
            rx_count--;
            status_dirty = 1;
        } else
            cpu->except(cpu, EXCEPTION_SPI_XRUN);

        return miso;
    }

    /*
     * Emulate one CPU-clock cycle of SPI activity. This
     * is where we update SFRs, invoke callbacks,
     * and generate IRQs.
     *
     * Returns 1 if we're raising an IRQ, 0 if not.
     */
    int tick(uint8_t *regs) {
        uint8_t con0 = regs[SPI_REG_CON0];

        if (!(con0 & SPI_ENABLE))
            return 0;

        if (timer) {
            /*
             * We're already transmitting/receiving one byte.
             * Count down the clock until it's done...
             */
            if (!--timer) {
                /*
                 * The byte just finished! Emulate the bus traffic, and
                 * enqueue the resulting MISO byte.
                 */
                uint8_t miso = radio.spiByte(tx_mosi);
                if (rx_count < SPI_FIFO_SIZE)
                    rx_fifo[rx_count++] = miso;
                else
                    cpu->except(cpu, EXCEPTION_SPI_XRUN);
            }
            status_dirty = 1;
        }

        if (tx_count && !timer) {
            /*
             * We aren't transmitting/receiving a byte yet, but we can start now.
             *
             * Move the byte from our FIFO to another register that
             * simulates the SPI output shift-register. We'll hold it
             * there until the proper amount of time has elapsed, which
             * we'll also calculate now.
             */
            
            tx_mosi = tx_fifo[--tx_count];
            timer = ticksPerByte(con0);
            status_dirty = 1;
        }   

        if (status_dirty) {
            // Update status register
            regs[SPI_REG_STATUS] = 
                (rx_count == SPI_FIFO_SIZE ? SPI_RX_FULL : 0) |
                (rx_count != 0 ? SPI_RX_READY : 0) |
                (tx_count == 0 ? SPI_TX_EMPTY : 0) |
                (tx_count != SPI_FIFO_SIZE ? SPI_TX_READY : 0);
        
            irq_state = !!(regs[SPI_REG_STATUS] & ~regs[SPI_REG_CON1]);
            status_dirty = 0;
        }

        return irq_state;
    }

 private:
    static uint32_t ticksPerByte(uint8_t con0) {
        /*
         * Calculate how many ticks we'll spend on each
         * transmitted byte at the current SPI bus speed.
         */

        switch ((con0 & SPI_CLOCK_MASK) >> SPI_CLOCK_SHIFT) {

        case 0:   return 16;   // 1/2 MCU clock
        case 1:   return 32;   // 1/4 MCU clock
        case 2:   return 64;   // 1/8 MCU clock
        case 3:   return 128;  // 1/16 MCU clock
        case 4:   return 256;  // 1/32 MCU clock
        default:  return 512;  // 1/64 MCU clock

        }
    }

    static const uint8_t SPI_REG_CON0    = 0;       // Bus speed, clock mode
    static const uint8_t SPI_REG_CON1    = 1;       // FIFO IRQ masking
    static const uint8_t SPI_REG_STATUS  = 2;       // FIFO status
    static const uint8_t SPI_REG_DATA    = 3;       // FIFO Data I/O

    static const uint8_t SPI_ENABLE      = 0x01;
    static const uint8_t SPI_CLOCK_MASK  = 0x70;    // Clock bits in CON0
    static const uint8_t SPI_CLOCK_SHIFT = 4;

    // Status bits, used in CON1 and STATUS
    static const uint8_t SPI_RX_FULL     = 0x08;
    static const uint8_t SPI_RX_READY    = 0x04;
    static const uint8_t SPI_TX_EMPTY    = 0x02;
    static const uint8_t SPI_TX_READY    = 0x01;

    static const uint8_t SPI_FIFO_SIZE   = 2;

    struct em8051 *cpu; // Only for exception reporting!
    
    uint8_t tx_fifo[SPI_FIFO_SIZE]; // Writes pushed -> into [0]
    uint8_t rx_fifo[SPI_FIFO_SIZE]; // Reads pulled <- from [0]
    uint8_t tx_count;   // Number of bytes in tx_fifo
    uint8_t rx_count;   // Number of bytes in rx_fifo
    uint8_t tx_mosi;    // Transmit shift register
    uint32_t timer;     // Cycle count remaining on current byte

    uint8_t irq_state;
    uint8_t status_dirty;
};
 
#endif
