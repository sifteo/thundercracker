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

#define LCD_CMD_NOP      0x00
#define LCD_CMD_CASET    0x2A
#define LCD_CMD_RASET    0x2B
#define LCD_CMD_RAMWR    0x2C

#define LCD_WRX          P1_0
#define LCD_CSX          P2_2
#define LCD_DCX          P2_1
#define LCD_RDX          P2_0

#define FLASH_LAT2       P2_7
#define FLASH_LAT1       P2_6
#define FLASH_OE         P2_5
#define FLASH_CE         P2_4
#define FLASH_WE         P2_3


static void hardware_init(void)
{
    P1 = 0x00;  	// Address/strobe low
    P2 = 0x28;   	// Chipselects active, output enables inactive
    P0DIR = 0xFF;	// Shared bus, floating
    P1DIR = 0x00;  	// Driven address/control lines
    P2DIR = 0x00;   	// Driven control lines
}

static void lcd_cmd_byte(uint8_t b)
{
    LCD_DCX = 0;      	// Command mode
    P0DIR = 0x00;     	// Drive command onto bus
    P0 = b;
    LCD_WRX = 1;      	// Write strobe
    LCD_WRX = 0;
    P0DIR = 0xFF;     	// Release bus
    LCD_DCX = 1;      	// Back to data mode
}

static void lcd_data_byte(uint8_t b)
{
    P0DIR = 0x00;     	// Drive data onto bus
    P0 = b;
    LCD_WRX = 1;      	// Write strobe
    LCD_WRX = 0;
    P0DIR = 0xFF;     	// Release bus
}

static void lcd_data_from_flash(uint8_t page, uint8_t chunk, uint8_t c_start, uint8_t c_len)
{
    P0DIR = 0x00;	// Drive bus with high address byte
    P0 = page;
    FLASH_LAT2 = 1;     // Latch 2 strobe
    FLASH_LAT2 = 0;
    P0 = chunk;
    FLASH_LAT1 = 1;     // Latch 1 strobe
    FLASH_LAT1 = 0;
    P0DIR = 0xFF;       // Release bus
    FLASH_OE = 0;	// Let the flash drive the bus

    // P1 acts as a counter that holds 7 bits of address as well as our write strobe
    P1 = c_start;

    while (c_len--) {
	// High byte
	P1++;   // Write strobe high. Bam!
	P1++;   // Write strobe low, load next address

	// Low byte
	P1++;   // Write strobe high. Bam!
	P1++;   // Write strobe low, load next address
    }

    FLASH_OE = 1; 	// Release bus
}

static void lcd_ckey_data_from_flash(uint8_t page1, uint8_t chunk1, uint8_t c_start1,
				     uint8_t page2, uint8_t chunk2, uint8_t c_start2,
				     uint8_t c_len, uint8_t ckey)
{
    uint8_t i;
    
    P0DIR = 0x00;	// Drive bus with high address byte
    P0 = page1;
    FLASH_LAT2 = 1;     // Latch 2 strobe
    FLASH_LAT2 = 0;
    P0 = chunk1;
    FLASH_LAT1 = 1;     // Latch 1 strobe
    FLASH_LAT1 = 0;
    P0DIR = 0xFF;       // Release bus
    FLASH_OE = 0;	// Let the flash drive the bus

    P1 = c_start1;

    for (i = 0; i < c_len; i++) {
	if (P0 == ckey) {
	    // Transparent pixel
	    
	    /*
	     * Swap over to a different page/chunk (slow!)
	     *
	     * (LOTS of room for optimization here.. also reduce latch
	     * fluff by taking advantage of the edge-triggered
	     * behaviour. No need to clear latch enable bits
	     * immediately.)
	     */

	    FLASH_OE = 1;
	    P0DIR = 0x00;
	    P0 = page2;
	    FLASH_LAT2 = 1;
	    FLASH_LAT2 = 0;
	    P0 = chunk2;
	    FLASH_LAT1 = 1;
	    FLASH_LAT1 = 0;
	    P0DIR = 0xFF;
	    FLASH_OE = 0;
	    P1 = c_start2 + (i << 2);

	    P1++;
	    P1++;
	    P1++;
	    P1++;

	    FLASH_OE = 1;
	    P0DIR = 0x00;
	    P0 = page1;
	    FLASH_LAT2 = 1;
	    FLASH_LAT2 = 0;
	    P0 = chunk1;
	    FLASH_LAT1 = 1;
	    FLASH_LAT1 = 0;
	    P0DIR = 0xFF;
	    FLASH_OE = 0;
	    P1 = c_start1 + (i << 2);

	} else {
	    // Opaque pixel. Write normally.
	    P1++;
	    P1++;
	    P1++;
	    P1++;
	}
    }

    FLASH_OE = 1; 	// Release bus
}

void main()
{
    unsigned frame = 0;
    unsigned line = 0;
    
    hardware_init();

    while (1) {
	lcd_cmd_byte(LCD_CMD_RAMWR);
	frame++;

	// Background only
#if 0
	for (line = 0; line < 128; line++) {
	    unsigned bg_y = (line + (frame << 1)) & 511;
	    lcd_data_from_flash(8 + (bg_y >> 7), bg_y << 1, 0, 64);
	    lcd_data_from_flash(8 + (bg_y >> 7), 1 + (bg_y << 1), 0, 64);
	}
#endif

	// Sprite only
#if 0
	for (line = 0; line < 128; line++) {
	    lcd_data_from_flash(frame & 7, line << 1, 0, 64);
	    lcd_data_from_flash(frame & 7, 1 + (line << 1), 0, 64);
	}
#endif

	// Chroma key
#if 1
	for (line = 0; line < 128; line++) {
	    unsigned bg_y = (line + (frame << 1)) & 511;
	    unsigned spr_f = (frame >> 1) & 7;

	    lcd_ckey_data_from_flash(spr_f, line << 1, 0,
				     8 + (bg_y >> 7), bg_y << 1, 0,
				     64, 0xF5);
	    lcd_ckey_data_from_flash(spr_f, 1 + (line << 1), 0,
				     8 + (bg_y >> 7), 1 + (bg_y << 1), 0,
				     64, 0xF5);
	}
#endif
    }
}
