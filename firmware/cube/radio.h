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
#include "hardware.h"

/*
 * Separate register bank for RF IRQ handlers. This register bank is
 * ONLY used here, so we can rely on its values to persist across IRQs.
 * This is where we store state for the packet receive state machine.
 */
#define RF_BANK  1

void radio_isr(void) __interrupt(VECTOR_RF) __naked __using(RF_BANK);
void radio_init(void);

/*
 * ACK reply buffer format
 */

#define ACK_LENGTH  		12
#define ACK_ACCEL_COUNTS	1
#define ACK_ACCEL_TOTALS	3

union ack_format {
    uint8_t bytes[1];
    struct {
	/*
	 * Our standard response packet format. This currently all
	 * comes to a total of 15 bytes. We probably also want a
	 * variable-length "tail" on this packet, to allow us to
	 * transmit specific data that the master requested, like HWID
	 * or firmware version. But this is the high-frequency
	 * telemetry data that we ALWAYS send at the full packet rate.
	 */

	/*
	 * For synchronizing LCD refreshes, the master can keep track
	 * of how many repaints the cube has performed. Ideally these
	 * repaints would be in turn sync'ed with the LCDC's hardware
	 * refresh timer. If we're tight on space, we don't need a
	 * full byte for this. Even a one-bit toggle woudl work,
	 * though we might want two bits to allow deeper queues.
	 */
	uint8_t frame_count;

        // Averaged accel data
	uint8_t accel_count[2];
	uint16_t accel_total[2];

	/*
	 * Need ~5 bits per sensor (5 other cubes * 4 sides + 1 idle =
	 * 21 states) So, there are plenty of bits free in here to
	 * encode things like button state.
	 */
	uint8_t neighbor[4];

	uint8_t flash_fifo_bytes;
    };
};

extern union ack_format __near ack_data;

#endif
