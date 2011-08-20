/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * Sifteo prototype simulator
 * M. Elizabeth Scott <beth@sifteo.com>
 *
 * Copyright <c> 2011 Sifteo, Inc. All rights reserved.
 */

#include "radio.h"

void radio_init(void)
{
}

void radio_exit(void)
{
}

uint8_t radio_spi_byte(uint8_t mosi)
{
    return 0xFF;
}

void radio_ctrl(int csn, int ce)
{
}

int radio_tick(void)
{
    return 0;
}
