/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * SPI test for the nrF24LE1.
 *
 * The Nordic datasheet was vague about the SPI timing and FIFOs..
 * This will give me a better idea of how they *actually* perform.
 *
 * M. Elizabeth Scott <beth@sifteo.com>
 * Copyright <c> 2011 Sifteo, Inc. All rights reserved.
 */

#include <stdint.h>
#include "hardware.h"

void main()
{
    uint8_t i = 0; 

    P0DIR = 0x00;
    P0 = 0xC0;

    SPIMCON1 = 0x0F;
    SPIMCON0 = 0x01;

    // Keep FIFO full
    SPIMDAT = ++i;
    while (1) {
        P0 ^= 1;
        SPIMDAT = ++i;
        while (!(SPIMSTAT & SPI_RX_READY));
        SPIMDAT;
    }
}
