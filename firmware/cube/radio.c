/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * Thundercracker cube firmware
 *
 * M. Elizabeth Scott <beth@sifteo.com>
 * Copyright <c> 2011 Sifteo, Inc. All rights reserved.
 */

#include "hardware.h"
#include "radio.h"
#include "flash.h"

/*
 * The ACK reply buffer lives in near memory. It needs to, so that
 * the ADC ISR can access it very quickly.
 */
union ack_format __near ack_data;

/*
 * Radio protocol opcodes.
 *
 * Our basic opcode format is a 2-bit command and a 6-bit length, with
 * length always encoded as the actual length - 1. This lets us fit
 * the most common commands into a single byte, and the 6-bit length
 * allows us to amortize the opcode size to less than a byte per
 * packet.
 *
 * Opcodes may persist past the end of a radio packet if and only if
 * that packet is the maximum length. At the end of any non-maximal
 * length packet, the radio state machine is reset. If you need to
 * explicitly reset the state machine, you can send a zero-length
 * packet.
 *
 * XXX: Figure out how best to allocate this opcode space once we get
 *      a sample of some real-world radio traffic for a game.
 */

#define RF_OP_MASK		0xc0
#define RF_ARG_MASK		0x3f

// Major opcodes
#define RF_OP_SPECIAL		0x00    // Multiplexor for special opcodes
#define RF_OP_VRAM_SKIP		0x40	// Seek forward arg+1 bytes
#define RF_OP_VRAM_DATA		0x80    // Write arg+1 bytes to VRAM
#define RF_OP_FLASH_QUEUE	0xc0    // Write arg+1 bytes to flash loadstream FIFO

// Special minor opcodes
#define RF_OP_VRAM_ADDRESS	0x00	// Change the VRAM write pointer
#define RF_OP_VRAM_DIRECT	0x01	// Change write mode: Direct
#define RF_OP_VRAM_VERTICAL	0x02	// Change write mode: Top-to-bottom

/*
 * Assembly macros.
 */

#define SPI_WAIT						__endasm; \
	__asm	mov	a, _SPIRSTAT				__endasm; \
	__asm	jnb	acc.2, (.-2)				__endasm; \
	__asm

#define RX_NEXT_BYTE						__endasm; \
	__asm	djnz	R_PACKET_REMAINING, rx_byte_loop	__endasm; \
	__asm	ljmp	rx_complete				__endasm; \
	__asm

/*
 * Register usage in RF_BANK.
 */

#define R_TMP			r0
#define R_BYTE			r3
#define R_FLASH_COUNT		r4
#define R_VRAM_COUNT		r5
#define R_PACKET_REMAINING	r6
#define R_PACKET_LEN		r7

static uint16_t rf_vram_ptr;		// Lives in DPTR during the ISR

/*
 * Radio ISR --
 *
 *    Receive one packet from the radio, store it according to our
 *    opcode state machine, and write out a new buffered ACK
 *    packet. This is very performance critical, and we can save on
 *    some key sources of overhead by using assembly, and managing
 *    our register usage manually.
 */

void radio_isr(void) __interrupt(VECTOR_RF) __naked __using(RF_BANK)
{
    __asm
	push	acc
	push	dpl
	push	dph
	push	psw
	mov	psw, #(RF_BANK << 3)
	mov	dpl, _rf_vram_ptr
	mov	dph, (_rf_vram_ptr + 1)

	;--------------------------------------------------------------------
	; Packet receive setup
	;--------------------------------------------------------------------

	; Read the length of this received packet

	clr	_RF_CSN				; Begin SPI transaction
	mov	_SPIRDAT, #RF_CMD_R_RX_PL_WID	; Read RX Payload Width command
	mov	_SPIRDAT, #0			; First dummy byte, keep the TX FIFO full
	SPI_WAIT	  			; Wait for Command/STATUS byte
	mov	a, _SPIRDAT			; Ignore STATUS byte
	SPI_WAIT   				; Wait for width byte
	mov	a, _SPIRDAT			; Total packet length
	setb	_RF_CSN				; End SPI transaction

	; If the packet is greater than RF_PAYLOAD_MAX, it is an error. We need
	; to discard it. This should be really rare, and honestly we only
	; do this because the data sheet claims it is SUPER IMPORTANT.
	; 
	; Since we have some code here to do an RX_FLUSH anyway, we also
	; send zero-length packets through this path. A zero-length packet
	; only has the effects of (1) causing an ACK transmission, and (2)
	; resetting the RX state machine, both of which are things we can do
	; cheaply through this path. And as we can see below, throwing out
	; zero-length packets early means much less special-casing in the
	; RX loop!

	mov	R_PACKET_LEN, a
	jz	#6$				; Skip right to RX_FLUSH if zero-length
	mov	R_PACKET_REMAINING, a

	add	a, #(0xFF - RF_PAYLOAD_MAX)
	jnc	#4$				; Jump if R_PACKET_LENGTH < RF_PAYLOAD_MAX

6$:
	clr	_RF_CSN				; Begin SPI transaction
	mov	_SPIRDAT, #RF_CMD_FLUSH_RX	; RX_FLUSH command
	SPI_WAIT				; Wait for command byte
	mov	a, _SPIRDAT			; Ignore dummy STATUS byte
	ljmp	#rx_complete			; Skip the RX loop (and end SPI transaction)
4$:

	; Start reading the incoming packet, then loop over all bytes.
	; We try to keep the SPI FIFOs full here, and we expect the
	; byte processing loop to be at least 16 clock cycles long, so
	; we do not explicitly check the SPI status after each byte.

	clr	_RF_CSN				; Begin SPI transaction
	mov	_SPIRDAT, #RF_CMD_R_RX_PAYLOAD	; Start reading RX packet
	mov	_SPIRDAT, #0			; First dummy byte, keep the TX FIFO full
	SPI_WAIT	  			; Wait for Command/STATUS byte
	mov	a, _SPIRDAT			; Ignore status
	SPI_WAIT	  			; Wait for first data byte

rx_byte_loop:
	mov	R_BYTE, _SPIRDAT		; Store received byte

	mov	a, R_PACKET_REMAINING		; Skip if this is the last RX byte
	dec	a
	jz	8$
	mov	_SPIRDAT, #0			; Write next dummy byte, start next RX
8$:

	;--------------------------------------------------------------------
	; Data byte handlers
	;--------------------------------------------------------------------

	; These handlers should be sorted in order of decreasing
	; frequency, to save clock cycles on the most frequent ops.

	;---------------------------------
	; VRAM Write Data
	;---------------------------------

	mov	a, R_VRAM_COUNT			; Are we in a VRAM write operation?
	jz	9$

	mov	a, R_BYTE			; Store R_BYTE at current VRAM pointer
	movx	@dptr, a
	inc	dptr

	dec	R_VRAM_COUNT			; Next packet byte, next opcode byte
	RX_NEXT_BYTE
9$:

	;---------------------------------
	; Flash Write Data
	;---------------------------------

	mov	a, R_FLASH_COUNT		; Are we in a Flash enqueue operation?
	jz	10$

	mov	R_TMP, a

	mov     a, _flash_fifo_head		; Load the flash write pointer
	add	a, #_flash_fifo			; Address relative to flash_fifo[]
	mov	R_TMP, a
	mov	a, R_BYTE			; Store R_BYTE in the FIFO
	mov	@R_TMP, a

	mov	a, _flash_fifo_head		; Advance head pointer
	inc	a
	anl	a, #FLASH_FIFO_MASK
	mov	_flash_fifo_head, a


	dec	R_FLASH_COUNT			; Next packet byte, next opcode byte
	RX_NEXT_BYTE
10$:

	;--------------------------------------------------------------------
	; Opcode byte handlers
	;--------------------------------------------------------------------

	mov	a, R_BYTE
	anl	a, #RF_OP_MASK

	;---------------------------------
	; VRAM Write Data
	;---------------------------------

	cjne	a, #RF_OP_VRAM_DATA, 11$
	mov	a, R_BYTE
	anl	a, #RF_ARG_MASK
	inc	a
	mov	R_VRAM_COUNT, a
	RX_NEXT_BYTE
11$:

	;---------------------------------
	; Flash Write Data
	;---------------------------------

	cjne	a, #RF_OP_FLASH_QUEUE, 12$
	mov	a, R_BYTE
	anl	a, #RF_ARG_MASK
	inc	a
	mov	R_FLASH_COUNT, a
	RX_NEXT_BYTE
12$:

	;---------------------------------
	; Other
	;---------------------------------

	; Catch-all for unimplemented opcodes...
	RX_NEXT_BYTE

	;--------------------------------------------------------------------
	; Packet receive completion
	;--------------------------------------------------------------------

rx_complete:

	setb	_RF_CSN		; End SPI transaction

	; If the packet was not exactly the maximum length, reset our state machine now.
	; Note that we do not have to initialize all of our registers, just those that keep
	; track of the intra-opcode state. State that persists between opcodes, like the VRAM
	; pointer, should not be touched.

	mov	a, R_PACKET_LEN
	xrl	a, #RF_PAYLOAD_MAX
	jz	7$

	mov	R_FLASH_COUNT, #0
	mov	R_VRAM_COUNT, #0
7$:

	; nRF Interrupt acknowledge

	clr	_RF_CSN						; Begin SPI transaction
	mov	_SPIRDAT, #(RF_CMD_W_REGISTER | RF_REG_STATUS)	; Start writing to STATUS
	mov	_SPIRDAT, #RF_STATUS_RX_DR			; Clear interrupt flag
	SPI_WAIT						; RX dummy byte 0
	mov	a, _SPIRDAT
	SPI_WAIT						; RX dummy byte 1
	mov	a, _SPIRDAT
	setb	_RF_CSN						; End SPI transaction

	;--------------------------------------------------------------------
	; ACK packet write
	;--------------------------------------------------------------------

	clr	_RF_CSN					; Begin SPI transaction
	mov	_SPIRDAT, #RF_CMD_W_ACK_PAYLD		; Start sending ACK packet
	mov	r1, #_ack_data
	mov	r0, #ACK_LENGTH

3$:	mov	_SPIRDAT, @r1
	inc	r1
	SPI_WAIT					; RX dummy byte
	mov	a, _SPIRDAT
	djnz	r0, 3$

	SPI_WAIT					; RX last dummy byte
	mov	a, _SPIRDAT
	setb	_RF_CSN					; End SPI transaction

	; Clear the accelerometer accumulators

	mov	(_ack_data + ACK_ACCEL_TOTALS + 0), #0
	mov	(_ack_data + ACK_ACCEL_TOTALS + 1), #0
	mov	(_ack_data + ACK_ACCEL_TOTALS + 2), #0
	mov	(_ack_data + ACK_ACCEL_TOTALS + 3), #0
	mov	(_ack_data + ACK_ACCEL_COUNTS + 0), #0
	mov	(_ack_data + ACK_ACCEL_COUNTS + 1), #0

	mov	_rf_vram_ptr, dpl
	mov	(_rf_vram_ptr + 1), dph
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

    // Use dynamic payload lengths
    radio_reg_write(RF_REG_FEATURE, 0x07);
    radio_reg_write(RF_REG_DYNPD, 0x01);

    // Enable RX interrupt
    radio_reg_write(RF_REG_CONFIG, RF_STATUS_RX_DR);
    IEN_RF = 1;

    // Start receiving
    RF_CE = 1;
}
