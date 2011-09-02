/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * Sifteo prototype simulator
 * M. Elizabeth Scott <beth@sifteo.com>
 *
 * Copyright <c> 2011 Sifteo, Inc. All rights reserved.
 */

#ifndef _LCD_H
#define _LCD_H

#define LCD_WIDTH    128
#define LCD_HEIGHT   128

#define FB_SIZE      0x10000    // Enough for LCD itself, and for profiler display
#define FB_MASK      0xFFFF
#define FB_ROW_SHIFT 8


struct lcd_pins {
    /* Configured for an 8-bit parallel bus, in 80-system mode */

    uint8_t   csx;        // IN, active-low
    uint8_t   dcx;        // IN, low=cmd high=data
    uint8_t   wrx;        // IN, rising edge
    uint8_t   rdx;        // IN, rising edge
    uint8_t   data_in;    // IN
};

void lcd_init(void);

uint16_t *lcd_framebuffer(void);
void lcd_te_pulse(void);

// For debugger
uint32_t lcd_write_count(void);

void lcd_cycle(struct lcd_pins *pins);
int lcd_te_tick(void);

#endif
