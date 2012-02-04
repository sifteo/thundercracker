/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * Thundercracker cube firmware
 *
 * M. Elizabeth Scott <beth@sifteo.com>
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#ifndef _POWER_H
#define _POWER_H

#include <stdint.h>
#include "sensors.h"

/*
 * The cube's idle timeout, measured in units of 1.57 seconds
 *
 * (The amount of time it takes for sensor_tick_counter to
 * overflow its low byte)
 */

#define IDLE_TIMEOUT   38   // 1 minute

/*
 * Power managament interface
 */

extern uint8_t power_sleep_timer;
 
void power_init();
void power_sleep();

// Reset sleep timer. Safe to call anywhere.
#define POWER_IDLE_RESET_ASM() \
    mov     _power_sleep_timer, _sensor_tick_counter_high

// Potentially enter deep sleep. Must be called from main idle loop.
#define power_idle_poll() { \
    uint8_t a = sensor_tick_counter_high; \
    a -= power_sleep_timer; \
    if (a == IDLE_TIMEOUT) \
        power_sleep(); \
}

#endif
