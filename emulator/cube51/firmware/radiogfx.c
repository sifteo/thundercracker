/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * nRF Radio + Graphics Engine + ??? = Profit!
 *
 * M. Elizabeth Scott <beth@sifteo.com>
 * Copyright <c> 2011 Sifteo, Inc. All rights reserved.
 */

#include <stdint.h>
#include <string.h>
#include "hardware.h"


/**********************************************************************
 * Data Formats
 **********************************************************************/

#define NUM_SPRITES  		2
#define CHROMA_KEY		0xF5

#define VRAM_LATCHED		800
#define VRAM_LATCHED_SIZE	(NUM_SPRITES * 6 + 2)

#define LVRAM_PAN_X		0
#define LVRAM_PAN_Y		1
#define LVRAM_SPRITES		3

#define LVRAM_SPR_X(i)		(LVRAM_SPRITES + (i)*6 + 0)
#define LVRAM_SPR_Y(i)		(LVRAM_SPRITES + (i)*6 + 1)
#define LVRAM_SPR_MX(i)		(LVRAM_SPRITES + (i)*6 + 2)
#define LVRAM_SPR_MY(i)		(LVRAM_SPRITES + (i)*6 + 3)
#define LVRAM_SPR_AH(i)		(LVRAM_SPRITES + (i)*6 + 4)
#define LVRAM_SPR_AL(i)		(LVRAM_SPRITES + (i)*6 + 5)

struct sprite_info {
    uint8_t x, y;
    uint8_t mask_x, mask_y;
    uint8_t addr_h, addr_l;
};

struct latched_vram {
    // The small portion of VRAM that we latch before every frame
    uint8_t pan_x, pan_y;
    struct sprite_info sprites[NUM_SPRITES];
};

static __xdata union {
    uint8_t bytes[1024];
    struct {
	uint8_t tilemap[800];
	struct latched_vram latched;
	uint8_t frame_trigger;
    };
} vram;

#define ACK_LENGTH  		11
#define ACK_ACCEL_COUNTS	1
#define ACK_ACCEL_TOTALS	3

static __near union {
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
    };
} ack_data;


/**********************************************************************
 * Interrupt Handlers
 **********************************************************************/

/*
 * Radio ISR --
 *
 *    Receive one packet from the radio, store it in xram, and write
 *    out a new buffered ACK packet.
 */

void rf_isr(void) __interrupt(VECTOR_RF) __naked __using(1)
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


/**********************************************************************
 * Graphics Engine
 **********************************************************************/

// Latched/local data for sprites, panning
static __near struct latched_vram lvram;

static uint8_t x_bg_first_w;	// Width of first displayed background tile, [1, 8]
static uint8_t x_bg_last_w;	// Width of first displayed background tile, [0, 7]
static uint8_t x_bg_first_addr;	// Low address offset for first displayed tile
static uint8_t x_bg_wrap;	// Load value for a dec counter to the next X map wraparound

static uint8_t y_bg_addr_l;	// Low part of tile addresses, inc by 32 each line
static uint16_t y_bg_map;	// Map address for the first tile on this line

// Called once per tile, to check for horizontal map wrapping
#define MAP_WRAP_CHECK() {			  	\
	if (!--bg_wrap)					\
	    DPTR -= 40;					\
    }

// Output a nonzero number of of pixels, not known at compile-time
#define PIXEL_BURST(_count) {				\
	register uint8_t _i = (_count);			\
	do {						\
	    ADDR_INC4();				\
	} while (--_i);					\
    }

// Load a 16-bit tile address from DPTR without incrementing
#pragma sdcc_hash +
#define ADDR_FROM_DPTR() {					\
    __asm movx	a, @dptr					__endasm; \
    __asm mov	ADDR_PORT, a					__endasm; \
    __asm inc	dptr						__endasm; \
    __asm mov	CTRL_PORT, #CTRL_FLASH_OUT | CTRL_FLASH_LAT1	__endasm; \
    __asm movx	a, @dptr					__endasm; \
    __asm mov	ADDR_PORT, a					__endasm; \
    __asm dec	dpl						__endasm; \
    __asm mov	CTRL_PORT, #CTRL_FLASH_OUT | CTRL_FLASH_LAT2	__endasm; \
    }

// Load a 16-bit tile address from DPTR, and auto-increment
#pragma sdcc_hash +
#define ADDR_FROM_DPTR_INC() {					\
    __asm movx	a, @dptr					__endasm; \
    __asm mov	ADDR_PORT, a					__endasm; \
    __asm inc	dptr						__endasm; \
    __asm mov	CTRL_PORT, #CTRL_FLASH_OUT | CTRL_FLASH_LAT1	__endasm; \
    __asm movx	a, @dptr					__endasm; \
    __asm mov	ADDR_PORT, a					__endasm; \
    __asm inc	dptr						__endasm; \
    __asm mov	CTRL_PORT, #CTRL_FLASH_OUT | CTRL_FLASH_LAT2	__endasm; \
    }

// Add 2 to DPTR. (Can do this in 2 clocks with inline assembly)
#define DPTR_INC2() {						\
    __asm inc	dptr						__endasm; \
    __asm inc	dptr						__endasm; \
    }

// Load a 16-bit address from sprite LVRAM
#define ADDR_FROM_SPRITE(_i) {				\
	ADDR_PORT = lvram.sprites[_i].addr_l;		\
	CTRL_PORT = CTRL_FLASH_OUT | CTRL_FLASH_LAT1;	\
	ADDR_PORT = lvram.sprites[_i].addr_h;		\
	CTRL_PORT = CTRL_FLASH_OUT | CTRL_FLASH_LAT2;	\
    }


/*
 * lcd_line_bg --
 *
 *    Scanline renderer, draws a single tiled background layer.
 */

static void lcd_line_bg(void)
{
    uint8_t x = 15;
    uint8_t bg_wrap = x_bg_wrap;

    DPTR = y_bg_map;

    // First partial or full tile
    ADDR_FROM_DPTR_INC();
    MAP_WRAP_CHECK();
    ADDR_PORT = y_bg_addr_l + x_bg_first_addr;
    PIXEL_BURST(x_bg_first_w);

    // There are always 15 full tiles on-screen
    do {
	ADDR_FROM_DPTR_INC();
	MAP_WRAP_CHECK();
	ADDR_PORT = y_bg_addr_l;
	ADDR_INC32();
    } while (--x);

    // Might be one more partial tile
    if (x_bg_last_w) {
	ADDR_FROM_DPTR_INC();
	MAP_WRAP_CHECK();
	ADDR_PORT = y_bg_addr_l;
	PIXEL_BURST(x_bg_last_w);
    }
}

/*
 * lcd_line_bg_spr0 --
 *
 *    Scanline renderer: One tiled background, one sprite (index 0)
 *
 *    XXX: This doesn't handle horizontal tile edge cases yet.
 *
 *    XXX: Sprite format may change. I don't really like the
 *         requirement for 64-pixel stride right now, and I've been
 *         expecting to converge sprites and backgrounds to both use
 *         the same scanline loops. (Sprites would internally just be
 *         represented as sequential arrays of tiles.  On a
 *         per-scanline basis, we would use different functions to
 *         load tile indices for each layer depending on whether it's
 *         a normal bg or if it's a sprite.)
 */

static void lcd_line_bg_spr0(void)
{
    uint8_t x = 15;
    uint8_t bg_wrap = x_bg_wrap;
    register uint8_t spr0_mask = lvram.sprites[0].mask_x;
    register uint8_t spr0_x = lvram.sprites[0].x + x_bg_first_w;
    uint8_t spr0_pixel_addr = (spr0_x - 1) << 2;

    DPTR = y_bg_map;

    // First partial or full tile
    ADDR_FROM_DPTR_INC();
    MAP_WRAP_CHECK();
    ADDR_PORT = y_bg_addr_l + x_bg_first_addr;
    PIXEL_BURST(x_bg_first_w);

    // There are always 15 full tiles on-screen
    do {
	if ((spr0_x & spr0_mask) && ((spr0_x + 7) & spr0_mask)) {
	    // All 8 pixels are non-sprite

	    ADDR_FROM_DPTR_INC();
	    MAP_WRAP_CHECK();
	    ADDR_PORT = y_bg_addr_l;
	    ADDR_INC32();
	    spr0_x += 8;
	    spr0_pixel_addr += 32;

	} else {
	    // A mixture of sprite and tile pixels.

#define SPR0_OPAQUE(i)				\
	test_##i:				\
	    if (BUS_PORT == CHROMA_KEY)		\
		goto transparent_##i;		\
	    ADDR_INC4();			\

#define SPR0_TRANSPARENT_TAIL(i)		\
	transparent_##i:			\
	    ADDR_FROM_DPTR();			\
	    ADDR_PORT = y_bg_addr_l + (i*4);	\
	    ADDR_INC4();			\

#define SPR0_TRANSPARENT(i, j)			\
	    SPR0_TRANSPARENT_TAIL(i);		\
	    ADDR_FROM_SPRITE(0);		\
	    ADDR_PORT = spr0_pixel_addr + (j*4);\
	    goto test_##j;			\

#define SPR0_END()				\
	    spr0_x += 8;			\
	    spr0_pixel_addr += 32;		\
	    DPTR_INC2();			\
	    MAP_WRAP_CHECK();			\
	    continue;				\

	    // Fast path: All opaque pixels in a row.

	    // XXX: The assembly generated by sdcc for this loop is okayish, but
	    //      still rather bad. There are still a lot of gains left to be had
	    //      by using inline assembly here.

	    ADDR_FROM_SPRITE(0);
	    ADDR_PORT = spr0_pixel_addr;
	    SPR0_OPAQUE(0);
	    SPR0_OPAQUE(1);
	    SPR0_OPAQUE(2);
	    SPR0_OPAQUE(3);
	    SPR0_OPAQUE(4);
	    SPR0_OPAQUE(5);
	    SPR0_OPAQUE(6);
	    SPR0_OPAQUE(7);
	    SPR0_END();

	    // Transparent pixel jump targets

	    SPR0_TRANSPARENT(0, 1);
	    SPR0_TRANSPARENT(1, 2);
	    SPR0_TRANSPARENT(2, 3);
	    SPR0_TRANSPARENT(3, 4);
	    SPR0_TRANSPARENT(4, 5);
	    SPR0_TRANSPARENT(5, 6);
	    SPR0_TRANSPARENT(6, 7);
	    SPR0_TRANSPARENT_TAIL(7);
	    SPR0_END();
	}

    } while (--x);

    // Might be one more partial tile
    if (x_bg_last_w) {
	ADDR_FROM_DPTR_INC();
	MAP_WRAP_CHECK();
	ADDR_PORT = y_bg_addr_l;
	PIXEL_BURST(x_bg_last_w);
    }
}


/*
 * lcd_render --
 *
 *    This generates one full frame.  Most of the work is done by per-
 *    scanline rendering functions, but this handles parameter latching
 *    and we handle most Y coordinate calculations.
 */

static void lcd_render(void)
{
    uint8_t y = LCD_HEIGHT;

    /*
     * Copy a critical portion of VRAM into internal RAM, both for
     * fast access and for consistency. We copy it with interrupts
     * disabled, so a radio packet can't be processed partway through.
     */

    IEN_EN = 0;
    __asm
	mov	dptr, #(_vram + VRAM_LATCHED)
	mov	r0, #VRAM_LATCHED_SIZE
	mov	r1, #_lvram
1$:	movx	a, @dptr
	mov	@r1, a
	inc	dptr
	inc	r1
	djnz	r0, 1$
    __endasm ;
    IEN_EN = 1;

    /*
     * Top-of-frame setup
     */

    {
	uint8_t tile_pan_x = lvram.pan_x >> 3;
	uint8_t tile_pan_y = lvram.pan_y >> 3;

	y_bg_addr_l = lvram.pan_y << 5;
	y_bg_map = lvram.pan_y & 0xF8;			// Y tile * 4
	y_bg_map += tile_pan_y << 5;			// Y tile * 16
	y_bg_map += tile_pan_x << 1;			// X tile * 2;

	x_bg_last_w = lvram.pan_x & 7;
	x_bg_first_w = 8 - x_bg_last_w;
	x_bg_first_addr = (lvram.pan_x << 2) & 0x1C;
	x_bg_wrap = 20 - tile_pan_x;
    }

    do {
	uint8_t active_sprites = 0;

	/*
	 * Per-line sprite accounting. Update all Y coordinates, and
	 * see which sprites are active. (Unrolled loop here, to allow
	 * calculating masks and array addresses at compile-time.)
	 */

#define SPRITE_Y_ACCT(i)						\
	if (!(++lvram.sprites[i].y & lvram.sprites[i].mask_y)) {	\
	    active_sprites |= 1 << i;					\
	    lvram.sprites[i].addr_l += 2;				\
	}							   	\

	SPRITE_Y_ACCT(0);
	SPRITE_Y_ACCT(1);

	/*
	 * Choose a scanline renderer
	 */

	switch (active_sprites) {
	case 0x00:	lcd_line_bg(); break;
	case 0x01:	lcd_line_bg_spr0(); break;
	}

	/*
	 * Update Y axis variables
	 */

	y_bg_addr_l += 32;
	if (!y_bg_addr_l) {
	    // Next tile, with vertical wrap
	    y_bg_map += 40;
	    if (y_bg_map >= 800)
		y_bg_map -= 800;
	}

    } while (--y);
}


/**********************************************************************
 * Main Program
 **********************************************************************/

static void lcd_cmd_byte(uint8_t b)
{
    CTRL_PORT = CTRL_LCD_CMD;
    BUS_DIR = 0;
    BUS_PORT = b;
    ADDR_INC2();
    BUS_DIR = 0xFF;
    CTRL_PORT = CTRL_IDLE;
}

static void rf_reg_write(uint8_t reg, uint8_t value)
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

void main(void)
{
    // I/O port init
    BUS_DIR = 0xFF;
    ADDR_PORT = 0;
    ADDR_DIR = 0;
    CTRL_PORT = CTRL_IDLE;
    CTRL_DIR = 0x01;

    // Radio clock running
    RF_CKEN = 1;

    // Enable RX interrupt
    rf_reg_write(RF_REG_CONFIG, RF_STATUS_RX_DR);
    IEN_EN = 1;
    IEN_RF = 1;

    // Set up continuous 8-bit, 4 ksps A/D conversion with interrupt
    ADCCON3 = 0x40;
    ADCCON2 = 0x25;
    ADCCON1 = 0x80;
    IEN_MISC = 1;

    // Start receiving
    RF_CE = 1;

    while (1) {
	// Sync with master
	// XXX disabled, see refresh_alt()
	// while (vram.frame_trigger == ack_data.frame_count);

	// Sync with LCD
	//while (!CTRL_LCD_TE);
	
	lcd_cmd_byte(LCD_CMD_RAMWR);
	lcd_render();
	ack_data.frame_count++;
    }
}
