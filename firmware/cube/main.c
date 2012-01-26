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
//#include "touch.h"
#include "battery.h"
#include "demo.h"

__bit global_busy_flag;

static void gpio_init(void);


void main(void)
{
    gpio_init();
    radio_init();
    flash_init();
    sensors_init();
    params_init();
    
    /*
     * Global interrupt enable, only AFTER every module has a chance to
     * run its init() code. Modules may set subsystem-specific interrupt
     * enables, but they may not set the global interrupt enable on their
     * own.
     */
    sti();

    /*
     * Allow incoming radio packets. We do this very last, after we are truly
     * ready to handle packets.
     *
     * Most importantly, we allow incoming radio packets only AFTER the global
     * IRQ enable is set. If a radio packet comes in earlier, its interrupt
     * will be delayed; and not all of our initialization code will correctly
     * preserve this flag in IRCON.
     */
    radio_rx_enable();

    // XXX
    demo();
    
    while (1) {
        global_busy_flag = 0;
        
        // Main tasks
        graphics_render();
        flash_handle_fifo();
        
        if (global_busy_flag)
            continue;
        
        // Idle-only tasks
        battery_poll();
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
     * Setting pull down on input so it doesn't float.
     */

    // MISC_CON = 0x34;

    /*
     * Neighbor TX pins
     *
     * We enable pull-downs for input mode, when we're receiving pulses from
     * our neighbors. This improves the isolation between each side's input.
     *
     * High drive is enabled.
     */

    MISC_CON = 0x60;
    MISC_CON = 0x61;
    MISC_CON = 0x65;
    MISC_CON = 0x67;
}
