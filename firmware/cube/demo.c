/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * Thundercracker cube firmware
 *
 * M. Elizabeth Scott <beth@sifteo.com>
 * Copyright <c> 2011 Sifteo, Inc. All rights reserved.
 */

#include <stdint.h>
#include "lcd.h"
#include "hardware.h"
#include "flash.h"
#include "draw.h"

extern const __code uint8_t img_logo[];
extern const __code uint8_t img_battery[];
extern const __code uint8_t img_power[];


// Signed 8-bit sin()
int8_t sin8(uint8_t angle)
{
    static const __code int8_t lut[] = {
	0x00, 0x03, 0x06, 0x09, 0x0c, 0x10, 0x13, 0x16,
	0x19, 0x1c, 0x1f, 0x22, 0x25, 0x28, 0x2b, 0x2e,
	0x31, 0x33, 0x36, 0x39, 0x3c, 0x3f, 0x41, 0x44,
	0x47, 0x49, 0x4c, 0x4e, 0x51, 0x53, 0x55, 0x58,
	0x5a, 0x5c, 0x5e, 0x60, 0x62, 0x64, 0x66, 0x68,
	0x6a, 0x6b, 0x6d, 0x6f, 0x70, 0x71, 0x73, 0x74,
	0x75, 0x76, 0x78, 0x79, 0x7a, 0x7a, 0x7b, 0x7c,
	0x7d, 0x7d, 0x7e, 0x7e, 0x7e, 0x7f, 0x7f, 0x7f,
    };

    if (angle & 0x80) {
	if (angle & 0x40)
	    return -lut[255 - angle];
	else
	    return -lut[angle & 0x3F];
    } else {
	if (angle & 0x40)
	    return lut[127 - angle];
	else
	    return lut[angle];
    }
}


void demo(void)
{
    draw_clear();
    draw_xy = XY(1,1);
    draw_attr = ATTR_RED;
    draw_string("Thunder");
    draw_attr = ATTR_ORANGE;
    draw_string("cracker");

    draw_xy = XY(1,5);
    draw_attr = ATTR_NONE;
    draw_image(img_logo);

    draw_xy = XY(0,11);
    draw_image(img_battery);
    draw_xy = XY(1,12);
    draw_image(img_power);

    draw_xy = XY(5,11);
    draw_attr = ATTR_RED;
    draw_image(img_battery);
    draw_xy = XY(6,12);
    draw_string("red");

    /*
     * Animation.. fun for testing, but it interferes with drawing
     * that the master is trying to do over the radio.
     */
#if 1
    {
	static uint16_t frame;
    
	while (1) {
	    int8_t x = sin8(frame << 1) >> 3;
	    int8_t y = sin8(frame << 3) >> 5;
	    if (x < 0) x += _SYS_VRAM_BG0_WIDTH * 8;
	    if (y < 0) y += _SYS_VRAM_BG0_WIDTH * 8;

	    vram.bg0_x = x;
	    vram.bg0_y = y;

	    draw_xy = XY(11,14);
	    draw_attr = ATTR_GRAY;
	    draw_hex(frame >> 8);
	    draw_hex(frame & 0xFF);

	    flash_handle_fifo();
	    graphics_render();

	    frame++;
	}
    }
#endif
}
