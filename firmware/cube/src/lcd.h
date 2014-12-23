/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * Thundercracker cube firmware
 *
 * Micah Elizabeth Scott <micah@misc.name>
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

#ifndef _LCD_H
#define _LCD_H

/*
 * VRAM definitions
 */

#include <stdint.h>
#include <sifteo/abi/vram.h>

static __xdata __at 0x0000 union _SYSVideoRAM vram;

/*
 * LCD Controller
 */

#define LCD_WIDTH       128
#define LCD_HEIGHT      128
#define LCD_PIXELS      (LCD_WIDTH * LCD_HEIGHT)
#define LCD_ROW_SHIFT   8

/*
 * LCD Bus Operations
 */

void lcd_sleep();
void lcd_pwm_fade();
void lcd_begin_frame();
void lcd_end_frame();

extern uint8_t lcd_window_x;
extern uint8_t lcd_window_y;
void lcd_address_and_write();

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
#define ASM_LCD_CMD_MODE()      __endasm; LCD_CMD_MODE(); __asm
#define ASM_LCD_DATA_MODE()     __endasm; LCD_DATA_MODE(); __asm

#define ASM_LCD_BYTE(_b)                        __endasm; \
    __asm mov   _P2, _b                         __endasm; \
    __asm ASM_ADDR_INC2();                      __endasm; \
    __asm

/*
 * Bus Clocking
 */
    
void addr_inc1() __naked;
void addr_inc2() __naked;
void addr_inc3() __naked;
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

/*
 * VRAM utilities
 */

// dptr=src, r0=dest, r1=count
void vram_atomic_copy() __naked;


#endif
