/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * Thundercracker cube firmware
 *
 * M. Elizabeth Scott <beth@sifteo.com>
 * Copyright <c> 2011 Sifteo, Inc. All rights reserved.
 */

#include "hardware.h"
#include "radio.h"

union ack_format __near ack_data;


/*
 * Radio ISR --
 *
 *    Receive one packet from the radio, store it in xram, and write
 *    out a new buffered ACK packet.
 */

void radio_isr(void) __interrupt(VECTOR_RF) __naked __using(1)
{
    __asm
	push	acc
	push	dpl
	push	dph
	push	psw
	mov	psw, #0x08			; Register bank 1

	; Start reading incoming packet
	; Do this first, since we can benefit from any latency reduction we can get.

	clr	_RF_CSN				; Begin SPI transaction
	mov	_SPIRDAT, #RF_CMD_R_RX_PAYLOAD	; Start reading RX packet
	mov	_SPIRDAT, #0			; First dummy byte, keep the TX FIFO full

	mov	a, _SPIRSTAT			; Wait for Command/STATUS byte
	jnb	acc.2, (.-2)
	mov	a, _SPIRDAT			; Ignore STATUS byte
	mov	_SPIRDAT, #0			; Write next dummy byte

	mov	a, _SPIRSTAT			; Wait for RX byte 0, block address
	jnb	acc.2, (.-2)
	mov	r0, _SPIRDAT			; Read block address. This is in units of 31 bytes
	mov	_SPIRDAT, #0			; Write next dummy byte

	; Apply the block address offset. Nominally this requires 16-bit multiplication by 31,
	; but that is really expensive. Instead, we can express N*31 as (N<<5)-N. But that still
	; requires a 16-bit shift, and it is ugly to implement on the 8051 instruction set. So,
	; we implement a different function that happens to equal N*31 for the values we care
	; about, and which uses only 8-bit math:
	;
	;   low(N*31) = (a << 5) - a
	;   high(N*31) = (a - 1) >> 3   (EXCEPT when a==0)

	mov	a, r0
	anl	a, #0x07	; Pre-mask before shifting...
	swap	a  		; << 5
	rlc	a		; Also sets C=0, for the subb below
	subb	a, r0
	mov	dpl, a

	mov	a, r0
	jz	1$
	dec	a
1$:	swap	a		; >> 3
	rl	a
	anl	a, #0x3		; Mask at 1024 bytes total
	mov	dph, a

	; Transfer 29 packet bytes in bulk to xram. (The first one and last two bytes are special)
	; If this loop takes at least 16 clock cycles per iteration, we do not need to
	; explicitly wait on the SPI engine, we can just stay synchronized with it.

	mov	r1, #29
2$:	mov	a, _SPIRDAT	; 2  Read payload byte
	mov	_SPIRDAT, #0	; 3  Write the next dummy byte
	movx	@dptr, a	; 5
	inc	dptr		; 1
	nop			; 1  (Pad to 16 clock cycles)
	nop			; 1
	djnz	r1, 2$		; 3

	; Last two bytes, drain the SPI RX FIFO

	mov	a, _SPIRDAT	; Read payload byte
	movx	@dptr, a
	inc	dptr
	mov	a, _SPIRDAT	; Read payload byte
	movx	@dptr, a
	setb	_RF_CSN		; End SPI transaction

	; nRF Interrupt acknowledge

	clr	_RF_CSN						; Begin SPI transaction
	mov	_SPIRDAT, #(RF_CMD_W_REGISTER | RF_REG_STATUS)	; Start writing to STATUS
	mov	_SPIRDAT, #RF_STATUS_RX_DR			; Clear interrupt flag
	mov	a, _SPIRSTAT					; RX dummy byte 0
	jnb	acc.2, (.-2)
	mov	a, _SPIRDAT
	mov	a, _SPIRSTAT					; RX dummy byte 1
	jnb	acc.2, (.-2)
	mov	a, _SPIRDAT
	setb	_RF_CSN						; End SPI transaction

	; Write the ACK packet, from our buffer.

	clr	_RF_CSN					; Begin SPI transaction
	mov	_SPIRDAT, #RF_CMD_W_ACK_PAYLD		; Start sending ACK packet
	mov	r1, #_ack_data
	mov	r0, #ACK_LENGTH

3$:	mov	_SPIRDAT, @r1
	inc	r1
	mov	a, _SPIRSTAT				; RX dummy byte
	jnb	acc.2, (.-2)
	mov	a, _SPIRDAT
	djnz	r0, 3$

	mov	a, _SPIRSTAT				; RX last dummy byte
	jnb	acc.2, (.-2)
	mov	a, _SPIRDAT
	setb	_RF_CSN					; End SPI transaction

	; Clear the accelerometer accumulators

	mov	(_ack_data + ACK_ACCEL_TOTALS + 0), #0
	mov	(_ack_data + ACK_ACCEL_TOTALS + 1), #0
	mov	(_ack_data + ACK_ACCEL_TOTALS + 2), #0
	mov	(_ack_data + ACK_ACCEL_TOTALS + 3), #0
	mov	(_ack_data + ACK_ACCEL_COUNTS + 0), #0
	mov	(_ack_data + ACK_ACCEL_COUNTS + 1), #0

	pop	psw
	pop	dph
	pop	dpl
	pop	acc
	reti
	
    __endasm ;
}

static void radio_reg_write(uint8_t reg, uint8_t value)
{
    RF_CSN = 0;

    SPIRDAT = RF_CMD_W_REGISTER | reg;
    SPIRDAT = value;
    while (!(SPIRSTAT & SPI_RX_READY));
    SPIRDAT;
    while (!(SPIRSTAT & SPI_RX_READY));
    SPIRDAT;

    RF_CSN = 1;
}

void radio_init(void)
{
    // Radio clock running
    RF_CKEN = 1;
    
    // Enable RX interrupt
    radio_reg_write(RF_REG_CONFIG, RF_STATUS_RX_DR);
    IEN_RF = 1;

    // Start receiving
    RF_CE = 1;
}
