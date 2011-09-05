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
 * High-level loadstream decoder interface
 */

#define FLASH_FIFO_MASK  63
extern uint8_t __idata flash_fifo[FLASH_FIFO_MASK + 1];
extern uint8_t flash_fifo_head;

void flash_init(void);
void flash_handle_fifo();
void flash_wait(void);

/*
 * Low-level hardware abstraction layer
 */

#define FLASH_SIZE		(2 * 1024 * 1024)
#define FLASH_BLOCK_SIZE	(64 * 1024)
#define FLASH_NUM_BLOCKS	32

extern uint8_t flash_addr_low;
extern uint8_t flash_addr_lat1;
extern uint8_t flash_addr_lat2;

void flash_erase(uint8_t blockCount);
void flash_program(uint8_t dat);

#endif
