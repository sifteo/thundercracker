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

static void gpio_init(void);


void main(void)
{
    gpio_init();
    sensors_init();
    flash_init();
    radio_init();       // Put radio_init() last, for minimal time between RX_FLUSH and sti()
    sti();

    demo();  // XXX

    while (1) {
        flash_handle_fifo();
        graphics_render();
    }
}

static void gpio_init(void)
{
    BUS_DIR = 0xFF;

    ADDR_PORT = 0;
    MISC_PORT = MISC_IDLE;
    CTRL_PORT = CTRL_IDLE;

    ADDR_DIR = 0;
    MISC_DIR = MISC_DIR_VALUE;
    CTRL_DIR = CTRL_DIR_VALUE;

    MISC_CON = 0x52;    // Pull-up on I2C SCL
    MISC_CON = 0x53;    // Pull-up on I2C SDA
}
