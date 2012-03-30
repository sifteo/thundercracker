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

#include <protocol.h>
#include "hardware.h"

/*
 * High-level loadstream decoder interface
 */

volatile extern uint8_t __idata flash_fifo[FLS_FIFO_SIZE];
volatile extern uint8_t flash_fifo_head;

#pragma callee_saves flash_init,flash_erase,flash_program_start,flash_program_word

void flash_init(void);
void flash_handle_fifo();

/*
 * Low-level hardware abstraction layer
 */

extern uint8_t flash_addr_low;          // Low 7 bits of address, left-justified
extern uint8_t flash_addr_lat1;         // Middle 7 bits of address, left-justified
extern uint8_t flash_addr_lat2;         // High 7 bits of address, left-justified
extern __bit flash_need_autoerase;      // Do we need to test for auto-erase at next write?

void flash_program_start(void);
void flash_program_end(void);
void flash_program_word(uint16_t dat) __naked;


#endif
