/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * Thundercracker cube firmware
 *
 * M. Elizabeth Scott <beth@sifteo.com>
 * Copyright <c> 2011 Sifteo, Inc. All rights reserved.
 */

#include <stdint.h>
#include "graphics.h"
#include "hardware.h"
#include "flash.h"
#include "draw.h"
#include "radio.h"
#include "touch.h"

extern const __code uint8_t img_logo[];
extern const __code uint8_t img_battery[];
extern const __code uint8_t img_power[];
extern const __code uint8_t img_radio_0[];
extern const __code uint8_t img_radio_1[];
extern const __code uint8_t img_radio_2[];
extern const __code uint8_t img_radio_3[];

void demo(void)
{
    draw_clear();

    draw_xy = XY(1,12);
    draw_attr = ATTR_RED;
    draw_string("Thunder");
    draw_attr = ATTR_ORANGE;
    draw_string("cracker");

    draw_xy = XY(1,5);
    draw_attr = ATTR_NONE;
    draw_image(img_logo);

#ifdef DEBUG_TOUCH
    draw_xy = XY(1,14);
    draw_string("DEBUG_TOUCH");

    while (1) {
        uint8_t i;
        uint8_t d = debug_touch;

        draw_xy = XY(1,2);
        draw_hex(d);

        for (i = 0; i < 18; i++)
            vram.bg0_tiles[i] = d < (i << 4) ? 0 : 0x26fe;

        graphics_render();
    }
#endif

    draw_xy = XY(0,0);
    draw_image(img_battery);
    draw_xy = XY(1,1);
    draw_image(img_power);

    draw_xy = XY(12,0);
    draw_image(img_radio_0);
    draw_xy = XY(12,0);
    draw_image(img_radio_3);

    draw_xy = XY(14,15);
    draw_attr = ATTR_GRAY;
    draw_hex(radio_get_cube_id());

    graphics_render();
    draw_exit();
}
