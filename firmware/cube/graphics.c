/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * Thundercracker cube firmware
 *
 * M. Elizabeth Scott <beth@sifteo.com>
 * Copyright <c> 2011 Sifteo, Inc. All rights reserved.
 */

#include <string.h>
#include "hardware.h"
#include "graphics.h"


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
 * line_bg --
 *
 *    Scanline renderer, draws a single tiled background layer.
 */

static void line_bg(void)
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
 * line_bg_spr0 --
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

static void line_bg_spr0(void)
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


static void lcd_cmd_byte(uint8_t b)
{
    CTRL_PORT = CTRL_LCD_CMD;
    BUS_DIR = 0;
    BUS_PORT = b;
    ADDR_INC2();
    BUS_DIR = 0xFF;
    CTRL_PORT = CTRL_IDLE;
}

static void lcd_data_byte(uint8_t b)
{
    BUS_DIR = 0;
    BUS_PORT = b;
    ADDR_INC2();
    BUS_DIR = 0xFF;
}

/*
 * graphics_render --
 *
 *    This generates one full frame.  Most of the work is done by per-
 *    scanline rendering functions, but this handles parameter latching
 *    and we handle most Y coordinate calculations.
 */

void graphics_render(void)
{
    uint8_t y = LCD_HEIGHT;

    // Address the LCD
    lcd_cmd_byte(LCD_CMD_RAMWR);

    /*
     * Copy a critical portion of VRAM into internal RAM, both for
     * fast access and for consistency. We copy it with interrupts
     * disabled, so a radio packet can't be processed partway through.
     */

    cli();
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
    sti();

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
	case 0x00:	line_bg(); break;
	case 0x01:	line_bg_spr0(); break;
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

    /*
     * Cleanup. Put the bus back in a sane state.
     * Especially important:
     *
     *   - Turn off the output drivers
     *
     *   - Deassert any lingering latch enables,
     *     so that future code which is expecting to
     *     emit a rising edge will actually do so.
     */

    CTRL_PORT = CTRL_IDLE;
}


void graphics_init(void)
{
    uint8_t i;

    // I/O port init
    BUS_DIR = 0xFF;
    ADDR_PORT = 0;
    ADDR_DIR = 0;
    CTRL_PORT = CTRL_IDLE;
    CTRL_DIR = 0x01;

    // LCD controller, wake up!
    lcd_cmd_byte(LCD_CMD_SLPOUT);
    
    // Display on.
    // XXX: Should wait until after the first complete frame has been rendered.
    lcd_cmd_byte(LCD_CMD_DISPON);

    // Write in 16-bit color mode
    lcd_cmd_byte(LCD_CMD_COLMOD);
    lcd_data_byte(LCD_COLMOD_16);

    /*
     * Just to make debugging easier, init VRAM with all sprites off,
     * and with sequential tiles displayed on bg0.
     */

    for (i = 0; i < NUM_SPRITES; i++) {
	vram.latched.sprites[i].mask_x = 0xFF;
	vram.latched.sprites[i].mask_y = 0xFF;
    }

    i = 0;
    do {
	uint16_t addr = ((i & 0xF) << 1) + (uint16_t)(i >> 4) * 40;
	vram.tilemap[addr] = i << 1;
	vram.tilemap[++addr] = (i >> 7) << 1;
    } while (++i);
}
