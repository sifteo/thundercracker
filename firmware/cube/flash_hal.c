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
 * SST39VF1681. But we expect to have to modify this to suit whatever
 * commodity flash memory we source for the product.
 */

#include "flash.h"

uint8_t flash_addr_low;
uint8_t flash_addr_lat1;
uint8_t flash_addr_lat2;
uint8_t flash_run_len;

/*
 * We can poll in a much tigher loop (and therefore exit the polling
 * with lower latency if we use Data# polling as opposed to toggle
 * polling, since we don't need to pulse the OE pin. This is probably
 * also rather lower power consumption, since we aren't toggling an
 * external pin in a tight loop!
 *
 * This polling is based on examining the most significant bit of the
 * data byte. While busy, it will be the inverse of the true data.
 *
 * XXX: This can hang forever if we try to program a '0' bit to '1'!
 *      We probably want to detect this and other flash errors via some
 *      kind of watchdog?
 */

static __bit flash_poll_data;  // What data bit are we expecting?


#define FLASH_CMD_PREFIX(_addr, _dat)		\
    ADDR_PORT = ((_addr) >> 6) & 0xFE;		\
    CTRL_PORT = CTRL_IDLE | CTRL_FLASH_LAT1;	\
    ADDR_PORT = (_addr) << 1;			\
    BUS_PORT = (_dat);				\
    CTRL_PORT = CTRL_FLASH_CMD;			\


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
     *
     * Erase operations are self-contained: we do all address setup
     * (including lat2) and we wait for the erase to finish.
     */

    if (flash_addr_low)
	return;
    if (flash_addr_lat1)
	return;
    if (flash_addr_lat2 & 3)
	return;

    CTRL_PORT = CTRL_IDLE;

    for (;;) {
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
	    BUS_DIR = 0xFF;
	    CTRL_PORT = CTRL_FLASH_OUT;

	    // Data# polling: Wait for a '1' bit
	    __asm  1$:	jnb	BUS_PORT.7, 1$  __endasm;

	} else {
	    // Single block
	    ADDR_PORT = flash_addr_lat2;
	    CTRL_PORT = CTRL_IDLE | CTRL_FLASH_LAT2;
	    ADDR_PORT = 0;
	    BUS_PORT = 0x30;
	    CTRL_PORT = CTRL_FLASH_CMD;
	    BUS_DIR = 0xFF;
	    CTRL_PORT = CTRL_FLASH_OUT;

	    __asm  2$:	jnb	BUS_PORT.7, 2$  __endasm;

	    if (!blockCount)
		break;
	    
	    blockCount--;
	    flash_addr_lat2 += 4;
	}
    }

    flash_program_start();
}

void flash_program_start(void)
{
    /*
     * Perform infrequent initialization steps that only need to occur
     * before a batch of program operations, and after any intervening
     * flash reads.
     */

    // We can keep LAT2 loaded, since the prefix sequences don't use it.
    ADDR_PORT = flash_addr_lat2;
    CTRL_PORT = CTRL_FLASH_OUT | CTRL_FLASH_LAT2;

    // Set flash_poll_data to whatever data is at the current address,
    // so we don't have to special-case the first program operation.

    __asm
	mov	c, BUS_PORT.7
	mov	_flash_poll_data, c
    __endasm ;
}    

void flash_program_words(uint16_t dat) __naked
{
    /*
     * Program two bytes, at any aligned address. Increment the
     * address. Then repeat all this, a total of "flash_run_len" times.
     *
     * The run length must be at least 1.
     * The bytes MUST have been erased first.
     *
     * This routine is highly performance critical itself, plus we
     * need to be careful about its impact on the flash decoder code.
     * This routine is written without any temporary registers, and we
     * use "#pragma callee_saves" to allow SDCC to notice this and
     * omit unnecessary save/restore code.
     */

    dat = dat;

    /*
     * Low byte
     */

    // Wait on the previous word-write
    __asm
	jnb	_flash_poll_data, 2$
3$:	jnb	BUS_PORT.7, 3$
        sjmp	1$
2$:	jb	BUS_PORT.7, 2$
1$:			   
    __endasm ;

    CTRL_PORT = CTRL_IDLE;
    BUS_DIR = 0;

    // Write unlock prefix
    FLASH_CMD_PREFIX(0xAAA, 0xAA);
    FLASH_CMD_PREFIX(0x555, 0x55);
    FLASH_CMD_PREFIX(0xAAA, 0xA0);

    // Write byte
    __asm
	mov	ADDR_PORT, _flash_addr_lat1
	mov	CTRL_PORT, #(CTRL_IDLE | CTRL_FLASH_LAT1)
	mov	ADDR_PORT, _flash_addr_low
	mov	BUS_PORT, DPL
	mov	CTRL_PORT, #CTRL_FLASH_CMD
    __endasm ;
    CTRL_PORT = CTRL_FLASH_CMD;
    BUS_DIR = 0xFF;
    CTRL_PORT = CTRL_FLASH_OUT;

    /*
     * We can do other useful work in-between bytes. This is
     * effectively free, since we'd be waiting anyway.
     */

    flash_addr_low += 2;

    // Calculate the next flash_poll_data flag.
    __asm
	mov	a, DPH
	rlc	a
	mov	_flash_poll_data, c
    __endasm ; 

    /*
     * High byte
     */

   // Wait for the low byte to finish
    __asm
	mov	a, DPL
	rlc	a
	jnc	5$

6$:	jnb	BUS_PORT.7, 6$
        sjmp	7$

5$:	jb	BUS_PORT.7, 5$
7$:			   
    __endasm ;

    CTRL_PORT = CTRL_IDLE;
    BUS_DIR = 0;

    // Write unlock prefix
    FLASH_CMD_PREFIX(0xAAA, 0xAA);
    FLASH_CMD_PREFIX(0x555, 0x55);
    FLASH_CMD_PREFIX(0xAAA, 0xA0);

    // Write data byte, without any temporary registers
    __asm
	mov	ADDR_PORT, _flash_addr_lat1
	mov	CTRL_PORT, #(CTRL_IDLE | CTRL_FLASH_LAT1)
	mov	ADDR_PORT, _flash_addr_low
	mov	BUS_PORT, DPH
    __endasm ;
    CTRL_PORT = CTRL_FLASH_CMD;
    BUS_DIR = 0xFF;
    CTRL_PORT = CTRL_FLASH_OUT;

    // Increment flash_addr on our way out, without any temporaries
    __asm
	; Common case, no overflow
	mov	a, _flash_addr_low
	add	a, #2
	mov	_flash_addr_low, a
	jz	11$

	; Loop over run length
13$:
	djnz	_flash_run_len, 14$
	ret
14$:	ljmp	_flash_program_words


	; Low byte overflow 
11$:
	mov	a, _flash_addr_lat1
	add	a, #2
	mov	_flash_addr_lat1, a
	jz	12$
	sjmp	13$

        ; Lat1 overflow
12$:
	mov	a, _flash_addr_lat2
	add	a, #2
	mov     _flash_addr_lat2, a

        ; Since we keep lat2 loaded into the latch, we need to at some point
        ; reload the latch before the next write. But we must not touch the
	; latch while the flash is busy! In the interest of keeping the common
	; case fast, just wait for the current write to finish then update lat2.

	jnb	_flash_poll_data, 10$
8$:	jnb	BUS_PORT.7, 8$
        sjmp	9$
10$:	jb	BUS_PORT.7, 10$
9$:			   

	mov	ADDR_PORT, a
	mov	CTRL_PORT, #(CTRL_IDLE | CTRL_FLASH_LAT2)

	mov	c, BUS_PORT.7
	mov	_flash_poll_data, c

	sjmp	13$

    __endasm ;
}
