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
#include "graphics.h"
#include "hardware.h"
#include "flash.h"

void main(void)
{
    graphics_init();
    sensors_init();
    radio_init();
    sti();

    while (1) {

	/*
	 * XXX: torture the flash a bit!
	 */
	flash_erase(0);
	{
	    uint8_t i;
	    for (i = 0; i < 128; i++)
		flash_program(i);
	}

	// Sync with master
	// XXX disabled, see refresh_alt()
	// while (vram.frame_trigger == ack_data.frame_count);

	// Sync with LCD
	//while (!CTRL_LCD_TE);
	
	graphics_render();
	ack_data.frame_count++;
    }
}
