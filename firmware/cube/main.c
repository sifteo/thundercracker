/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * Thundercracker cube firmware
 *
 * M. Elizabeth Scott <beth@sifteo.com>
 * Copyright <c> 2011 Sifteo, Inc. All rights reserved.
 */

#include <stdint.h>
#include "radio.h"
#include "sensors.h"
#include "lcd.h"
#include "hardware.h"
#include "flash.h"

void main(void)
{
    lcd_init();
    sensors_init();
    radio_init();
    flash_init();
    sti();

    /*
     * XXX: Draw idle-mode screen. For now, we're just testing _SYS_VM_BG0_ROM.
     */
    {
	uint8_t x;
	const char __code *title = " Thundercracker    prototype!     ";

	vram.mode = _SYS_VM_BG0_ROM;
	vram.flags = _SYS_VF_CONTINUOUS;

	x = 0;
	do {
	    vram.bg0_tiles[(x & 0xF) + (x >> 4)*18] = (uint8_t)(x << 1) | ((x & 0x80) << 2) | 0x8800;
	} while (--x);

	x = 0;
	while (*title) {
	    vram.bg0_tiles[x++] = (*title - ' ') << 1;
	    title++;
	}
    }

    while (1) {
	flash_handle_fifo();
	graphics_render();
    }
}
