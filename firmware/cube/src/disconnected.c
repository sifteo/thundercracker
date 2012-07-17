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

void draw_hex(uint8_t value)
{
    value = value;
    __asm
        mov     a, DPL
        mov     _DPL, _draw_xy
        mov     _DPH, (_draw_xy + 1)

        mov     r0, a           ; Store low nybble for later    
        swap    a               ; Start with the high nybble
        mov     r1, #2          ; Loop over two nybbles
2$:

        anl     a, #0xF
        clr     ac
        clr     c
        da      a               ; If > 9, add 6. This almost covers the gap between '9' and 'A'
        jnb     acc.4, 1$       ;   ... at least we can test efficiently whether the nybble was >9 now
        inc     a               ;   and add one more. This closes the total gap (7 characters)
1$:     add     a, #0x10        ; Add index for '0'
        rl      a               ; Index to 7:7

        movx    @dptr, a        ; Draw, with current address
        inc     dptr
        clr     a
        movx    @dptr, a
        inc     dptr

        mov     a, r0           ; Restore saved nybble
        djnz    r1, 2$          ; Next!

        mov     _draw_xy, _DPL
        mov     (_draw_xy + 1), _DPH
    __endasm ;
}

void disconnected_poll(void)
{
    draw_xy = 0;
    draw_hex(ack_data.battery_v[1]);
    draw_hex(ack_data.battery_v[0]);

    power_idle_poll();
}

