/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * Thundercracker cube firmware
 *
 * M. Elizabeth Scott <beth@sifteo.com>
 * Copyright <c> 2011 Sifteo, Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
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
