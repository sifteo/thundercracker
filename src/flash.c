/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * Sifteo prototype simulator
 * M. Elizabeth Scott <beth@sifteo.com>
 *
 * Copyright <c> 2011 Sifteo, Inc. All rights reserved.
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
