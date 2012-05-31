/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * Thundercracker cube firmware
 *
 * M. Elizabeth Scott <beth@sifteo.com>
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

/*
 * Internal declarations for Touch sensor
 */

#ifndef _SENSORS_TOUCH_H
#define _SENSORS_TOUCH_H

//#define TOUCH_DEBOUNCE

#ifdef TOUCH_DEBOUNCE
    #define TOUCH_DEBOUNCE_ON 5
    #define TOUCH_DEBOUNCE_OFF 10

    extern uint8_t touch_on;
    extern uint8_t touch_off;
#endif

#endif
