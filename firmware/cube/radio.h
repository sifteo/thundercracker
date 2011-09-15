/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * Thundercracker cube firmware
 *
 * M. Elizabeth Scott <beth@sifteo.com>
 * Copyright <c> 2011 Sifteo, Inc. All rights reserved.
 */

#ifndef _RADIO_H
#define _RADIO_H

#include <stdint.h>
#include <protocol.h>
#include "hardware.h"

/*
 * Separate register bank for RF IRQ handlers. This register bank is
 * ONLY used here, so we can rely on its values to persist across IRQs.
 * This is where we store state for the packet receive state machine.
 */
#define RF_BANK  1

void radio_isr(void) __interrupt(VECTOR_RF) __naked __using(RF_BANK);
void radio_init(void);

extern RF_ACKType __near ack_data;

#endif
