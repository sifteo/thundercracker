/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * Thundercracker cube firmware
 *
 * M. Elizabeth Scott <beth@sifteo.com>
 * Copyright <c> 2011 Sifteo, Inc. All rights reserved.
 */

#include <stdint.h>
#include "graphics.h"
#include "cube_hardware.h"
#include "flash.h"
#include "draw.h"
#include "radio.h"
#include "sensors.h"
#include "power.h"

extern const __code uint8_t img_logo[];
extern const __code uint8_t img_battery[];
extern const __code uint8_t img_power[];
extern const __code uint8_t img_radio_0[];
extern const __code uint8_t img_radio_1[];
extern const __code uint8_t img_radio_2[];
extern const __code uint8_t img_radio_3[];

#if defined(DEBUG_NBR) || defined(DEBUG_TOUCH) || defined(DEBUG_ACC)
#define	DEBUG_SCREEN 1
#endif

void demo(void)
{
#ifdef DEBUG_SCREEN
	volatile uint8_t dbg_cnt=0;
#endif

    draw_clear();

    #ifdef CUBE_CHAN
    	draw_xy = XY(0,15);
    	draw_attr = ATTR_GRAY;
    	draw_hex(CUBE_CHAN);
	#endif

    draw_xy = XY(14,15);
    draw_attr = ATTR_GRAY;
    draw_hex(radio_get_cube_id());

#ifdef DEBUG_SCREEN
    draw_attr = ATTR_NONE;
    while(1) {

    	power_wdt_set();	//reset watch-dog in debug mode loop

    	draw_xy = XY(14,0);
    	draw_hex(dbg_cnt++);

	#ifdef DEBUG_TOUCH
    	draw_xy = XY(1,13);
    	if( touch ) {
    		draw_string("Touch!");
    	} else {
    		draw_string("      ");
    	}
    	draw_xy = XY(8,13);
    	draw_hex(touch_count);
	#endif

	#ifdef DEBUG_NBR
    	draw_xy = XY(2,14);
    	draw_hex(nbr_data[0]);
    	draw_xy = XY(5,14);
    	draw_hex(nbr_data[1]);
    	draw_xy = XY(8,14);
    	draw_hex(nbr_data[2]);
    	draw_xy = XY(11,14);
    	draw_hex(nbr_data[3]);
    	draw_xy = XY(5,15);
    	draw_hex(nbr_data_valid);
    	draw_xy = XY(8,15);
    	draw_hex(nbr_data_invalid);
	#endif

    	graphics_render();
    }

#else

    draw_xy = XY(1,12);
    draw_attr = ATTR_RED;
    draw_string("Thunder");
    draw_attr = ATTR_ORANGE;
    draw_string("cracker");

    draw_xy = XY(1,5);
    draw_attr = ATTR_NONE;
    draw_image(img_logo);

    draw_xy = XY(0,0);
    draw_image(img_battery);
    draw_xy = XY(1,1);
    draw_image(img_power);

    draw_xy = XY(12,0);
    draw_image(img_radio_0);
    draw_xy = XY(12,0);
    draw_image(img_radio_3);

    graphics_render();

    draw_exit();

    #ifdef DEBUG_FLASH
        vram.mode = _SYS_VM_BG0;
        vram.flags = _SYS_VF_CONTINUOUS;
    #endif

#endif
}
