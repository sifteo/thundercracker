/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * Thundercracker cube firmware
 *
 * M. Elizabeth Scott <beth@sifteo.com>
 * Copyright <c> 2011 Sifteo, Inc. All rights reserved.
 */

#ifndef _PARAMS_H
#define _PARAMS_H

#include <stdint.h>
#include <protocol.h>
#include "hardware.h"


/*
 * This structure specifies the layout of parameters in our OTP flash
 * space. Normally this area is write-protected.
 */
 
#define     PARAMS_BASE     0xFC00
#define     PARAMS_HWID     (PARAMS_BASE + 0)
 
static __xdata __at PARAMS_BASE struct {
    uint8_t hwid[HWID_LEN];
    uint8_t hwid_flag;
} params;


/*
 * Public Functions
 */

void params_init();


#endif
