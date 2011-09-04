/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * Thundercracker cube firmware
 *
 * M. Elizabeth Scott <beth@sifteo.com>
 * Copyright <c> 2011 Sifteo, Inc. All rights reserved.
 */

#include "flash.h"

uint8_t flash_addr_low;
uint8_t flash_addr_lat1;
uint8_t flash_addr_lat2;

#define FLASH_CMD_PREFIX(_addr, _dat)		\
    ADDR_PORT = ((_addr) >> 6) & 0xFE;		\
    CTRL_PORT = CTRL_IDLE | CTRL_FLASH_LAT1;	\
    ADDR_PORT = (_addr) << 1;			\
    BUS_PORT = (_dat);				\
    CTRL_PORT = CTRL_FLASH_CMD;			\

void flash_wait(void)
{
    /*
     * While the flash is busy, it will toggle status bits on the data
     * bus. If we get two consecutive reads which are identical, the
     * operation is complete.
     *
     * Toggle bits are updated on falling edges of OE, so we do have to
     * keep toggling CTRL_PORT between CTRL_IDLE and CTRL_FLASH_OUT.
     */

    uint8_t x, y;

    CTRL_PORT = CTRL_FLASH_OUT;
    x = BUS_PORT;
    CTRL_PORT = CTRL_IDLE;

    do {
	CTRL_PORT = CTRL_FLASH_OUT;
	y = x;
	x = BUS_PORT;
	CTRL_PORT = CTRL_IDLE;
    } while (x != y);
}

void flash_erase_chip(void)
{
    flash_wait();

    BUS_DIR = 0;
    FLASH_CMD_PREFIX(0xAAA, 0xAA);
    FLASH_CMD_PREFIX(0x555, 0x55);
    FLASH_CMD_PREFIX(0xAAA, 0x80);
    FLASH_CMD_PREFIX(0xAAA, 0xAA);
    FLASH_CMD_PREFIX(0x555, 0x55);
    FLASH_CMD_PREFIX(0xAAA, 0x10);
    BUS_DIR = 0xFF;
}

void flash_program_byte(uint8_t dat)
{
    ADDR_PORT = flash_addr_lat2;
    CTRL_PORT = CTRL_IDLE | CTRL_FLASH_LAT2;

    flash_wait();

    BUS_DIR = 0;
    FLASH_CMD_PREFIX(0xAAA, 0xAA);
    FLASH_CMD_PREFIX(0x555, 0x55);
    FLASH_CMD_PREFIX(0xAAA, 0xA0);

    ADDR_PORT = flash_addr_lat1;
    CTRL_PORT = CTRL_IDLE | CTRL_FLASH_LAT1;
    ADDR_PORT = flash_addr_low;
    BUS_PORT = dat;
    CTRL_PORT = CTRL_FLASH_CMD;
    BUS_DIR = 0xFF;

    if (!(flash_addr_low += 2))
	if (!(flash_addr_lat1 += 2))
	    flash_addr_lat2 += 2;
}
