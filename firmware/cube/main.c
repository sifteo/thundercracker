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
#include "draw.h"

extern const __code uint8_t img_logo[];
extern const __code uint8_t img_battery[];
extern const __code uint8_t img_power[];

void main(void)
{
    lcd_init();
    sensors_init();
    radio_init();
    flash_init();
    sti();

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

    draw_xy = XY(14,15);
    draw_attr = ATTR_GRAY;
    draw_hex(0xF5);

    while (1) {
	flash_handle_fifo();
	graphics_render();
    }
}
