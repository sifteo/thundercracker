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
#include "cube_hardware.h"

/*
 * High-level loadstream decoder interface
 */

volatile extern uint8_t __idata flash_fifo[FLS_FIFO_SIZE];
volatile extern uint8_t flash_fifo_head;                        // Pointer to next write location
volatile extern __bit flash_reset_request;                      // Pending reset?

void flash_init(void);
void flash_handle_fifo(void) __naked;

extern uint8_t flash_addr_low;          // Low 7 bits of address, left-justified
extern uint8_t flash_addr_lat1;         // Middle 7 bits of address, left-justified
extern uint8_t flash_addr_lat2;         // High 7 bits of address, left-justified
extern __bit flash_addr_a21;            // Bank selection bit

void flash_buffer_begin(void);
void flash_buffer_word(void) __naked;   // Assembly-callable only
void flash_buffer_commit(void);

#endif
