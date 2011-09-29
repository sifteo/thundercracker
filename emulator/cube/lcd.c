/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * Sifteo prototype simulator
 * M. Elizabeth Scott <beth@sifteo.com>
 *
 * Copyright <c> 2011 Sifteo, Inc. All rights reserved.
 */

#include <stdint.h>
#include <string.h>

#include "lcd.h"
#include "emulator.h"

#define CMD_NOP      0x00
#define CMD_SWRESET  0x01
#define CMD_SLPIN    0x10
#define CMD_SLPOUT   0x11
#define CMD_DISPOFF  0x28
#define CMD_DISPON   0x29
#define CMD_CASET    0x2A
#define CMD_RASET    0x2B
#define CMD_RAMWR    0x2C
#define CMD_TEOFF    0x34
#define CMD_TEON     0x35
#define CMD_MADCTR   0x36
#define CMD_COLMOD   0x3A

#define COLMOD_12    3
#define COLMOD_16    5
#define COLMOD_18    6

#define MADCTR_MY    0x80
#define MADCTR_MX    0x40
#define MADCTR_MV    0x20
#define MADCTR_ML    0x10   // Not implemented
#define MADCTR_RGB   0x08   // Not implemented

#define TE_WIDTH_US  1000


static struct {
    uint32_t write_count;
    uint32_t te_timer_head;
    uint32_t te_timer_tail;

    /* 16-bit RGB 5-6-5 format */
    uint16_t fb_mem[FB_SIZE];

    /* Hardware interface */
    uint8_t prev_wrx;

    /*
     * LCD Controller State
     */
 
    uint8_t current_cmd;
    uint8_t cmd_bytecount;
    uint8_t pixel_bytes[3];

    unsigned xs, xe, ys, ye;
    unsigned row, col;

    uint8_t madctr;
    uint8_t colmod;
    uint8_t mode_awake;
    uint8_t mode_display_on;
    uint8_t mode_te;
} lcd;

static uint8_t clamp(uint8_t val, uint8_t min, uint8_t max)
{
    if (val < min) val = min;
    if (val > max) val = max;
    return val;
}

void lcd_init(void)
{
    memset(&lcd, 0, sizeof lcd);

    // Framebuffer contents undefined. Simulate that...
    uint32_t i;
    for (i = 0; i < FB_SIZE; i++)
	lcd.fb_mem[i] = 31337 * (1+i);

    lcd.xe = LCD_WIDTH - 1;
    lcd.ye = LCD_HEIGHT - 1;
    lcd.colmod = COLMOD_18;
}

static void lcd_first_pixel(void)
{
    // Return to start row/column
    lcd.row = lcd.ys;
    lcd.col = lcd.xs;
}

static void lcd_write_pixel(uint16_t pixel)
{
    unsigned vRow, vCol, addr;

    // Logical to physical address translation
    vRow = (lcd.madctr & MADCTR_MY) ? (LCD_HEIGHT - 1 - lcd.row) : lcd.row;
    vCol = (lcd.madctr & MADCTR_MX) ? (LCD_WIDTH - 1 - lcd.col) : lcd.col;
    addr = (lcd.madctr & MADCTR_MV) 
	? (vRow + (vCol << FB_ROW_SHIFT))
	: (vCol + (vRow << FB_ROW_SHIFT));
    
    lcd.fb_mem[addr & FB_MASK] = pixel;
    
    if (++lcd.col > lcd.xe) {
	lcd.col = lcd.xs;
	if (++lcd.row > lcd.ye)
	    lcd.row = lcd.ys;
    }
}

static void lcd_write_byte(uint8_t b)
{
    lcd.pixel_bytes[lcd.cmd_bytecount++] = b;

    switch (lcd.colmod) {

    case COLMOD_12:
	if (lcd.cmd_bytecount == 3) {
	    uint8_t r1 = lcd.pixel_bytes[0] >> 4;
	    uint8_t g1 = lcd.pixel_bytes[0] & 0x0F;
	    uint8_t b1 = lcd.pixel_bytes[1] >> 4;

	    uint8_t r2 = lcd.pixel_bytes[1] & 0x0F;
	    uint8_t g2 = lcd.pixel_bytes[2] >> 4;
	    uint8_t b2 = lcd.pixel_bytes[2] & 0x0F;

	    lcd.cmd_bytecount = 0;

	    lcd_write_pixel( (r1 << 12) | ((r1 >> 3) << 11) |
			     (g1 << 7) | ((g1 >> 2) << 5) |
			     (b1 << 1) | (b1 >> 3) );

	    lcd_write_pixel( (r2 << 12) | ((r2 >> 3) << 11) |
			     (g2 << 7) | ((g2 >> 2) << 5) |
			     (b2 << 1) | (b2 >> 3) );
	}
	break;

    case COLMOD_16:
	if (lcd.cmd_bytecount == 2) {
	    lcd.cmd_bytecount = 0;
	    lcd_write_pixel( (lcd.pixel_bytes[0] << 8) |
			     lcd.pixel_bytes[1] );
	}
	break;

    case COLMOD_18:
	if (lcd.cmd_bytecount == 3) {
	    uint8_t r = lcd.pixel_bytes[0] >> 3;
	    uint8_t g = lcd.pixel_bytes[1] >> 2;
	    uint8_t b = lcd.pixel_bytes[2] >> 3;

	    lcd.cmd_bytecount = 0;
	    lcd_write_pixel( (r << 11) | (g << 5) | b );
	}
	break;

    default:
	lcd.cmd_bytecount = 0;
	break;
    }
}

static void lcd_cmd(uint8_t op)
{
    lcd.current_cmd = op;
    lcd.cmd_bytecount = 0;

    switch (op) {

    case CMD_RAMWR:
	lcd_first_pixel();
	lcd.write_count++;
	break;

    case CMD_SWRESET:
	lcd_init();
	break;

    case CMD_SLPIN:
	lcd.mode_awake = 0;
	break;

    case CMD_SLPOUT:
	lcd.mode_awake = 1;
	break;

    case CMD_DISPOFF:
	lcd.mode_display_on = 0;
	break;

    case CMD_DISPON:
	lcd.mode_display_on = 1;
	break;

    case CMD_TEOFF:
	lcd.mode_te = 0;
	break;

    case CMD_TEON:
	lcd.mode_te = 1;
	break;

    }
}

static void lcd_data(uint8_t byte)
{
    switch (lcd.current_cmd) {

    case CMD_CASET:
	switch (lcd.cmd_bytecount++) {
	case 1:  lcd.xs = clamp(byte, 0, 0x83);
	case 3:  lcd.xe = clamp(byte, 0, 0x83);
	}
	break;

    case CMD_RASET:
	switch (lcd.cmd_bytecount++) {
	case 1:  lcd.ys = clamp(byte, 0, 0xa1);
	case 3:  lcd.ye = clamp(byte, 0, 0xa1);
	}
	break;

    case CMD_MADCTR:
	lcd.madctr = byte;
	break;

    case CMD_COLMOD:
	lcd.colmod = byte;
	break;

    case CMD_RAMWR:
	lcd_write_byte(byte);
	break;
    }
}

void lcd_cycle(struct lcd_pins *pins)
{
    /*
     * Make lots of assumptions...
     *
     * This is pretending to be an SPFD5414 controller, with the following settings:
     *
     *   - 8-bit parallel interface, in 80-series mode
     *   - 16-bit color depth, RGB-565 (3AH = 05)
     */

    if (!pins->csx && pins->wrx && !lcd.prev_wrx) {
	if (pins->dcx) {
	    /* Data write strobe */
	    lcd_data(pins->data_in);
	} else {
	    /* Command write strobe */
	    lcd_cmd(pins->data_in);
	}
    }

    lcd.prev_wrx = pins->wrx;
}

uint32_t lcd_write_count(void)
{
    uint32_t cnt = lcd.write_count;
    lcd.write_count = 0;
    return cnt;
}

uint16_t *lcd_framebuffer(void)
{
    return (lcd.mode_awake && lcd.mode_display_on) ? lcd.fb_mem : NULL;
}

void lcd_te_pulse(void)
{
    if (lcd.mode_te) {
	// This runs on the GUI thread, use a lock-free timer.
	lcd.te_timer_head += USEC_TO_CYCLES(TE_WIDTH_US);
    }
}

int lcd_te_tick(void)
{
    /*
     * We have a separate entry point for the TE timer, since it
     * really does need to happen every tick rather than just when
     * there's a graphics pin state change.
     */

    if (lcd.te_timer_head != lcd.te_timer_tail) {
	lcd.te_timer_tail++;
	return 1;
    }
    return 0;
}
