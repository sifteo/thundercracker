/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * Thundercracker cube firmware
 *
 * M. Elizabeth Scott <beth@sifteo.com>
 * Copyright <c> 2011 Sifteo, Inc. All rights reserved.
 */

#ifndef _LCD_H
#define _LCD_H

#include <stdint.h>
typedef unsigned long long uint64_t;
#include <sifteo/abi.h>
static __xdata __at 0x0000 union _SYSVideoRAM vram;

void lcd_sleep();
void lcd_begin_frame();
void lcd_end_frame();

void graphics_render() __naked;


#define LCD_WRITE_BEGIN() {                     \
        BUS_DIR = 0;                            \
    }

#define LCD_WRITE_END() {                       \
        BUS_DIR = 0xFF;                         \
    }

#define LCD_CMD_MODE() {                        \
        CTRL_PORT = CTRL_LCD_CMD;               \
    }

#define LCD_DATA_MODE() {                       \
        CTRL_PORT = CTRL_IDLE;                  \
    }

#define LCD_BYTE(b)     {                       \
        BUS_PORT = (b);                         \
        ADDR_INC2();                            \
    }

#endif
