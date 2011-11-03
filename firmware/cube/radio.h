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

uint8_t radio_get_cube_id(void);

extern RF_MemACKType __near ack_data;
extern uint8_t __near ack_len;

/*
 * IRQ control. We have some critical sections where we'd really like
 * to prevent a radio packet from modifying VRAM in-between particular
 * operations.
 */
 
#define radio_irq_enable()          { IEN_RF = 1; }
#define radio_irq_disable()         { IEN_RF = 0; }
#define radio_rx_enable()           { RF_CE = 1;  }
#define radio_rx_disable()          { RF_CE = 0;  }
#define radio_critical_section(_x)  { radio_irq_disable() _x radio_irq_enable() }

#endif
