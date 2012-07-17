/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * Thundercracker cube firmware
 *
 * M. Elizabeth Scott <beth@sifteo.com>
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#ifndef _POWER_H
#define _POWER_H

#include <cube_hardware.h>

/*
 * Power managament interface
 */

void power_init();
void power_sleep();

/*
 * Reset the watchdog timer. Must be called from main loop and
 * from early initialization. Currently we use a value of 2 seconds.
 */

#ifdef DISABLE_WDT
    #define power_wdt_set()
#else
    #pragma sdcc_hash +
    #define power_wdt_set() \
        __asm   mov     a, _WDSV        __endasm ;\
        __asm   mov     _WDSV, #0       __endasm ;\
        __asm   mov     _WDSV, #1       __endasm ;
#endif

#endif
