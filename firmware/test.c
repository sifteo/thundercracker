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


void hardware_init(void)
{
    BUS_DIR = 0xFF;

    ADDR_PORT = 0;
    ADDR_DIR = 0;

    CTRL_PORT = CTRL_IDLE;
    CTRL_DIR = 0;
}

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


void main()
{
    hardware_init();

    while (1) {
	unsigned frame;

	// Background only
	if (1)
	    for (frame = 0; frame < 256; frame++) {
		uint32_t bg_addr = 0x40000LU + ((uint32_t)(frame & 0xFF) << (LCD_ROW_SHIFT + 1));
		lcd_cmd_byte(LCD_CMD_RAMWR);
		lcd_flash_copy_fullscreen(bg_addr);
	    }

	// Sprite only
	if (1)
	    for (frame = 0; frame < 256; frame++) {
		uint8_t spr_f = (frame >> 2) & 7;
		uint32_t spr_addr = (uint32_t)spr_f << 15;
		lcd_cmd_byte(LCD_CMD_RAMWR);
		lcd_flash_copy_fullscreen(spr_addr);
	    }
	
	// Chroma key
	if (1)
	    for (frame = 0; frame < 256; frame++) {
		uint8_t spr_f = (frame >> 1) & 7;
		uint32_t spr_addr = (uint32_t)spr_f << 15;
		uint32_t bg_addr = 0x40000LU + ((uint32_t)(frame & 0x7F) << (LCD_ROW_SHIFT + 2));
		lcd_cmd_byte(LCD_CMD_RAMWR);
		lcd_flash_chromakey_fullscreen(spr_addr, bg_addr);
	    }
    }
}
