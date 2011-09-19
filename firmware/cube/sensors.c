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
 * Channel swap and return
 */
#define ADC_ISR_RET				__endasm ; \
	__asm	xrl	_ADCCON1, #0x04		__endasm ; \
	__asm	pop	psw			__endasm ; \
	__asm	pop	acc			__endasm ; \
	__asm	reti				__endasm ; \
	__asm

/*
 * A/D Converter ISR --
 *
 *    Stores this sample in the ack_data buffer, and swaps channels.
 *
 *    This ISR does not switch register banks, because it does not
 *    use registers. Only the accumulator needs to be saved. Performance
 *    here is really important, since the ISR is invoked so frequently.
 */

void adc_isr(void) __interrupt(VECTOR_MISC) __naked
{
    __asm
	push	acc
	push	psw

	mov	a,_ADCCON1		; What channel are we on? We only have two.
	jb	acc.2, 1$

	; Sample channel 0
	mov	a, _ADCDATH
	cjne	a, (_ack_data + RF_ACK_ACCEL + 0), 2$
	ADC_ISR_RET

	; Channel 0 has changed
2$:	mov	(_ack_data + RF_ACK_ACCEL + 0), a
	orl	_ack_len, #RF_ACK_LEN_ACCEL
	ADC_ISR_RET

	; Sample channel 1
1$:	mov	a, _ADCDATH
	cjne	a, (_ack_data + RF_ACK_ACCEL + 1), 3$
	ADC_ISR_RET

	; Channel 1 has changed
3$:	mov	(_ack_data + RF_ACK_ACCEL + 1), a
	orl	_ack_len, #RF_ACK_LEN_ACCEL
	ADC_ISR_RET

    __endasm ;
}


void sensors_init()
{
    // Set up continuous 8-bit, 2 ksps A/D conversion with interrupt
    ADCCON3 = 0x40;
    ADCCON2 = 0x20;
    ADCCON1 = 0x80;
    IEN_MISC = 1;
}
