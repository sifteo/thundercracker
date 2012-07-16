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
extern const __code uint8_t img_battery_bars_1[];
extern const __code uint8_t img_battery_bars_2[];
extern const __code uint8_t img_battery_bars_3[];
extern const __code uint8_t img_battery_bars_4[];


void disconnected_init(void)
{
    draw_clear();

    draw_xy = XY(1,5);
    draw_image(img_logo);

    draw_xy = XY(11,0);
    draw_image(img_battery);

    draw_xy = XY(12,1);
    draw_image(img_battery_bars_1);
}


void disconnected_poll(void)
{
    power_idle_poll();
}
