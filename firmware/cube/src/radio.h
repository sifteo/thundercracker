/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * Thundercracker cube firmware
 *
 * M. Elizabeth Scott <beth@sifteo.com>
 * Copyright <c> 2011 Sifteo, Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#ifndef _RADIO_H
#define _RADIO_H

#include <stdint.h>
#include <protocol.h>
#include <cube_hardware.h>

/*
 * Separate register bank for RF IRQ handlers. This register bank is
 * ONLY used here, so we can rely on its values to persist across IRQs.
 * This is where we store state for the packet receive state machine.
 */
#define RF_BANK  1

void radio_isr(void) __interrupt(VECTOR_RF) __naked __using(RF_BANK);
void radio_init(void);
void radio_set_idle_addr(void);
void radio_set_pairing_addr(void);
void radio_ack_query() __naked;
void radio_fifo_status() __naked;

extern RF_MemACKType __near ack_data;
extern uint8_t __near ack_bits;
extern uint8_t radio_query[32];
extern __bit radio_connected;
extern __bit radio_idle_hop;

/*
 * We track the length of the next ACK packet using a bitmap, where each
 * bit maps to one of the valid packet lengths as identified by the
 * RF_ACK_LEN_* constants in protocol.h. To request a packet, set the
 * corresponding bit in ack_bits.. When we send a packet, we'll size it
 * according to the most significant '1' bit.
 */

#define RF_ACK_BIT_FRAME            0x01
#define RF_ACK_BIT_ACCEL            0x02
#define RF_ACK_BIT_NEIGHBOR         0x04
#define RF_ACK_BIT_FLASH_FIFO       0x08
#define RF_ACK_BIT_BATTERY_V        0x10
#define RF_ACK_BIT_HWID             0x20

#define RF_ACK_ABIT_FRAME           acc.0
#define RF_ACK_ABIT_ACCEL           acc.1
#define RF_ACK_ABIT_NEIGHBOR        acc.2
#define RF_ACK_ABIT_FLASH_FIFO      acc.3
#define RF_ACK_ABIT_BATTERY_V       acc.4
#define RF_ACK_ABIT_HWID            acc.5

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

#define SPI_WAIT                                        __endasm; \
        __asm   mov     a, _SPIRSTAT                    __endasm; \
        __asm   jnb     acc.2, (.-2)                    __endasm; \
        __asm

#endif
