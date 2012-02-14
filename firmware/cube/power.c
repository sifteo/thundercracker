/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * Thundercracker cube firmware
 *
 * M. Elizabeth Scott <beth@sifteo.com>
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#include "power.h"
#include "hardware.h"
#include "lcd.h"
#include "radio.h"

uint8_t power_sleep_timer;


void power_init(void)
{
    /*
     * If we're being powered on without a wakup-from-pin, go directly to
     * sleep. We don't wake up when the batteries are first inserted, only
     * when we get a touch signal.
     *
     * This default can be overridden by compiling with -DWAKE_ON_POWERUP.
     */

    // Data sheet specifies that we need to rest PWRDWN to 0 after reading it.
    uint8_t powerupReason = PWRDWN;
    PWRDWN = 0;

#ifndef WAKE_ON_POWERUP
    if (!(powerupReason & 0x80))
        power_sleep();
#endif

    /*
     * Basic poweron
     */

    BUS_DIR = 0xFF;

    ADDR_PORT = 0;
    MISC_PORT = MISC_IDLE;
    CTRL_PORT = CTRL_IDLE;

    ADDR_DIR = 0;
    MISC_DIR = MISC_DIR_VALUE;
    CTRL_DIR = CTRL_DIR_VALUE;

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

void power_sleep(void)
{
#ifndef REV1
    /*
     * Turn off all peripherals, and put the CPU into Deep Sleep mode.
     * Order matters, don't cause bus contention.
     */

    lcd_sleep();                // Sleep sequence for LCD controller
    cli();                      // Stop all interrupt handlers
    radio_rx_disable();         // Take the radio out of RX mode
    RF_CKEN = 0;                // Stop the radio clock
    RNGCTL = 0;                 // RNG peripheral off
    ADCCON1 = 0;                // ADC peripheral off

    BUS_DIR = 0xFF;             // Float the bus before we've set CTRL_PORT

    ADDR_PORT = 0;              // Drive address bus low
    MISC_PORT = MISC_IDLE;      // Neighbor hardware idle
    CTRL_PORT = CTRL_SLEEP;     // Turns off DC-DC converters

    ADDR_DIR = 0;               // Default drive values
    MISC_DIR = MISC_DIR_VALUE;
    CTRL_DIR = CTRL_DIR_VALUE;  
    
    BUS_PORT = 0;               // Drive bus port low
    BUS_DIR = 0;

    /*
     * Wakeup on touch
     */

    TOUCH_WUPOC = TOUCH_WUOPC_BIT;
    WUCON = 0xFB;

    while (1) {
        PWRDWN = 1;

        /*
         * We loop purely out of paranoia. This point should never be reached.
         * On wakeup, the system experiences a soft reset.
         */
    }
#endif
}
