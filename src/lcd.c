/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * Sifteo prototype simulator
 * M. Elizabeth Scott <beth@sifteo.com>
 *
 * Copyright <c> 2011 Sifteo, Inc. All rights reserved.
 */

#include <stdint.h>
#include <SDL.h>
#include "lcd.h"

#define LCD_WIDTH    128
#define LCD_HEIGHT   128
#define SCALE        3

#define FB_SIZE      0x10000
#define FB_MASK      0xFFFF
#define FB_ROW_SHIFT 8

#define WIN_WIDTH    (LCD_WIDTH * SCALE)
#define WIN_HEIGHT   (LCD_HEIGHT * SCALE)

#define CMD_NOP      0x00
#define CMD_CASET    0x2A
#define CMD_RASET    0x2B
#define CMD_RAMWR    0x2C


static struct {
    SDL_Thread *thread;
    SDL_Surface *surface;
    SDL_sem *initSem;

    int is_running;
    int need_repaint;
    uint32_t write_count;

    /* 16-bit RGB 5-6-5 format */
    uint8_t fb_mem[FB_SIZE];

    /* Hardware interface */
    struct lcd_pins prev_pins;

    /* LCD Controller State */
    uint8_t current_cmd;
    unsigned cmd_bytecount;
    unsigned xs, xe, ys, ye;
    unsigned row;
} lcd;


static void lcd_repaint(void)
{
    int x, y, i, j;
    uint16_t *src = (uint16_t *) lcd.fb_mem;
    uint16_t *dest = (uint16_t *) lcd.surface->pixels;
    uint16_t *s_line;

    SDL_LockSurface(lcd.surface);

    for (y = LCD_HEIGHT; y--;) {
	for (i = SCALE; i--;) {
	    s_line = src;
	    for (x = LCD_WIDTH; x--;) {
		for (j = SCALE; j--;)
		    *(dest++) = *s_line;
		s_line++;
	    }
	}
	src += LCD_WIDTH;
    }

    SDL_UnlockSurface(lcd.surface);
    SDL_Flip(lcd.surface);
}

static int lcd_thread(void *param)
{
    lcd.surface = SDL_SetVideoMode(WIN_WIDTH, WIN_HEIGHT, 16, 0);
    if (lcd.surface == NULL)
	return;

    SDL_WM_SetCaption("Simulated LCD", NULL);

    // Initialization finished after we've redrawn once
    lcd_repaint();
    SDL_SemPost(lcd.initSem);

    while (lcd.is_running) {
	if (lcd.need_repaint) {
	    lcd.need_repaint = 0;
	    lcd_repaint();
	} else {
	    SDL_Delay(10);
	}
    }

    return 0;
}

static void lcd_reset(void)
{
    lcd.current_cmd = CMD_NOP;
    lcd.need_repaint = 1;
    lcd.xs = 0;
    lcd.ys = 0;
    lcd.xe = LCD_WIDTH - 1;
    lcd.ye = LCD_HEIGHT - 1;
}

void lcd_init(void)
{
    SDL_Init(SDL_INIT_VIDEO);
    lcd_reset();
    lcd.is_running = 1;

    lcd.initSem = SDL_CreateSemaphore(0);
    lcd.thread = SDL_CreateThread(lcd_thread, NULL);
    SDL_SemWait(lcd.initSem);
}

void lcd_exit(void)
{
    /*
     * XXX: Let the thred die on its own. SDL_Quit and SDL_WaitThread
     *      are both crashing on my Mac, somewhere deep in pthread
     *      code.
     */
    lcd.is_running = 0;
}

int lcd_eventloop(void)
{
    SDL_Event event;
    
    while (SDL_PollEvent(&event)) {
	switch (event.type) {
	    
	case SDL_QUIT:
	    return 1;
	    break;
	    
	}
    }

    return 0;
}

static inline uint8_t clamp(uint8_t val, uint8_t min, uint8_t max)
{
    if (val < min) val = min;
    if (val > max) val = max;
    return val;
}

static void lcd_cmd(uint8_t op)
{
    lcd.current_cmd = op;
    lcd.cmd_bytecount = 0;

    switch (op) {

    case CMD_RAMWR:
	// Return to start row/column
	lcd.row = lcd.ys;
	lcd.cmd_bytecount = lcd.xs << 1;
	lcd.write_count++;
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

    case CMD_RAMWR:
	lcd.fb_mem[FB_MASK & ((lcd.row << FB_ROW_SHIFT) + lcd.cmd_bytecount)] = byte;
	lcd.cmd_bytecount++;
	lcd.need_repaint = 1;
	if (lcd.cmd_bytecount > 1 + (lcd.xe << 1)) {
	    lcd.cmd_bytecount = lcd.xs << 1;
	    lcd.row++;
	    if (lcd.row > lcd.ye)
		lcd.row = lcd.ys;
	}
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

    // Assume we aren't driving the data output for now
    pins->data_drv = 0;

    if (pins->wrx && !lcd.prev_pins.wrx && !pins->dcx && !pins->csx) {
	/* Command write strobe */
	lcd_cmd(pins->data_in);
    }

    if (pins->wrx && !lcd.prev_pins.wrx && pins->dcx && !pins->csx) {
	/* Data write strobe */
	lcd_data(pins->data_in);
    }

    lcd.prev_pins = *pins;
}

uint32_t lcd_write_count(void)
{
    uint32_t cnt = lcd.write_count;
    lcd.write_count = 0;
    return cnt;
}
