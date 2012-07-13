/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * Thundercracker cube firmware
 *
 * M. Elizabeth Scott <beth@sifteo.com>
 * Copyright <c> 2011 Sifteo, Inc. All rights reserved.
 */

#include "graphics.h"
#include "draw.h"

extern const __code uint8_t img_logo[];
extern const __code uint8_t img_battery[];

/*
 * Update the screen, showing the "disconnected" animation.
 * Called from the main loop, when we aren't connected to anybody yet.
 */

void disconnected_screen(void)
{
    draw_clear();

    #ifdef CUBE_CHAN
        draw_xy = XY(0,15);
        draw_attr = ATTR_GRAY;
        draw_hex(CUBE_CHAN);
    #endif

    draw_xy = XY(14,15);
    draw_attr = ATTR_GRAY;
    draw_hex(radio_get_cube_id());

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
}
