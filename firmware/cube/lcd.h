/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * Thundercracker cube firmware
 *
 * M. Elizabeth Scott <beth@sifteo.com>
 * Copyright <c> 2011 Sifteo, Inc. All rights reserved.
 */

#ifndef _LCD_H
#define _LCD_H

/*
 * VRAM definitions
 */

#include <stdint.h>
typedef unsigned long long uint64_t;
typedef signed long long int64_t;
#include <sifteo/abi.h>
static __xdata __at 0x0000 union _SYSVideoRAM vram;

/*
 * LCD Bus Operations
 */

 void lcd_sleep();
void lcd_begin_frame();
void lcd_end_frame();

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

#define ASM_LCD_WRITE_BEGIN()   __endasm; LCD_WRITE_BEGIN(); __asm
#define ASM_LCD_WRITE_END()     __endasm; LCD_WRITE_END(); __asm
    
/*
 * Bus Clocking
 */
    
void addr_inc4() __naked;
void addr_inc8() __naked;
void addr_inc12() __naked;
void addr_inc16() __naked;
void addr_inc20() __naked;
void addr_inc24() __naked;
void addr_inc28() __naked;
void addr_inc32() __naked;    

#define ADDR_INC2()     { ADDR_PORT++; ADDR_PORT++; }
#define ADDR_INC4()     { ADDR_PORT++; ADDR_PORT++; ADDR_PORT++; ADDR_PORT++; }

#define ASM_ADDR_INC2()   __endasm; ADDR_INC2(); __asm
#define ASM_ADDR_INC4()   __endasm; ADDR_INC4(); __asm
     
#endif
