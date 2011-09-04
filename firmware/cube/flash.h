/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * Thundercracker cube firmware
 *
 * M. Elizabeth Scott <beth@sifteo.com>
 * Copyright <c> 2011 Sifteo, Inc. All rights reserved.
 */

#ifndef _FLASH_H
#define _FLASH_H

#include <stdint.h>
#include "hardware.h"

/*
 * Wait for an earlier write/erase to finish. This happens implicitly
 * in the functions here, but you need to call it explicitly before
 * any flash reads.
 */
void flash_wait(void);

/*
 * Flash erase operations.
 */
void flash_erase_chip(void);

/*
 * Flash programming. The address is stored globally, and
 * auto-incremented after every programmed byte.
 */

extern uint8_t flash_addr_low;
extern uint8_t flash_addr_lat1;
extern uint8_t flash_addr_lat2;

// Programs one byte, and increments flash_addr.
void flash_program_byte(uint8_t dat);

#endif
