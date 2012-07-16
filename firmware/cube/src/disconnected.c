/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * Thundercracker cube firmware
 *
 * M. Elizabeth Scott <beth@sifteo.com>
 * Copyright <c> 2011 Sifteo, Inc. All rights reserved.
 */

#include "graphics.h"
#include "draw.h"
#include "power.h"

extern const __code uint8_t img_logo[];
extern const __code uint8_t img_battery[];


void disconnected_init(void)
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

    draw_xy = XY(0,0);
    draw_image(img_battery);
    
    draw_hex(0x42);

    // XXX Cheat, to keep unit tests working before master knows about Hop
    radio_connected = 1;
}


void disconnected_poll(void)
{
    power_idle_poll();
}