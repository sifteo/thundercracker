/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * Sifteo prototype simulator
 * M. Elizabeth Scott <beth@sifteo.com>
 *
 * Copyright <c> 2011 Sifteo, Inc. All rights reserved.
 */

/*
 * This file implements a single SPI master controller.
 * The nRF24LE1 has two of these, one for the on-chip
 * SPI bus used by the radio, and one for the external
 * SPI bus.
 *
 * It's a simple TX/RX FIFO with Interrupt support. We
 * implement the controller itself, using a supplied callback
 * for emulating one byte of data transfer with the actual
 * SPI peripheral.
 *
 * The callback is invoked after the last bit of every
 * transferred byte. Its argument is the full byte that was sent
 * via MOSI, and its return value is the byte that the slave would
 * have been writing to MISO concurrently with that output byte.
 *
 * (This means that theoretically a simulated peripheral could "cheat"
 * and return a response that depended on the value of the byte written
 * over MOSI. Don't do that. Real hardware would have been clocking out
 * that response byte before it knew what the command byte was.)
 */

#ifndef _SPI_H
#define _SPI_H

#define SPI_REG_CON0    0       // Bus speed, clock mode
#define SPI_REG_CON1    1       // FIFO IRQ masking
#define SPI_REG_STATUS  2       // FIFO status
#define SPI_REG_DATA    3       // FIFO Data I/O

#define SPI_ENABLE	0x01
#define SPI_CLOCK_MASK  0x70    // Clock bits in CON0
#define SPI_CLOCK_SHIFT 4

// Status bits, used in CON1 and STATUS
#define SPI_RX_FULL     0x08
#define SPI_RX_READY    0x04
#define SPI_TX_EMPTY    0x02
#define SPI_TX_READY    0x01

#define SPI_FIFO_SIZE	2

struct spi_master {
    struct em8051 *cpu;	// Only for exception reporting!

    uint8_t (*callback)(uint8_t mosi);
    
    uint8_t tx_fifo[SPI_FIFO_SIZE]; // Writes pushed -> into [0]
    uint8_t rx_fifo[SPI_FIFO_SIZE]; // Reads pulled <- from [0]
    uint8_t tx_count;	// Number of bytes in tx_fifo
    uint8_t rx_count;	// Number of bytes in rx_fifo
    uint8_t tx_mosi;	// Transmit shift register
    uint32_t timer;     // Cycle count remaining on current byte

    uint8_t irq_state;
    uint8_t status_dirty;
};

void spi_init(struct spi_master *self);

// Emulate access to SPI_REG_DATA
void spi_write_data(struct spi_master *self, uint8_t mosi);
uint8_t spi_read_data(struct spi_master *self);

/*
 * Emulate one CPU-clock cycle of SPI activity. This
 * is where we update SFRs, invoke callbacks,
 * and generate IRQs.
 *
 * Returns 1 if we're raising an IRQ, 0 if not.
 */
int spi_tick(struct spi_master *self, uint8_t *regs);
 
#endif
