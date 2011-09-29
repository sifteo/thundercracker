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
#include "demo.h"


void main(void)
{
    lcd_init();
    sensors_init();
    radio_init();
    flash_init();
    sti();

    demo();  // XXX

    while (1) {
        flash_handle_fifo();
        graphics_render();
    }
}
