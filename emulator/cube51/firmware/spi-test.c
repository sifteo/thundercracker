/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * Simple test for the on-board SPI controller, in interrupt-driven mode.
 * M. Elizabeth Scott <beth@sifteo.com>
 *
 * Copyright <c> 2011 Sifteo, Inc. All rights reserved.
 */

#include <stdint.h>
#include "hardware.h"

static __xdata uint8_t debug = 0;
static __xdata uint8_t payload[32];

void rf_reg_write(uint8_t reg, uint8_t value)
{
    RF_CSN = 0;

    SPIRDAT = RF_CMD_W_REGISTER | reg;
    SPIRDAT = value;
    while (!(SPIRSTAT & SPI_RX_READY));
    SPIRDAT;
    while (!(SPIRSTAT & SPI_RX_READY));
    SPIRDAT;

    RF_CSN = 1;
}

void rf_isr(void) __interrupt(VECTOR_RF)
{
    register uint8_t i;
    
    /*
     * Write the STATUS register, to clear the IRQ.
     * We also get a STATUS read for free here.
     */

    RF_CSN = 0;					// Begin SPI transaction
    SPIRDAT = RF_CMD_W_REGISTER |		// Command byte
	RF_REG_STATUS;
    SPIRDAT = 0x70;				// Prefill TX FIFO with STATUS write
    while (!(SPIRSTAT & SPI_RX_READY));		// Wait for Command/STATUS byte
    SPIRDAT;					// Dummy read of STATUS byte
    while (!(SPIRSTAT & SPI_RX_READY));		// Wait for payload byte
    SPIRDAT;					// Dummy read
    RF_CSN = 1;					// End SPI transaction

    /*
     * If we had reason to check for other interrupts, we could use
     * the value of STATUS we read above to do so. But in this case,
     * we only get IRQs for incoming packets, so we're guaranteed to
     * have a packet waiting for us in the FIFO right now. Read it.
     */

    RF_CSN = 0;					// Begin SPI transaction
    SPIRDAT = RF_CMD_R_RX_PAYLOAD;		// Command byte
    SPIRDAT = 0;				// First dummy byte, keep the TX FIFO full
    while (!(SPIRSTAT & SPI_RX_READY));		// Wait for Command/STATUS byte
    SPIRDAT;					// Dummy read of STATUS byte
    for (i = 0; i < sizeof payload - 1; i++) {
	SPIRDAT = 0;				// Write next dummy byte
	while (!(SPIRSTAT & SPI_RX_READY));	// Wait for payload byte
	payload[i] = SPIRDAT;			// Read payload byte
    }
    while (!(SPIRSTAT & SPI_RX_READY));		// Wait for last payload byte
    payload[i] = SPIRDAT;			// Read last payload byte
    RF_CSN = 1;					// End SPI transaction
    
    debug++;    
}

void main(void)
{
    // Radio clock running
    RF_CKEN = 1;

    // Enable RX interrupt
    rf_reg_write(RF_REG_CONFIG, RF_STATUS_RX_DR);
    IEN_EN = 1;
    IEN_RF = 1;

    // Start receiving
    RF_CE = 1;
}
