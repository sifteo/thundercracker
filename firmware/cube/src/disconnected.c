/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * Thundercracker cube firmware
 *
 * M. Elizabeth Scott <beth@sifteo.com>
 * Copyright <c> 2011 Sifteo, Inc. All rights reserved.
 */

#include <stdint.h>
#include "graphics.h"
#include "draw.h"
#include "sensors.h"
#include "power.h"

extern __bit disc_battery_draw;     // 0 = drawing logo, 1 = drawing battery

#define BATTERY_HEIGHT  19          // Number of scanlines at the top of the display reserved for battery
#define FP_BITS         4           // Fixed point precision, in bits

// Arbitrary horizontal limits, let the logo get off-screen a bit without wrapping
#define X_MIN           ((-20) << FP_BITS)
#define X_MAX           (( 20) << FP_BITS)

// Hard vertical limits (battery clipping, bottom screen edge)
#define Y_MIN           ((BATTERY_HEIGHT - 48) << FP_BITS)
#define Y_MAX           ((BATTERY_HEIGHT + 23) << FP_BITS)

extern int16_t disc_logo_x;         // State for bouncing logo, in 16-bit fixed point math
extern int16_t disc_logo_y;
extern int16_t disc_logo_dx;
extern int16_t disc_logo_dy;

extern const __code uint8_t img_logo[];
extern const __code uint8_t img_battery[];
extern const __code uint8_t img_battery_bars_1[];
extern const __code uint8_t img_battery_bars_2[];
extern const __code uint8_t img_battery_bars_3[];
extern const __code uint8_t img_battery_bars_4[];


static void draw_logo(void) __naked
{
    // Draw our logo at the standard position
    __asm
        DRAW_XY (1,5)
        mov     dptr, #_img_logo
        ljmp    _draw_image
    __endasm ;
}

void disconnected_init(void)
{
    /*
     * Initialize disconnected-mode state
     */

    disc_battery_draw = 0;

    disc_logo_x = 0;
    disc_logo_y = BATTERY_HEIGHT << FP_BITS;
    disc_logo_dx = 0;
    disc_logo_dy = 0;

    /*
     * First frame: Update the whole screen, but draw only the
     * logo. If we do either of our usual partial-screen updates,
     * the first frame will have a brief flash of garbage on it.
     */

    draw_clear();
    draw_logo();
    vram.num_lines = 128;
}

void disconnected_poll(void)
{
    /*
     * Update sleep timer
     */

    power_idle_poll();

    if (disc_battery_draw) {
        /*
         * Update battery indicator.
         *
         * We don't draw the battery indicator at all until we've finished
         * sampling (battery_v is nonzero).
         */

        disc_battery_draw = 0;

        draw_clear();
        vram.num_lines = BATTERY_HEIGHT;

        __asm
            mov     a, (_ack_data + RF_ACK_BATTERY_V)   ; Skip if we have no samples yet
            jz      2$

            DRAW_XY (11, 0)                             ; Draw battery outline
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

            DRAW_XY (12, 1)                             ; Draw the correct battery bars image
            lcall   _draw_image

    2$:
        __endasm ;
        return;
    }

    /*
     * Update the rest of the screen (bouncing logo).
     *
     * We do this separately from the battery indicator, so we can pan the
     * logo independently from the battery indicator.
     */

    disc_battery_draw = 1;

    draw_clear();
    draw_logo();

    vram.first_line = BATTERY_HEIGHT;
    vram.num_lines = 128 - BATTERY_HEIGHT;

    {
        int8_t x = disc_logo_x >> FP_BITS;
        int8_t y = disc_logo_y >> FP_BITS;

        // Motion equations. This includes a spring force returning
        // the logo to the center, an accelerometer force, plus some damping.

        disc_logo_x += (disc_logo_dx -= x + (disc_logo_dx >> FP_BITS) + ack_data.accel[0]);
        disc_logo_y += (disc_logo_dy -= -BATTERY_HEIGHT + y + (disc_logo_dy >> FP_BITS) + ack_data.accel[1]);

        // Convert to unsigned BG0 panning amount
        if (x < 0) x += 144;
        vram.bg0_x = x;
        if (y < 0) y += 144;
        vram.bg0_y = y;
    }

    // Clamp to the allowable deflection range.
    // If we get too far out, we may start clipping against the top edge, or
    // wrapping around the other edges. Tacky!

    if (disc_logo_x < X_MIN) {
        disc_logo_x = X_MIN;
        disc_logo_dx = -disc_logo_dx;
    }
    if (disc_logo_x > X_MAX) {
        disc_logo_x = X_MAX;
        disc_logo_dx = -disc_logo_dx;
    }
    if (disc_logo_y < Y_MIN) {
        disc_logo_y = Y_MIN;
        disc_logo_dy = -disc_logo_dy;
    }
    if (disc_logo_y > Y_MAX) {
        disc_logo_y = Y_MAX;
        disc_logo_dy = -disc_logo_dy;
    }
}

