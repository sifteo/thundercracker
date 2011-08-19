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
}

void spi_write_data(struct spi_master *self, uint8_t mosi)
{
}

uint8_t spi_read_data(struct spi_master *self)
{
    return 0;
}

int spi_tick(struct spi_master *self, uint8_t *regs)
{
    return 0;
}
