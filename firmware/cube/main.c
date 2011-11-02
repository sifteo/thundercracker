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
#include "graphics.h"
#include "hardware.h"
#include "flash.h"
#include "params.h"
#include "demo.h"

static void gpio_init(void);


void main(void)
{
    gpio_init();
    radio_init();
    flash_init();
    sensors_init();
    params_init();
    sti();

    demo();  // XXX

    while (1) {
        flash_handle_fifo();
        graphics_render();
    }
}

static void gpio_init(void)
{
    /*
     * Basics
     */

    BUS_DIR = 0xFF;

    ADDR_PORT = 0;
    MISC_PORT = MISC_IDLE;
    CTRL_PORT = CTRL_IDLE;

    ADDR_DIR = 0;
    MISC_DIR = MISC_DIR_VALUE;
    CTRL_DIR = CTRL_DIR_VALUE;

    /*
     * Pull-ups on I2C. These don't seem to have much effect, but it's
     * the thought that counts...
     */

    MISC_CON = 0x52;    // Pull-up on I2C SCL
    MISC_CON = 0x53;    // Pull-up on I2C SDA

    /*
     * It's really important that there's no pull-up/pull-down on our
     * touch sensor input. Reset that, just in case.
     */

    MISC_CON = 0x04;

    /*
     * Neighbor TX pins
     *
     * We enable pull-downs for input mode, when we're receiving pulses from
     * our neighbors. This improves the isolation between each side's input.
     *
     * We do NOT use high-drive mode, as it seems to actually make things a
     * lot worse!
     */

    MISC_CON = 0x30;
    MISC_CON = 0x31;
    MISC_CON = 0x35;
    MISC_CON = 0x37;
}

