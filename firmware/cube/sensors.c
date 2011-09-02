/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * Thundercracker cube firmware
 *
 * M. Elizabeth Scott <beth@sifteo.com>
 * Copyright <c> 2011 Sifteo, Inc. All rights reserved.
 */

#include <stdint.h>
#include "sensors.h"
#include "hardware.h"
#include "radio.h"


/*
 * A/D Converter ISR --
 *
 *    Stores this sample in the ack_data buffer, and swaps channels.
 */

void adc_isr(void) __interrupt(VECTOR_MISC) __naked __using(1)
{
    __asm
	push	acc
	push	psw
	mov	psw, #0x08			; Register bank 1

	mov	a,_ADCCON1			; What channel are we on? We only have two.
	jb	acc.2, 1$

	; Channel 0
	mov	a, (_ack_data + ACK_ACCEL_TOTALS + 0)
	add	a, _ADCDATH
	mov	(_ack_data + ACK_ACCEL_TOTALS + 0), a
	mov	a, (_ack_data + ACK_ACCEL_TOTALS + 1)
	addc	a, #0
	mov	(_ack_data + ACK_ACCEL_TOTALS + 1), a
	inc	(_ack_data + ACK_ACCEL_COUNTS + 0)

	xrl	_ADCCON1,#0x04			; Channel swap
	pop	psw
	pop	acc
	reti

	; Channel 1
1$:	mov	a, (_ack_data + ACK_ACCEL_TOTALS + 2)
	add	a, _ADCDATH
	mov	(_ack_data + ACK_ACCEL_TOTALS + 2), a
	mov	a, (_ack_data + ACK_ACCEL_TOTALS + 3)
	addc	a, #0
	mov	(_ack_data + ACK_ACCEL_TOTALS + 3), a
	inc	(_ack_data + ACK_ACCEL_COUNTS + 1)

	xrl	_ADCCON1,#0x04			; Channel swap
	pop	psw
	pop	acc
	reti

    __endasm ;
}


void sensors_init()
{
    // Set up continuous 8-bit, 4 ksps A/D conversion with interrupt
    ADCCON3 = 0x40;
    ADCCON2 = 0x25;
    ADCCON1 = 0x80;
    IEN_MISC = 1;
}
