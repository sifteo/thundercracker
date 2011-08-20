/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * Sifteo prototype simulator
 * M. Elizabeth Scott <beth@sifteo.com>
 *
 * Copyright <c> 2011 Sifteo, Inc. All rights reserved.
 */

/*
 * XXX: Pick a specific flash memory model.
 * 
 * I've had my eye on the Numonyx/Micron M29W160ET70N6E.
 * It has a number of nice features:
 *
 *  - Less than $1 on Digi-key, in qty 100k
 *
 *  - 16 megabits, which is our maximum size using this 21-bit addressing technique
 *
 *  - 8-bit parallel interface, with random-access read
 *
 *  - Automatic standby. It will draw 4.5mA when "actively" reading, however
 *    it automatically enters a sub-100uA standby mode if the bus is idle for 150 nS.
 *    Our maximum read rate is (12 MHz / 3 cycles per inc / 2 incs per read) 2 MHz,
 *    which is a period of 500nS. So even if we're spending 100% of our CPU time
 *    reading the flash, it should draw an average of (150nS active / 500nS period *
 *    4.5mA full power + 0.1mA standby) 1.45 mA. Not too bad. And in practice, we'll
 *    be spending CPU time doing plenty of other things. We could integrate rough
 *    power usage monitoring into the simulation perhaps.
 */

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "flash.h"

/* Maximum flash size supported by our addressing scheme */
#define FLASH_SIZE   (1 << 21)
#define FLASH_MASK   (FLASH_SIZE - 1)

struct {
    FILE *file;
    uint8_t data[FLASH_SIZE];
} flash;

void flash_init(const char *filename)
{
    size_t result;

    flash.file = fopen(filename, "rb+");
    if (!flash.file) {
        flash.file = fopen(filename, "wb+");
    }
    if (!flash.file) {
	perror("Error opening flash");
	exit(1);
    }

    result =  fread(flash.data, 1, FLASH_SIZE, flash.file);
    if (result < 0) {
	perror("Error reading flash");
	exit(1);
    }	
}

void flash_exit(void)
{
    fclose(flash.file);
}

void flash_cycle(struct flash_pins *pins)
{
    // XXX: Read-only for now
    pins->data_drv = !pins->oe && !pins->ce;
    pins->data_out = flash.data[FLASH_MASK & pins->addr];
}
