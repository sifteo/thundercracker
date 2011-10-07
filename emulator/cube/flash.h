/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * Sifteo prototype simulator
 * M. Elizabeth Scott <beth@sifteo.com>
 *
 * Copyright <c> 2011 Sifteo, Inc. All rights reserved.
 */

#ifndef _FLASH_H
#define _FLASH_H

#include <stdint.h>
#include "emu8051.h"

enum busy_flag {
    BF_IDLE     = 0,
    BF_PROGRAM  = (1 << 0),
    BF_ERASE    = (1 << 1),
};

struct flash_pins {
    uint32_t  addr;       // IN
    uint8_t   oe;         // IN, active-low
    uint8_t   ce;         // IN, active-low
    uint8_t   we;         // IN, active-low
    uint8_t   data_in;    // IN

    uint8_t   data_drv;   // OUT, active-high
};

void flash_init(const char *filename);
void flash_exit(void);

uint32_t flash_cycle_count(void);
enum busy_flag flash_busy_flag(void);
unsigned flash_busy_percent(void);

void flash_tick(struct em8051 *cpu);
void flash_cycle(struct flash_pins *pins);
uint8_t flash_data_out(void);

// For the front-end's flash memory visualization
uint32_t flash_size(void);
uint8_t *flash_buffer(void);

#endif
