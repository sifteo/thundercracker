/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * Thundercracker cube firmware
 *
 * M. Elizabeth Scott <beth@sifteo.com>
 * Copyright <c> 2011 Sifteo, Inc. All rights reserved.
 */

#include <stdint.h>
#include <stdio.h>
#include "main.h"
#include "radio.h"

#ifndef TOUCH_STANDALONE
#include "sensors.h"
#endif

#include "graphics.h"
#include "hardware.h"
#include "flash.h"
#include "params.h"
#include "touch.h"
#include "battery.h"
#include "demo.h"
#include "draw.h"

__bit global_busy_flag;

static void gpio_init(void);

void main(void)
{
	gpio_init();
	radio_init();
    flash_init();
#ifdef TOUCH_STANDALONE
    touch_init();
#else
    sensors_init();
#endif
    params_init();
    sti();

    //demo();
#ifdef TOUCH_STANDALONE
    DEBUG_UART_INIT();
    while(CLKLFCTRL&0x08 == 0x0);
#endif

    while (1) {
#ifdef TOUCH_STANDALONE
    	if(ts_dataReady) {
    		//touchval = (15*prev_touchval + touchval) >> 4;
    		ts_touchval.val = (7*ts_prev_touchval.val + ts_touchval.val) >> 3;
    		//touchval = (3*prev_touchval + touchval) >> 2;
    		//touchval = (prev_touchval + touchval) >> 1;
    		ts_prev_touchval.val = ts_touchval.val;

        	DEBUG_UART('\r');
        	DEBUG_UART(ts_touchval.valL);
        	DEBUG_UART(ts_touchval.valH);
        	DEBUG_UART('\n');
        	ts_touchval.val = 0;
        	ts_dataReady = 0;
    	}
#endif
    	global_busy_flag = 0;
        
        // Main tasks
    	//graphics_render();
        //flash_handle_fifo();
        
        if (global_busy_flag)
        	continue;
        
        // Idle-only tasks
        //battery_poll();
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

