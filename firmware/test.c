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

static void lcd_data_from_flash(uint32_t flash_addr, unsigned len)
{
    P0DIR = 0x00;	// Drive bus with high address byte
    P0 = flash_addr >> 15;
    FLASH_LAT2 = 1;     // Latch 2 strobe
    FLASH_LAT2 = 0;
    P0 = flash_addr >> 7;
    FLASH_LAT1 = 1;     // Latch 1 strobe
    FLASH_LAT1 = 0;
    P0DIR = 0xFF;       // Release bus
    FLASH_OE = 0;	// Let the flash drive the bus

    // P1 acts as a counter that holds 7 bits of address as well as our write strobe
    P1 = flash_addr << 1;

    // It takes 2x as many increment cycles as we have bytes to write
    len <<= 1;

    while (len--)
	P1++;

    FLASH_OE = 1; 	// Release bus
}


void main()
{
    hardware_init();
    while (1) {
	lcd_cmd_byte(LCD_CMD_RAMWR);
	lcd_data_from_flash(0x1000, 63);
    }
}
