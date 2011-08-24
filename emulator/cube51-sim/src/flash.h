/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * Sifteo prototype simulator
 * M. Elizabeth Scott <beth@sifteo.com>
 *
 * Copyright <c> 2011 Sifteo, Inc. All rights reserved.
 */

#ifndef _FLASH_H
#define _FLASH_H

struct flash_pins {
    uint32_t  addr;       // IN
    uint8_t   oe;         // IN, active-low
    uint8_t   ce;         // IN, active-low
    uint8_t   we;         // IN, active-low
    uint8_t   data_in;    // IN

    uint8_t   data_out;   // OUT
    uint8_t   data_drv;   // OUT, active-high
};

void flash_init(const char *filename);
void flash_exit(void);
void flash_cycle(struct flash_pins *pins);

#endif
