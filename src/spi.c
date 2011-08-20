/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * Sifteo prototype simulator
 * M. Elizabeth Scott <beth@sifteo.com>
 *
 * Copyright <c> 2011 Sifteo, Inc. All rights reserved.
 */

#include <stdint.h>
#include "spi.h"

void spi_init(struct spi_master *self)
{
    self->rx_count = 0;
    self->tx_count = 0;
    self->timer = 0;
}

void spi_write_data(struct spi_master *self, uint8_t mosi)
{
    if (self->tx_count < 2) {
	self->tx_fifo[1] = self->tx_fifo[0];
	self->tx_fifo[0] = mosi;
	self->tx_count++;
    }	
}

uint8_t spi_read_data(struct spi_master *self)
{
    uint8_t miso = self->rx_fifo[0];

    if (self->rx_count > 0)  {
	self->rx_fifo[0] = self->rx_fifo[1];
	self->rx_count--;
    }

    return miso;
}

static uint32_t spi_ticks_per_byte(uint8_t con0)
{
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

int spi_tick(struct spi_master *self, uint8_t *regs)
{
    uint8_t con0 = regs[SPI_REG_CON0];

    if (!(con0 & SPI_ENABLE))
	return 0;

    if (self->timer) {
	/*
	 * We're already transmitting/receiving one byte.
	 * Count down the clock until it's done...
	 */
	if (!--self->timer) {
	    /*
	     * The byte just finished! Emulate the bus traffic, and
	     * enqueue the resulting MISO byte.
	     */
	    uint8_t miso = self->callback(self->tx_mosi);
	    if (self->rx_count < 2)
		self->rx_fifo[self->rx_count++] = miso;
	}
    }

    if (self->tx_count && !self->timer) {
	/*
	 * We aren't transmitting/receiving a byte yet, but we can start now.
	 *
	 * Move the byte from our FIFO to another register that
	 * simulates the SPI output shift-register. We'll hold it
	 * there until the proper amount of time has elapsed, which
	 * we'll also calculate now.
	 */

	self->tx_mosi = self->tx_fifo[--self->tx_count];
	self->timer = spi_ticks_per_byte(con0);
    }	

    // Update status register
    regs[SPI_REG_STATUS] = 
	(self->rx_count == 2 ? SPI_RX_FULL : 0) |
	(self->rx_count != 0 ? SPI_RX_READY : 0) |
	(self->tx_count == 0 ? SPI_TX_EMPTY : 0) |
	(self->tx_count != 2 ? SPI_TX_READY : 0);

    return !!(regs[SPI_REG_STATUS] & ~regs[SPI_REG_CON1]);
}
