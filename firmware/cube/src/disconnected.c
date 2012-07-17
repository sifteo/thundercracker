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
}

void disconnected_poll(void)
{
    /*
     * Update battery indicator.
     *
     * We don't draw the battery indicator at all until we've finished
     * sampling (battery_v is nonzero).
     */

    __asm
        mov     a, (_ack_data + RF_ACK_BATTERY_V)   ; Skip if we have no samples yet
        jz      2$

        mov     _draw_xy+0, #(XY(11,0) >> 0)        ; Draw battery outline
        mov     _draw_xy+1, #(XY(11,0) >> 8)
        mov     dptr, #_img_battery
        lcall   _draw_image

        mov     dptr, #_img_battery_bars_4          ; Pick a bars image based on the battery voltage
        mov     a, (_ack_data + RF_ACK_BATTERY_V)
        addc    a, #(256 - BATTERY_THRESHOLD_3)
        jc      1$
        mov     dptr, #_img_battery_bars_3
        addc    a, #(BATTERY_THRESHOLD_3 - BATTERY_THRESHOLD_2)
        jc      1$
        mov     dptr, #_img_battery_bars_2
        addc    a, #(BATTERY_THRESHOLD_2 - BATTERY_THRESHOLD_1)
        jc      1$
        mov     dptr, #_img_battery_bars_1
1$:     

        mov     _draw_xy+0, #(XY(12,1) >> 0)        ; Draw the correct battery bars image
        mov     _draw_xy+1, #(XY(12,1) >> 8)
        lcall   _draw_image

2$:
    __endasm ;

    /*
     * Update sleep timer
     */

    power_idle_poll();
}

