/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * Thundercracker cube firmware
 *
 * M. Elizabeth Scott <beth@sifteo.com>
 * Copyright <c> 2011 Sifteo, Inc. All rights reserved.
 */

/*
 * Flash hardware abstraction layer.
 *
 * Currently this is written for SST flashes, specifically the
 * SST39V1681. But we expect to have to modify this to suit whatever
 * commodity flash memory we source for the product.
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
     * Toggle bits are updated on falling edges of OE, so we do have
     * to keep toggling CTRL_PORT between CTRL_IDLE and
     * CTRL_FLASH_OUT.
     *
     * This is in inline assembly primarily to reduce the register
     * footprint.  We can do this using only ACC, but sdcc insists on
     * using an additional register that ends up getting
     * pushed/popped.  The same code is inlined below, in
     * flash_program, for the same reason.
     */

    __asm
	mov	CTRL_PORT, #CTRL_FLASH_OUT
1$:	mov	a, BUS_PORT
	mov	CTRL_PORT, #CTRL_IDLE
	mov	CTRL_PORT, #CTRL_FLASH_OUT
	cjne	a, BUS_PORT, 1$
	mov	CTRL_PORT, #CTRL_IDLE
    __endasm ;
}

void flash_erase(uint8_t blockCount)
{
    /*
     * Erase a number of blocks, or the entire chip.
     *
     * blockCount is as specified in the load stream, as a count of
     * 64K blocks minus one.  I.e. flash_erase(0) erases one block,
     * flash_erase(1) erases two, and flash_erase(FLASH_NUM_BLOCKS -
     * 1) or higher would do a full-chip erase, assuming the current
     * address is zero.
     *
     * Erases are ignored if the address is unaligned. This is one of
     * our safety measures against corrupted loadstreams causing
     * repeated erases which could wear out the flash quickly.
     */

    if (flash_addr_low)
	return;
    if (flash_addr_lat1)
	return;
    if (flash_addr_lat2 & 3)
	return;

    ADDR_PORT = flash_addr_lat2;
    CTRL_PORT = CTRL_IDLE | CTRL_FLASH_LAT2;

    for (;;) {
	flash_wait();
	BUS_DIR = 0;

	// Common unlock prefix for all erase ops
	FLASH_CMD_PREFIX(0xAAA, 0xAA);
	FLASH_CMD_PREFIX(0x555, 0x55);
	FLASH_CMD_PREFIX(0xAAA, 0x80);
	FLASH_CMD_PREFIX(0xAAA, 0xAA);
	FLASH_CMD_PREFIX(0x555, 0x55);

	if (blockCount >= (FLASH_NUM_BLOCKS - 1) && flash_addr_lat2 == 0) {
	    // Whole-chip erase
	    FLASH_CMD_PREFIX(0xAAA, 0x10);
	    CTRL_PORT = CTRL_IDLE;
	    break;

	} else {
	    // Single block
	    ADDR_PORT = flash_addr_lat2;
	    CTRL_PORT = CTRL_IDLE | CTRL_FLASH_LAT2;
	    ADDR_PORT = 0;
	    BUS_PORT = 0x30;
	    CTRL_PORT = CTRL_FLASH_CMD;
	    CTRL_PORT = CTRL_IDLE;

	    if (!blockCount)
		break;
	    
	    blockCount--;
	    flash_addr_lat2 += 4;
	}

	BUS_DIR = 0xFF;
    }
    BUS_DIR = 0xFF;
}

void flash_program(uint8_t dat)
{
    /*
     * Program one byte, at any address. This effectively bitwise ANDs
     * 'dat' with the current flash address, and increments the
     * address.
     *
     * This is partly in inline asm so that we can more carefully
     * control how registers are used. That, plus the "#pragma
     * callee_saves", makes this function much more efficient for the
     * decoder to call frequently.
     */

    // lat2 isn't used by CMD_PREFIX, so we can set it while we wait.
    ADDR_PORT = flash_addr_lat2;
    CTRL_PORT = CTRL_IDLE | CTRL_FLASH_LAT2;

    // flash_wait(), inlined
    __asm
	mov	CTRL_PORT, #CTRL_FLASH_OUT
	1$:
	mov	a, BUS_PORT
	mov	CTRL_PORT, #CTRL_IDLE
	mov	CTRL_PORT, #CTRL_FLASH_OUT
	cjne	a, BUS_PORT, 1$
	mov	CTRL_PORT, #CTRL_IDLE
    __endasm ;

    // Write unlock prefix
    BUS_DIR = 0;
    FLASH_CMD_PREFIX(0xAAA, 0xAA);
    FLASH_CMD_PREFIX(0x555, 0x55);
    FLASH_CMD_PREFIX(0xAAA, 0xA0);

    // Write data byte, without any temporary registers
    dat = dat;
    __asm
	mov	ADDR_PORT, _flash_addr_lat1
	mov	CTRL_PORT, #(CTRL_IDLE | CTRL_FLASH_LAT1)
	mov	ADDR_PORT, _flash_addr_low
	mov	BUS_PORT, DPL
    __endasm ;
    CTRL_PORT = CTRL_FLASH_CMD;
    CTRL_PORT = CTRL_IDLE;
    BUS_DIR = 0xFF;

    // Increment flash_addr, without any temporaries
    __asm
	mov	a, _flash_addr_low
	add	a, #2
	mov	_flash_addr_low, a
	jnz	2$
	mov	a, _flash_addr_lat1
	add	a, #2
	mov	_flash_addr_lat1, a
	jnz	2$
	inc	_flash_addr_lat2
	inc	_flash_addr_lat2
	2$:
    __endasm ;
}
