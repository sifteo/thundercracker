/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * Sifteo prototype firmware
 * M. Elizabeth Scott <beth@sifteo.com>
 *
 * Copyright <c> 2011 Sifteo, Inc. All rights reserved.
 */

#include <stdint.h>
#include <mcs51/8051.h>

sfr at 0x93 P0DIR;
sfr at 0x94 P1DIR;
sfr at 0x95 P2DIR;
sfr at 0x96 P3DIR;

#define LCD_WIDTH 	128
#define LCD_HEIGHT	128
#define LCD_PIXELS	(LCD_WIDTH * LCD_HEIGHT)
#define LCD_ROW_SHIFT	8

#define CHROMA_KEY	0xF5

#define BUS_PORT	P0
#define BUS_DIR 	P0DIR

#define ADDR_PORT	P1
#define ADDR_DIR	P1DIR

#define CTRL_PORT	P2
#define CTRL_DIR 	P2DIR

#define CTRL_LCD_RDX	(1 << 0)
#define CTRL_LCD_DCX	(1 << 1)
#define CTRL_LCD_CSX	(1 << 2)
#define CTRL_FLASH_WE	(1 << 3)
#define CTRL_FLASH_CE	(1 << 4)
#define CTRL_FLASH_OE	(1 << 5)
#define CTRL_FLASH_LAT1	(1 << 6)
#define CTRL_FLASH_LAT2	(1 << 7)

#define CTRL_IDLE	(CTRL_FLASH_WE | CTRL_FLASH_OE | CTRL_LCD_DCX)
#define CTRL_LCD_CMD	(CTRL_FLASH_WE | CTRL_FLASH_OE)
#define CTRL_FLASH_OUT	(CTRL_FLASH_WE | CTRL_LCD_DCX)

#define LCD_CMD_NOP  	0x00
#define LCD_CMD_CASET	0x2A
#define LCD_CMD_RASET	0x2B
#define LCD_CMD_RAMWR	0x2C

uint8_t xdata tilemap[256];

#define TILE(s, x, y)  (tilemap[(x) + ((y)<<(s))])


/*
 * Blatantly insufficient for a real chip, but so far this sets up
 * everything the simulator cares about.
 */
void hardware_init(void)
{
    BUS_DIR = 0xFF;

    ADDR_PORT = 0;
    ADDR_DIR = 0;

    CTRL_PORT = CTRL_IDLE;
    CTRL_DIR = 0;
}

/*
 * One-off LCD command byte. Slow but who cares?
 */
void lcd_cmd_byte(uint8_t b)
{
    CTRL_PORT = CTRL_LCD_CMD;
    BUS_DIR = 0;
    BUS_PORT = b;
    ADDR_PORT++;
    ADDR_PORT++;
    BUS_DIR = 0xFF;
    CTRL_PORT = CTRL_IDLE;
}

/*
 * One-off LCD data byte. Really slow, for setup only.
 */
void lcd_data_byte(uint8_t b)
{
    BUS_DIR = 0;
    BUS_PORT = b;
    ADDR_PORT++;
    ADDR_PORT++;
    BUS_DIR = 0xFF;
}

/*
 * Lookup table that can burst up to 32 pixels at a time.
 * We probably spend so much time setting this up that the savings are negated :P
 */
void lcd_addr_burst(uint8_t pixels)
{
    pixels;

__asm
    ; Equivalent to:
    ; a = 0x100 - (pixels << 3)

    clr a
    clr c
    subb a, dpl
    rl a
    rl a
    rl a
    anl a, #0xF8

    mov DPTR, #table
    jmp @a+DPTR

table:
    ; This table holds 128 copies of "inc ADDR_PORT".
    ; It occupies exactly 256 bytes in memory, and the full table represents 32 pixels worth of burst transfer.

    .db 5,ADDR_PORT,5,ADDR_PORT,5,ADDR_PORT,5,ADDR_PORT,5,ADDR_PORT,5,ADDR_PORT,5,ADDR_PORT,5,ADDR_PORT  ; 1 x 8
    .db 5,ADDR_PORT,5,ADDR_PORT,5,ADDR_PORT,5,ADDR_PORT,5,ADDR_PORT,5,ADDR_PORT,5,ADDR_PORT,5,ADDR_PORT  ; 2 x 8
    .db 5,ADDR_PORT,5,ADDR_PORT,5,ADDR_PORT,5,ADDR_PORT,5,ADDR_PORT,5,ADDR_PORT,5,ADDR_PORT,5,ADDR_PORT  ; 3 x 8
    .db 5,ADDR_PORT,5,ADDR_PORT,5,ADDR_PORT,5,ADDR_PORT,5,ADDR_PORT,5,ADDR_PORT,5,ADDR_PORT,5,ADDR_PORT  ; 4 x 8
    .db 5,ADDR_PORT,5,ADDR_PORT,5,ADDR_PORT,5,ADDR_PORT,5,ADDR_PORT,5,ADDR_PORT,5,ADDR_PORT,5,ADDR_PORT  ; 5 x 8
    .db 5,ADDR_PORT,5,ADDR_PORT,5,ADDR_PORT,5,ADDR_PORT,5,ADDR_PORT,5,ADDR_PORT,5,ADDR_PORT,5,ADDR_PORT  ; 6 x 8
    .db 5,ADDR_PORT,5,ADDR_PORT,5,ADDR_PORT,5,ADDR_PORT,5,ADDR_PORT,5,ADDR_PORT,5,ADDR_PORT,5,ADDR_PORT  ; 7 x 8
    .db 5,ADDR_PORT,5,ADDR_PORT,5,ADDR_PORT,5,ADDR_PORT,5,ADDR_PORT,5,ADDR_PORT,5,ADDR_PORT,5,ADDR_PORT  ; 8 x 8
    .db 5,ADDR_PORT,5,ADDR_PORT,5,ADDR_PORT,5,ADDR_PORT,5,ADDR_PORT,5,ADDR_PORT,5,ADDR_PORT,5,ADDR_PORT  ; 9 x 8
    .db 5,ADDR_PORT,5,ADDR_PORT,5,ADDR_PORT,5,ADDR_PORT,5,ADDR_PORT,5,ADDR_PORT,5,ADDR_PORT,5,ADDR_PORT  ; 10 x 8
    .db 5,ADDR_PORT,5,ADDR_PORT,5,ADDR_PORT,5,ADDR_PORT,5,ADDR_PORT,5,ADDR_PORT,5,ADDR_PORT,5,ADDR_PORT  ; 11 x 8
    .db 5,ADDR_PORT,5,ADDR_PORT,5,ADDR_PORT,5,ADDR_PORT,5,ADDR_PORT,5,ADDR_PORT,5,ADDR_PORT,5,ADDR_PORT  ; 12 x 8
    .db 5,ADDR_PORT,5,ADDR_PORT,5,ADDR_PORT,5,ADDR_PORT,5,ADDR_PORT,5,ADDR_PORT,5,ADDR_PORT,5,ADDR_PORT  ; 13 x 8
    .db 5,ADDR_PORT,5,ADDR_PORT,5,ADDR_PORT,5,ADDR_PORT,5,ADDR_PORT,5,ADDR_PORT,5,ADDR_PORT,5,ADDR_PORT  ; 14 x 8
    .db 5,ADDR_PORT,5,ADDR_PORT,5,ADDR_PORT,5,ADDR_PORT,5,ADDR_PORT,5,ADDR_PORT,5,ADDR_PORT,5,ADDR_PORT  ; 15 x 8
    .db 5,ADDR_PORT,5,ADDR_PORT,5,ADDR_PORT,5,ADDR_PORT,5,ADDR_PORT,5,ADDR_PORT,5,ADDR_PORT,5,ADDR_PORT  ; 16 x 8
__endasm ;
}

/*
 * Flexible copy from any flash address to the LCD
 */
void lcd_flash_copy(uint32_t addr, uint16_t pixel_count)
{
    for (;;) {
	uint8_t addr_high = (uint8_t)(addr >> 13) & 0xFE;
	uint8_t addr_mid  = (uint8_t)(addr >> 6) & 0xFE;
	uint8_t addr_low  = addr << 1;

	ADDR_PORT = addr_high;
	CTRL_PORT = CTRL_IDLE | CTRL_FLASH_LAT2;
	ADDR_PORT = addr_mid;
	CTRL_PORT = CTRL_FLASH_OUT | CTRL_FLASH_LAT1;
	ADDR_PORT = addr_low;

	if (pixel_count > 64 && addr_low == 0) {
	    lcd_addr_burst(32);
	    lcd_addr_burst(32);
	    pixel_count -= 64;
	    addr += 128;

	} else {
	    uint8_t burst_count = 32 - (addr_low >> 3);

	    if (burst_count >= pixel_count) {
		lcd_addr_burst(burst_count);
		CTRL_PORT = CTRL_IDLE;
		return;
	    } else {
		lcd_addr_burst(burst_count);
		pixel_count -= burst_count;
		addr += burst_count << 1;
	    }
	}
    }
}

/*
 * Optimized copy, assumes everything is perfectly aligned and always
 * blits a full screen of data.
 */
void lcd_flash_copy_fullscreen(uint32_t addr)
{
    uint8_t addr_high = (uint8_t)(addr >> 13) & 0xFE;
    uint8_t addr_mid  = (uint8_t)(addr >> 6) & 0xFE;
    uint8_t chunk = 0;

    do {
	uint8_t i = 8;  // 64 pixels, as eight 8-pixel chunks

	ADDR_PORT = addr_high;
	CTRL_PORT = CTRL_IDLE | CTRL_FLASH_LAT2;
	ADDR_PORT = addr_mid;
	CTRL_PORT = CTRL_FLASH_OUT | CTRL_FLASH_LAT1;
	ADDR_PORT = 0;

	do {
	    ADDR_PORT++; ADDR_PORT++; ADDR_PORT++; ADDR_PORT++;  // 0
	    ADDR_PORT++; ADDR_PORT++; ADDR_PORT++; ADDR_PORT++;  // 1
	    ADDR_PORT++; ADDR_PORT++; ADDR_PORT++; ADDR_PORT++;  // 2
	    ADDR_PORT++; ADDR_PORT++; ADDR_PORT++; ADDR_PORT++;  // 3
	    ADDR_PORT++; ADDR_PORT++; ADDR_PORT++; ADDR_PORT++;  // 4
	    ADDR_PORT++; ADDR_PORT++; ADDR_PORT++; ADDR_PORT++;  // 5
	    ADDR_PORT++; ADDR_PORT++; ADDR_PORT++; ADDR_PORT++;  // 6
	    ADDR_PORT++; ADDR_PORT++; ADDR_PORT++; ADDR_PORT++;  // 7
	} while (--i);

	if (!(addr_mid += 2)) addr_high += 2;
    } while (--chunk);
}

/*
 * Arbitrary-sized chromakey blit. If we hit a pixel that matches our
 * chromakey, we show data from a background image instead of the
 * foreground.
 */
void lcd_flash_chromakey(uint32_t fgAddr, uint32_t bgAddr, uint16_t pixel_count)
{
    do {
	uint8_t fg_high = (uint8_t)(fgAddr >> 13) & 0xFE;
	uint8_t fg_mid  = (uint8_t)(fgAddr >> 6) & 0xFE;
	uint8_t fg_low  = fgAddr << 1;

	uint8_t bg_high = (uint8_t)(bgAddr >> 13) & 0xFE;
	uint8_t bg_mid  = (uint8_t)(bgAddr >> 6) & 0xFE;
	uint8_t bg_low  = bgAddr << 1;

	// XXX: fg/bg may have different alignment
	uint8_t burst_count = 32 - (fg_low >> 3);
	uint8_t i;
	if (burst_count > pixel_count)
	    burst_count = pixel_count;

	ADDR_PORT = fg_high;
	CTRL_PORT = CTRL_IDLE | CTRL_FLASH_LAT2;
	ADDR_PORT = fg_mid;
	CTRL_PORT = CTRL_FLASH_OUT | CTRL_FLASH_LAT1;
	ADDR_PORT = fg_low;

	fgAddr += burst_count << 1;
	bgAddr += burst_count << 1;
	pixel_count -= burst_count;

	for (i = burst_count; i--;) {
	    if (BUS_PORT == CHROMA_KEY) {
		// Transparent pixel, swap addresses

		ADDR_PORT = bg_high;
		CTRL_PORT = CTRL_IDLE | CTRL_FLASH_LAT2;
		ADDR_PORT = bg_mid;
		CTRL_PORT = CTRL_FLASH_OUT | CTRL_FLASH_LAT1;
		ADDR_PORT = bg_low + ((burst_count - i - 1) << 2);
		
		ADDR_PORT++;
		ADDR_PORT++;
		ADDR_PORT++;
		ADDR_PORT++;

		ADDR_PORT = fg_high;
		CTRL_PORT = CTRL_IDLE | CTRL_FLASH_LAT2;
		ADDR_PORT = fg_mid;
		CTRL_PORT = CTRL_FLASH_OUT | CTRL_FLASH_LAT1;
		ADDR_PORT = fg_low + ((burst_count - i - 1) << 2);

	    } else {
		ADDR_PORT++;
		ADDR_PORT++;
		ADDR_PORT++;
		ADDR_PORT++;
	    }
	}
    } while (pixel_count);
}

/*
 * Optimized chroma-key blit. Assumes perfect alignment, and copies a
 * full screen of data.
 */
void lcd_flash_chromakey_fullscreen(uint32_t fgAddr, uint32_t bgAddr)
{
    uint8_t fg_high = (uint8_t)(fgAddr >> 13) & 0xFE;
    uint8_t fg_mid  = (uint8_t)(fgAddr >> 6) & 0xFE;
    
    uint8_t bg_high = (uint8_t)(bgAddr >> 13) & 0xFE;
    uint8_t bg_mid  = (uint8_t)(bgAddr >> 6) & 0xFE;

    uint8_t chunk = 0;

    ADDR_PORT = fg_high;
    CTRL_PORT = CTRL_IDLE | CTRL_FLASH_LAT2;
    ADDR_PORT = fg_mid;
    CTRL_PORT = CTRL_FLASH_OUT | CTRL_FLASH_LAT1;
    ADDR_PORT = 0;

    do {
	uint8_t cycle = 8;

	do {
	    // Partially unrolled loop, 64 = 8x8

#define CHROMA_TEST 							\
	    if (BUS_PORT == CHROMA_KEY) { 				\
		uint8_t a = ADDR_PORT; 					\
									\
		ADDR_PORT = bg_high; 					\
		CTRL_PORT = CTRL_IDLE | CTRL_FLASH_LAT2;		\
		ADDR_PORT = bg_mid; 					\
		CTRL_PORT = CTRL_FLASH_OUT | CTRL_FLASH_LAT1;		\
		ADDR_PORT = a; 						\
									\
		ADDR_PORT++; ADDR_PORT++; ADDR_PORT++;ADDR_PORT++;	\
									\
		ADDR_PORT = fg_high;					\
		CTRL_PORT = CTRL_IDLE | CTRL_FLASH_LAT2;		\
		ADDR_PORT = fg_mid;					\
		CTRL_PORT = CTRL_FLASH_OUT | CTRL_FLASH_LAT1;		\
		ADDR_PORT = a + 4;					\
	    } else {							\
		ADDR_PORT++; ADDR_PORT++; ADDR_PORT++; ADDR_PORT++;	\
	    }

	    CHROMA_TEST CHROMA_TEST CHROMA_TEST CHROMA_TEST
	    CHROMA_TEST CHROMA_TEST CHROMA_TEST CHROMA_TEST

#undef CHROMA_TEST

	} while (--cycle);

	if (!(fg_mid += 2)) fg_high += 2;
	if (!(bg_mid += 2)) bg_high += 2;

	// Must load the first pixel of the next chunk, for the colorkey test
	ADDR_PORT = fg_high;
	CTRL_PORT = CTRL_IDLE | CTRL_FLASH_LAT2;
	ADDR_PORT = fg_mid;
	CTRL_PORT = CTRL_FLASH_OUT | CTRL_FLASH_LAT1;
	ADDR_PORT = 0;

    } while (--chunk);
}

/*
 * Tile graphics!
 *
 * There are a lot of tile modes we could use. For example, the
 * 7x3=21-bit address bus is really well suited for a layout that has
 * 128 accessible 8x8 tiles at a time, since we then need to change
 * only one of the address bytes per-tile.
 *
 * However, even a relatively simple game like Chroma Shuffle couldn't
 * use that mode. with an 8x8 tile size, it would need at least 640
 * tiles, which is quite a bit beyond this hypothetical mode's
 * 128-tile limit.
 *
 * There are probably tons of uses for a 8x8 mode with a 16-bit tile
 * ID- for example, games that use a lot of text, and need to be able
 * to combine text with graphics.
 *
 * But for this demo, I'm going the other direction and implementing a
 * 16x16 mode with 8-bit tile IDs. Each tile occupies an aligned
 * 512-byte chunk of memory. The tilemap is 8x8, and only occupies 64
 * bytes.
 *
 * For simplicity, we're still requiring that a tile segment begins on
 * a 16kB boundary.
 *
 * This mode isn't especially suited for arbitrary text, but it's good
 * for puzzle games where the game pieces are relatively large.
 */
void lcd_render_tiles_16x16_8bit(uint8_t segment)
{
    uint8_t map_index = 0;
    uint8_t y_low = 0;
    uint8_t y_high = 0;

    do {
	// Loop over map columns
	do {
	    uint8_t tile_index = tilemap[map_index++];
		
	    ADDR_PORT = segment + ((tile_index >> 4) & 0x0E);
	    CTRL_PORT = CTRL_IDLE | CTRL_FLASH_LAT2;
	    ADDR_PORT = y_high + (tile_index << 3);
	    CTRL_PORT = CTRL_FLASH_OUT | CTRL_FLASH_LAT1;
	    ADDR_PORT = y_low;

	    // Burst out one row of one tile (16 pixels)

	    ADDR_PORT++; ADDR_PORT++; ADDR_PORT++; ADDR_PORT++; // 1
	    ADDR_PORT++; ADDR_PORT++; ADDR_PORT++; ADDR_PORT++; // 2
	    ADDR_PORT++; ADDR_PORT++; ADDR_PORT++; ADDR_PORT++; // 3
	    ADDR_PORT++; ADDR_PORT++; ADDR_PORT++; ADDR_PORT++; // 4
	    ADDR_PORT++; ADDR_PORT++; ADDR_PORT++; ADDR_PORT++; // 5
	    ADDR_PORT++; ADDR_PORT++; ADDR_PORT++; ADDR_PORT++; // 6
	    ADDR_PORT++; ADDR_PORT++; ADDR_PORT++; ADDR_PORT++; // 7
	    ADDR_PORT++; ADDR_PORT++; ADDR_PORT++; ADDR_PORT++; // 8
	    ADDR_PORT++; ADDR_PORT++; ADDR_PORT++; ADDR_PORT++; // 9
	    ADDR_PORT++; ADDR_PORT++; ADDR_PORT++; ADDR_PORT++; // 10
	    ADDR_PORT++; ADDR_PORT++; ADDR_PORT++; ADDR_PORT++; // 11
	    ADDR_PORT++; ADDR_PORT++; ADDR_PORT++; ADDR_PORT++; // 12
	    ADDR_PORT++; ADDR_PORT++; ADDR_PORT++; ADDR_PORT++; // 13
	    ADDR_PORT++; ADDR_PORT++; ADDR_PORT++; ADDR_PORT++; // 14
	    ADDR_PORT++; ADDR_PORT++; ADDR_PORT++; ADDR_PORT++; // 15
	    ADDR_PORT++; ADDR_PORT++; ADDR_PORT++; ADDR_PORT++; // 16
	    
	} while (map_index & 7);

	/*
	 * We can store four tile rows per address chunk.
	 * After that, roll over y_high. We'll do four such
	 * roll-overs per 16 pixel tile.
	 */

	y_low += 64;
	if (y_low)
	    map_index += 0xF8;  // No map advancement
	else {
	    y_high += 2;
	    if (y_high & 8)
		y_high = 0;
	    else
		map_index += 0xF8;  // No map advancement
	}
    } while (!(map_index & 64));
}

void gems_draw_gem(uint8_t x, uint8_t y, uint8_t index)
{
    // A gem is 32x32, a.k.a. a 2x2 tile grid
    index <<= 2;
    TILE(3, 0 + (x << 1), 0 + (y << 1)) = 0 + index;
    TILE(3, 1 + (x << 1), 0 + (y << 1)) = 1 + index;
    TILE(3, 0 + (x << 1), 1 + (y << 1)) = 2 + index;
    TILE(3, 1 + (x << 1), 1 + (y << 1)) = 3 + index;
}

uint32_t xor128(void)
{
    // Simple PRNG.
    // Reference: http://en.wikipedia.org/wiki/Xorshift

    static uint32_t x = 123456789;
    static uint32_t y = 362436069;
    static uint32_t z = 521288629;
    static uint32_t w = 88675123;
    uint32_t t;
 
    t = x ^ (x << 11);
    x = y; y = z; z = w;
    return w = w ^ (w >> 19) ^ (t ^ (t >> 8));
}

void gems_init()
{
    // Draw a uniform grid of gems, all alike.

    uint8_t x, y;

    for (y = 0; y < 4; y++)
	for (x = 0; x < 4; x++)
	    gems_draw_gem(x, y, 0);
}

void gems_shuffle()
{
    // Mix it up, replace a few pseudo-random gems.

    uint32_t r = xor128();
    uint8_t i = 4;

    do {
	uint8_t gem = (uint8_t)r % 40;
	uint8_t x = (r >> 6) & 3;
	uint8_t y = (r >> 4) & 3;

	gems_draw_gem(x, y, gem);

	r >>= 8;
    } while (--i);
}


/*
 * IT IS DEMO TIME.
 */

void main()
{
    hardware_init();

    while (1) {
	unsigned frame;

	// Background only
	if (1) {
	    for (frame = 0; frame < 256; frame++) {
		uint32_t bg_addr = 0x40000LU + ((uint32_t)(frame & 0xFF) << (LCD_ROW_SHIFT + 1));
		lcd_cmd_byte(LCD_CMD_RAMWR);
		lcd_flash_copy_fullscreen(bg_addr);
	    }
	}

	// Sprite only
	if (1) {
	    for (frame = 0; frame < 256; frame++) {
		uint8_t spr_f = (frame >> 2) & 7;
		uint32_t spr_addr = (uint32_t)spr_f << 15;
		lcd_cmd_byte(LCD_CMD_RAMWR);
		lcd_flash_copy_fullscreen(spr_addr);
	    }
	}
	
	// Chroma key
	if (1) {
	    for (frame = 0; frame < 256; frame++) {
		uint8_t spr_f = (frame >> 1) & 7;
		uint32_t spr_addr = (uint32_t)spr_f << 15;
		uint32_t bg_addr = 0x40000LU + ((uint32_t)(frame & 0x7F) << (LCD_ROW_SHIFT + 2));
		lcd_cmd_byte(LCD_CMD_RAMWR);
		lcd_flash_chromakey_fullscreen(spr_addr, bg_addr);
	    }
	}

	// Static 16x16 tile graphics (Chroma Extra-lite)
	if (1) {
	    gems_init();
	    for (frame = 0; frame < 256; frame++) {
		lcd_cmd_byte(LCD_CMD_RAMWR);
		lcd_render_tiles_16x16_8bit(0x34);
		gems_shuffle();
	    }
	}
    }
}
