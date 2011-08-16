/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * Sifteo prototype simulator
 * M. Elizabeth Scott <beth@sifteo.com>
 *
 * Copyright <c> 2011 Sifteo, Inc. All rights reserved.
 */

#include <stdint.h>
#include "flash.h"

void flash_init(const char *filename)
{
}

void flash_exit(void)
{
}

void flash_cycle(struct flash_pins *pins)
{
    pins->data_drv = 0;
}
