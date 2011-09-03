/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * Frivolous graphics demo.
 * M. Elizabeth Scott <beth@sifteo.com>
 *
 * Copyright <c> 2011 Sifteo, Inc. All rights reserved.
 */

#include <stdint.h>
#include "hardware.h"

#pragma disable_warning 84   // Variable may be used before initialization

#define ANGLE_90      	64
#define ANGLE_180      	128
#define ANGLE_270      	192
#define ANGLE_360	256

#define CHROMA_KEY	0xF5

#define TILE8(x, y)	(tilemap.bytes[(x) + ((y)<<3)])			// 8-bit tiles, 8x100 grid
#define TILE20(x, y)	(tilemap.words[(x) + ((y)<<4) + ((y)<<2)])	// 16-bit tiles, 20x20 grid

#define NUM_SPRITES	4

__xdata union {
    uint8_t bytes[800];
    uint16_t words[400];
} tilemap;

// For now, a very limited number of 32x32 sprites
__near struct {
    uint8_t x, y;
    int8_t xd, yd;
} oam[NUM_SPRITES];

static const uint16_t __code earthbound_fourside_160[] =
#include "earthbound_fourside_160.h"


/*
 * One-off LCD command byte. Slow but who cares?
 */
void lcd_cmd_byte(uint8_t b)
{
    CTRL_PORT = CTRL_LCD_CMD;
    BUS_DIR = 0;
    BUS_PORT = b;
    ADDR_INC2();
    BUS_DIR = 0xFF;
    CTRL_PORT = CTRL_IDLE;
}

/*
 * One-off LCD data byte. Really slow, for setup only.
 */
void lcd_data_byte(uint8_t b)
{
    BUS_DIR = 0;
    BUS_PORT = b;
    ADDR_INC2();
    BUS_DIR = 0xFF;
}

void hardware_init(void)
{
    BUS_DIR = 0xFF;
    ADDR_PORT = 0;
    ADDR_DIR = 0;
    CTRL_PORT = CTRL_IDLE;
    CTRL_DIR = 0x01;

    lcd_cmd_byte(LCD_CMD_SLPOUT);
    lcd_cmd_byte(LCD_CMD_DISPON);
    lcd_cmd_byte(LCD_CMD_TEON);

    lcd_cmd_byte(LCD_CMD_COLMOD);
    lcd_data_byte(LCD_COLMOD_16);
}

void lcd_begin_frame(void)
{
    lcd_cmd_byte(LCD_CMD_RAMWR);

    // Optional: Vertical sync
    //while (!CTRL_LCD_TE);
}

static void lcd_addr_burst(uint8_t pixels)
{
    uint8_t hi = pixels >> 3;
    uint8_t low = pixels & 0x7;

    if (hi)
	// Fast DJNZ loop, 8-pixel bursts
	do {
	    ADDR_INC32();
	} while (--hi);

    if (low)
	// Fast DJNZ loop, single pixels
	do {
	    ADDR_INC4();
	} while (--low);
}

/*
 * Flexible copy from any flash address to the LCD
 */
void lcd_flash_copy(uint32_t addr, uint16_t pixel_count)
{
    for (;;) {
	uint8_t addr_high = (uint8_t)(addr >> 13) & 0xFE;
	uint8_t addr_mid  = (uint8_t)(addr >> 6) & 0xFE;
	uint8_t addr_low  = addr << 1;

	ADDR_PORT = addr_high;
	CTRL_PORT = CTRL_IDLE | CTRL_FLASH_LAT2;
	ADDR_PORT = addr_mid;
	CTRL_PORT = CTRL_FLASH_OUT | CTRL_FLASH_LAT1;
	ADDR_PORT = addr_low;

	if (pixel_count > 64 && addr_low == 0) {
	    lcd_addr_burst(64);
	    pixel_count -= 64;
	    addr += 128;

	} else {
	    uint8_t burst_count = 32 - (addr_low >> 3);

	    if (burst_count >= pixel_count) {
		lcd_addr_burst(burst_count);
		CTRL_PORT = CTRL_IDLE;
		return;
	    } else {
		lcd_addr_burst(burst_count);
		pixel_count -= burst_count;
		addr += burst_count << 1;
	    }
	}
    }
}

/*
 * Optimized copy, assumes everything is perfectly aligned and always
 * blits a full screen of data.
 */
void lcd_flash_copy_fullscreen(uint32_t addr)
{
    uint8_t addr_high = (uint8_t)(addr >> 13) & 0xFE;
    uint8_t addr_mid  = (uint8_t)(addr >> 6) & 0xFE;
    uint8_t chunk = 0;

    do {
	ADDR_PORT = addr_high;
	CTRL_PORT = CTRL_IDLE | CTRL_FLASH_LAT2;
	ADDR_PORT = addr_mid;
	CTRL_PORT = CTRL_FLASH_OUT | CTRL_FLASH_LAT1;
	ADDR_PORT = 0;

	lcd_addr_burst(64);

	if (!(addr_mid += 2)) addr_high += 2;
    } while (--chunk);
}

/*
 * Arbitrary-sized chromakey blit. If we hit a pixel that matches our
 * chromakey, we show data from a background image instead of the
 * foreground.
 */
void lcd_flash_chromakey(uint32_t fgAddr, uint32_t bgAddr, uint16_t pixel_count)
{
    do {
	uint8_t fg_high = (uint8_t)(fgAddr >> 13) & 0xFE;
	uint8_t fg_mid  = (uint8_t)(fgAddr >> 6) & 0xFE;
	uint8_t fg_low  = fgAddr << 1;

	uint8_t bg_high = (uint8_t)(bgAddr >> 13) & 0xFE;
	uint8_t bg_mid  = (uint8_t)(bgAddr >> 6) & 0xFE;
	uint8_t bg_low  = bgAddr << 1;

	// XXX: fg/bg may have different alignment
	uint8_t burst_count = 32 - (fg_low >> 3);
	uint8_t i;
	if (burst_count > pixel_count)
	    burst_count = pixel_count;

	ADDR_PORT = fg_high;
	CTRL_PORT = CTRL_IDLE | CTRL_FLASH_LAT2;
	ADDR_PORT = fg_mid;
	CTRL_PORT = CTRL_FLASH_OUT | CTRL_FLASH_LAT1;
	ADDR_PORT = fg_low;

	fgAddr += burst_count << 1;
	bgAddr += burst_count << 1;
	pixel_count -= burst_count;

	for (i = burst_count; i--;) {
	    if (BUS_PORT == CHROMA_KEY) {
		// Transparent pixel, swap addresses

		ADDR_PORT = bg_high;
		CTRL_PORT = CTRL_IDLE | CTRL_FLASH_LAT2;
		ADDR_PORT = bg_mid;
		CTRL_PORT = CTRL_FLASH_OUT | CTRL_FLASH_LAT1;
		ADDR_PORT = bg_low + ((burst_count - i - 1) << 2);
		
		ADDR_INC4();

		ADDR_PORT = fg_high;
		CTRL_PORT = CTRL_IDLE | CTRL_FLASH_LAT2;
		ADDR_PORT = fg_mid;
		CTRL_PORT = CTRL_FLASH_OUT | CTRL_FLASH_LAT1;
		ADDR_PORT = fg_low + ((burst_count - i - 1) << 2);

	    } else {
		ADDR_INC4();
	    }
	}
    } while (pixel_count);
}

/*
 * Optimized chroma-key blit. Assumes perfect alignment, and copies a
 * full screen of data.
 */
void lcd_flash_chromakey_fullscreen(uint32_t fgAddr, uint32_t bgAddr)
{
    uint8_t fg_high = (uint8_t)(fgAddr >> 13) & 0xFE;
    uint8_t fg_mid  = (uint8_t)(fgAddr >> 6) & 0xFE;
    
    uint8_t bg_high = (uint8_t)(bgAddr >> 13) & 0xFE;
    uint8_t bg_mid  = (uint8_t)(bgAddr >> 6) & 0xFE;

    uint8_t chunk = 0;

    ADDR_PORT = fg_high;
    CTRL_PORT = CTRL_IDLE | CTRL_FLASH_LAT2;
    ADDR_PORT = fg_mid;
    CTRL_PORT = CTRL_FLASH_OUT | CTRL_FLASH_LAT1;
    ADDR_PORT = 0;

    do {
	uint8_t cycle = 8;

	do {
	    // Partially unrolled loop, 64 = 8x8

#define CHROMA_TEST 							\
	    if (BUS_PORT == CHROMA_KEY) { 				\
		uint8_t a = ADDR_PORT; 					\
									\
		ADDR_PORT = bg_high; 					\
		CTRL_PORT = CTRL_IDLE | CTRL_FLASH_LAT2;		\
		ADDR_PORT = bg_mid; 					\
		CTRL_PORT = CTRL_FLASH_OUT | CTRL_FLASH_LAT1;		\
		ADDR_PORT = a; 						\
									\
		ADDR_INC4();						\
									\
		ADDR_PORT = fg_high;					\
		CTRL_PORT = CTRL_IDLE | CTRL_FLASH_LAT2;		\
		ADDR_PORT = fg_mid;					\
		CTRL_PORT = CTRL_FLASH_OUT | CTRL_FLASH_LAT1;		\
		ADDR_PORT = a + 4;					\
	    } else {							\
		ADDR_INC4();	\
	    }

	    CHROMA_TEST CHROMA_TEST CHROMA_TEST CHROMA_TEST
	    CHROMA_TEST CHROMA_TEST CHROMA_TEST CHROMA_TEST

#undef CHROMA_TEST

	} while (--cycle);

	if (!(fg_mid += 2)) fg_high += 2;
	if (!(bg_mid += 2)) bg_high += 2;

	// Must load the first pixel of the next chunk, for the colorkey test
	ADDR_PORT = fg_high;
	CTRL_PORT = CTRL_IDLE | CTRL_FLASH_LAT2;
	ADDR_PORT = fg_mid;
	CTRL_PORT = CTRL_FLASH_OUT | CTRL_FLASH_LAT1;
	ADDR_PORT = 0;

    } while (--chunk);
}

/*
 * Tile graphics!
 *
 * There are a lot of tile modes we could use. For example, the
 * 7x3=21-bit address bus is really well suited for a layout that has
 * 128 accessible 8x8 tiles at a time, since we then need to change
 * only one of the address bytes per-tile.
 *
 * However, even a relatively simple game like Chroma Shuffle couldn't
 * use that mode. with an 8x8 tile size, it would need at least 640
 * tiles, which is quite a bit beyond this hypothetical mode's
 * 128-tile limit.
 *
 * There are probably tons of uses for a 8x8 mode with a 16-bit tile
 * ID- for example, games that use a lot of text, and need to be able
 * to combine text with graphics.
 *
 * But for this demo, I'm going the other direction and implementing a
 * 16x16 mode with 8-bit tile IDs. Each tile occupies an aligned
 * 512-byte chunk of memory. The tilemap is 8x8, and only occupies 64
 * bytes.
 *
 * For simplicity, we're still requiring that a tile segment begins on
 * a 16kB boundary.
 *
 * This mode isn't especially suited for arbitrary text, but it's good
 * for puzzle games where the game pieces are relatively large.
 */
void lcd_render_tiles_16x16_8bit_8wide(uint8_t segment)
{
    uint8_t map_index = 0;
    uint8_t y_low = 0;
    uint8_t y_high = 0;

    do {
	// Loop over map columns
	do {
	    uint8_t tile_index = tilemap.bytes[map_index++];
		
	    ADDR_PORT = segment + ((tile_index >> 4) & 0x0E);
	    CTRL_PORT = CTRL_IDLE | CTRL_FLASH_LAT2;
	    ADDR_PORT = y_high + (tile_index << 3);
	    CTRL_PORT = CTRL_FLASH_OUT | CTRL_FLASH_LAT1;
	    ADDR_PORT = y_low;

	    // Burst out one row of one tile (16 pixels)
	    lcd_addr_burst(16);
	    
	} while (map_index & 7);

	/*
	 * We can store four tile rows per address chunk.
	 * After that, roll over y_high. We'll do four such
	 * roll-overs per 16 pixel tile.
	 */

	y_low += 64;
	if (y_low)
	    map_index += 0xF8;  // No map advancement
	else {
	    y_high += 2;
	    if (y_high & 8)
		y_high = 0;
	    else
		map_index += 0xF8;  // No map advancement
	}
    } while (!(map_index & 64));
}

/*
 * Tile graphics mode: 8x8 tiles, 16-bit tile indices, 20x20
 * tile map, with support for pixel panning in X and Y.
 *
 * This lines up fairly well with our addressing scheme. Each tile
 * is 128 bytes, which lines up perfectly with our low 7 bits of
 * addressing. To avoid unnecessary shifting, we directly assign
 * the two-byte tile index to the lower and upper address parts.
 *
 * This means the entirety of flash is addressable from the tilemap.
 * It also means that bits 0 and 8 of the tile index are reserved, and
 * currently must be zero. In the future these could be used for
 * transparency, if we supported multiple tile layers.
 */
void lcd_render_tiles_8x8_16bit_20wide(uint8_t pan_x, uint8_t pan_y)
{
    uint8_t tile_pan_x = pan_x >> 3;
    uint8_t tile_pan_y = pan_y >> 3;
    uint8_t line_addr = pan_y << 5;
    uint8_t first_column_addr = (pan_x << 2) & 0x1C;
    uint8_t last_tile_width = pan_x & 7;
    uint8_t first_tile_width = 8 - last_tile_width;
    __xdata uint8_t *map = &tilemap.bytes[(tile_pan_y << 5) + (tile_pan_y << 3) + (tile_pan_x << 1)];
    uint8_t y = LCD_HEIGHT;

    do {
	uint8_t x;

	/*
	 * XXX: There is a HUGE optimization opportunity here with regard to map addressing.
	 *      The dptr manipulation code that SDCC generates here is very bad... and it's
	 *      possible we might want to use some of the nRF24LE1's specific features, like
	 *      the PDATA addressing mode.
	 */

	// First tile on the line, (1 <= width <= 8)
	ADDR_PORT = *(map++);
	CTRL_PORT = CTRL_FLASH_OUT | CTRL_FLASH_LAT1;
	ADDR_PORT = *(map++);
	CTRL_PORT = CTRL_FLASH_OUT | CTRL_FLASH_LAT2;
	ADDR_PORT = line_addr + first_column_addr;
	lcd_addr_burst(first_tile_width);
	
	// There are always 15 full tiles on-screen
	x = 15;
	do {
	    ADDR_PORT = *(map++);
	    CTRL_PORT = CTRL_FLASH_OUT | CTRL_FLASH_LAT1;
	    ADDR_PORT = *(map++);
	    CTRL_PORT = CTRL_FLASH_OUT | CTRL_FLASH_LAT2;
	    ADDR_PORT = line_addr;
	    ADDR_INC32();
	} while (--x);

	// Might be one more partial tile, (0 <= width <= 7)
	if (last_tile_width) {
	    ADDR_PORT = map[0];
	    CTRL_PORT = CTRL_FLASH_OUT | CTRL_FLASH_LAT1;
	    ADDR_PORT = map[1];
	    CTRL_PORT = CTRL_FLASH_OUT | CTRL_FLASH_LAT2;
	    ADDR_PORT = line_addr;
	    lcd_addr_burst(last_tile_width);
	}

	line_addr += 32;
	if (line_addr)
	    map -= 32;
	else
	    map += 8;

    } while (--y);
}

void lcd_render_sprites_32x32(uint8_t segment)
{
    // XXX: Trick to force SDCC to allocate 'mask' as a register rather than using an immediate.
    volatile uint8_t m = 0xE0;

    uint8_t x, y;

    // We keep the segment constant. Everything has to fit in 16 kB for this mode.
    ADDR_PORT = segment;
    CTRL_PORT = CTRL_IDLE | CTRL_FLASH_LAT2;

#define SPRITE_ROW(i)							\
	if (oam[i].y++ & mask)						\
	    sx##i = 0x80;						\
	else {								\
	    sx##i = oam[i].x;						\
	    sah##i = (oam[i].y & 0x1E) | (i << 5);			\
	    sal##i = (oam[i].y & 1) << 7;				\
	    n_active++;							\
	    active_id = i;						\
	}

#define SPRITE_TEST(lbl, i)    						\
	lbl##_sprite_test_##i:						\
	    if (!(sx##i & mask))					\
		goto lbl##_sprite_target_##i;			      	\

#define SPRITE_TARGET(lbl, i, _X)    					\
	lbl##_sprite_target_##i:					\
	    CTRL_PORT = CTRL_IDLE;					\
	    ADDR_PORT = sah##i;						\
	    CTRL_PORT = CTRL_FLASH_OUT | CTRL_FLASH_LAT1;		\
	    ADDR_PORT = sal##i | (sx##i << 2);				\
	    if (BUS_PORT != CHROMA_KEY) {				\
		ADDR_INC4();						\
		continue;						\
	    } else {							\
		_X;							\
	    }

#define BG_BEGIN()							\
    	CTRL_PORT = CTRL_IDLE;						\
	BUS_DIR = 0;							\
	BUS_PORT = 0xFF;						\

#define BG_END()							\
	BUS_DIR = 0xFF;							\

#define BG_PIXEL()							\
        BG_BEGIN();							\
	ADDR_INC4();							\
	BG_END();

#define BG_TARGET(lbl)							\
    	lbl##_background:						\
	    BG_PIXEL();							\
	    continue;

    /*
     * XXX: A lot more optimization here should be possible, since we can easily
     *      decompose the scanline into at most three segments: background before,
     *      sprite body, and background after.
     */
#define SPRITE_SINGLE(i)						\
        case i:								\
	    ADDR_PORT = sah##i;						\
	    CTRL_PORT = CTRL_FLASH_OUT | CTRL_FLASH_LAT1;		\
	    do {							\
		sx##i++;						\
		ADDR_PORT = sal##i | (sx##i << 2);			\
		if (!(sx##i & mask)) {					\
		    if (BUS_PORT != CHROMA_KEY) {			\
			ADDR_INC4();					\
		    } else {						\
			goto single##i##_bg;				\
		    }							\
		} else {						\
 		    single##i##_bg:					\
		    BG_PIXEL();						\
		    CTRL_PORT = CTRL_FLASH_OUT;				\
		}							\
	    } while (--x);						\
	    break;	

    y = LCD_HEIGHT;
    do {
	uint8_t sah0, sah1, sah2, sah3;
	uint8_t sal0, sal1, sal2, sal3;

	register uint8_t mask;
	register uint8_t sx0, sx1, sx2, sx3;

	uint8_t n_active = 0;
	uint8_t active_id;
	
	x = LCD_WIDTH;
	mask = m;

	/*
	 * Prepare each row. If a sprite doesn't occur at all on this
	 * row, disable it by interpreting its column position as
	 * off-screen, so that the X test below will hide the sprite.
	 *
	 * We'll also pre-calculate the Y portion of the address for
	 * any visible sprite.
	 */
	SPRITE_ROW(0);
	SPRITE_ROW(1);
	SPRITE_ROW(2);
        SPRITE_ROW(3);

	/*
	 * This code stands in for an AWESOME SCANLINE CLASSIFIER
	 *
	 * We use multiple scanline rendering algorithms, depending on
	 * how many sprites are active during this line. Later there
	 * might be a fast way to pre-classify these, and have a table
	 * of function pointers per-scanline? *shrug loudly*
	 */

	if (n_active == 0) {
	    /* No sprites enabled. Just do a background burst */
	    
	    BG_BEGIN();
	    lcd_addr_burst(LCD_WIDTH);
	    BG_END();

	} else if (n_active == 1) {
	    /* Just a single sprite */

	    switch (active_id) {
		SPRITE_SINGLE(0);
		SPRITE_SINGLE(1);
		SPRITE_SINGLE(2);
		SPRITE_SINGLE(3);
	    }

	} else {
	    /* Generic 4-sprite rendering */

	    do {
		sx0++;
		sx1++;
		sx2++;
		sx3++;
		SPRITE_TEST(r4, 0);
		SPRITE_TEST(r4, 1);
		SPRITE_TEST(r4, 2);
		SPRITE_TEST(r4, 3);
		BG_TARGET(r4);
		SPRITE_TARGET(r4, 0, goto r4_sprite_test_1);
		SPRITE_TARGET(r4, 1, goto r4_sprite_test_2);
		SPRITE_TARGET(r4, 2, goto r4_sprite_test_3);
		SPRITE_TARGET(r4, 3, goto r4_background);
	    } while (--x);
	}

    } while (--y);

    oam[0].y += 128;
    oam[1].y += 128;
    oam[2].y += 128;
    oam[3].y += 128;
}

// Signed 8-bit sin()
int8_t sin8(uint8_t angle)
{
    static const __code int8_t lut[] = {
	0x00, 0x03, 0x06, 0x09, 0x0c, 0x10, 0x13, 0x16,
	0x19, 0x1c, 0x1f, 0x22, 0x25, 0x28, 0x2b, 0x2e,
	0x31, 0x33, 0x36, 0x39, 0x3c, 0x3f, 0x41, 0x44,
	0x47, 0x49, 0x4c, 0x4e, 0x51, 0x53, 0x55, 0x58,
	0x5a, 0x5c, 0x5e, 0x60, 0x62, 0x64, 0x66, 0x68,
	0x6a, 0x6b, 0x6d, 0x6f, 0x70, 0x71, 0x73, 0x74,
	0x75, 0x76, 0x78, 0x79, 0x7a, 0x7a, 0x7b, 0x7c,
	0x7d, 0x7d, 0x7e, 0x7e, 0x7e, 0x7f, 0x7f, 0x7f,
    };

    if (angle & 0x80) {
	if (angle & 0x40)
	    return -lut[255 - angle];
	else
	    return -lut[angle & 0x3F];
    } else {
	if (angle & 0x40)
	    return lut[127 - angle];
	else
	    return lut[angle];
    }
}

// This is the magic size, 64x128, 16KB, 14 bits. Make it part of your consciousness.
void lcd_render_affine_64x128(uint8_t segment, uint8_t angle, uint8_t scale)
{
    uint8_t y, x;

    uint16_t x_acc = 0x80;  // 1/2 texel rounding adjustment
    uint16_t y_acc = 0x80;
    
    uint16_t sin_val = (sin8(angle) * (int16_t)scale) >> 6;
    register uint16_t sin2_val = sin_val + sin_val;
    register uint16_t cos_val = (sin8(ANGLE_90 - angle) * (int16_t)scale) >> 6;

    // We keep the segment constant. Everything has to fit in 16 kB for this mode.
    ADDR_PORT = segment;
    CTRL_PORT = CTRL_IDLE | CTRL_FLASH_LAT2;

    y = LCD_HEIGHT;
    do {
	register uint16_t x_acc_row = x_acc;
	register uint16_t y_acc_row = y_acc;

	x = LCD_WIDTH / 8;
	do {

	    /*
	     * For speed, output unrolled bursts of two pixels, with
	     * only half-precision on the Y texture axis. This causes
	     * some additional rotation artifacts, but when the texture
	     * is nearly right-side-up the artifacts are minimal.
	     */

#define AFFINE_PIXELS()					       	\
	    CTRL_PORT = CTRL_FLASH_OUT;					\
	    ADDR_PORT = (uint8_t)((y_acc_row += sin2_val) >> 8) << 1;	\
	    CTRL_PORT = CTRL_FLASH_OUT | CTRL_FLASH_LAT1;		\
	    ADDR_PORT = (uint8_t)((x_acc_row += cos_val) >> 8) << 2;	\
	    ADDR_INC4();						\
	    ADDR_PORT = (uint8_t)((x_acc_row += cos_val) >> 8) << 2;	\
	    ADDR_INC4();

	    AFFINE_PIXELS();
	    AFFINE_PIXELS();
	    AFFINE_PIXELS();
	    AFFINE_PIXELS();

	} while (--x);

	x_acc -= sin_val;
	y_acc += cos_val;

    } while (--y);
}

// Common lookup table used for fixed-point distance and scale calculations:
//   [int((64 << 8) / (width+0.5)) for width in range(128)]
static const __code uint16_t lut_64_fp8_div[] = {
    32768, 10922, 6553, 4681, 3640, 2978, 2520, 2184, 1927, 1724, 1560, 1424, 1310,
    1213, 1129, 1057, 992, 936, 885, 840, 799, 762, 728, 697, 668, 642, 618, 595,
    574, 555, 537, 520, 504, 489, 474, 461, 448, 436, 425, 414, 404, 394, 385, 376,
    368, 360, 352, 344, 337, 330, 324, 318, 312, 306, 300, 295, 289, 284, 280, 275,
    270, 266, 262, 258, 254, 250, 246, 242, 239, 235, 232, 229, 225, 222, 219, 217,
    214, 211, 208, 206, 203, 201, 198, 196, 193, 191, 189, 187, 185, 183, 181, 179,
    177, 175, 173, 171, 169, 168, 166, 164, 163, 161, 159, 158, 156, 155, 153, 152,
    151, 149, 148, 146, 145, 144, 143, 141, 140, 139, 138, 137, 135, 134, 133, 132,
    131, 130, 129, 128
};

/*
 * Render one scanline, with 1-dimensional texture mapping suitable for
 * "rotating cube" type effects, or for 2.5-D ray casting.
 * Source material is a 64-pixel wide texture, at the specified scanline address.
 */
const uint8_t texmap_segment = 0x8c000 >> 13;
const uint8_t texmap_bg_left = 0xFF;
const uint8_t texmap_bg_right = 0xFF;
void lcd_render_1d_texture(uint8_t row_addr, uint8_t width)
{
    uint8_t l_margin, r_margin;
    register uint16_t scale;
    register uint16_t acc = 0x80;  // 1/2 pixel rounding adjustment
    
    // Scaling calculations, per-scanline
    r_margin = l_margin = LCD_WIDTH - width;
    l_margin = l_margin >> 1;
    r_margin = (r_margin + 1) >> 1;
    scale = lut_64_fp8_div[width];
    
    if (l_margin) {
        CTRL_PORT = CTRL_IDLE;
        BUS_DIR = 0;
        BUS_PORT = texmap_bg_left;
        lcd_addr_burst(l_margin);
        BUS_DIR = 0xFF;
    }
    
    if (width) {
        ADDR_PORT = texmap_segment;
        CTRL_PORT = CTRL_IDLE | CTRL_FLASH_LAT2;
        ADDR_PORT = row_addr;
        CTRL_PORT = CTRL_FLASH_OUT | CTRL_FLASH_LAT1;
        
        do {
            ADDR_PORT = (uint8_t)((acc += scale) >> 8) << 2;
            ADDR_INC4();
        } while (--width);
    }

    if (r_margin) {
        CTRL_PORT = CTRL_IDLE;
        BUS_DIR = 0;
        BUS_PORT = texmap_bg_right;
        lcd_addr_burst(r_margin);
        BUS_DIR = 0xFF;
    }
}

void gems_draw_gem(uint8_t x, uint8_t y, uint8_t index)
{
    // A gem is 32x32, a.k.a. a 2x2 tile grid
    index <<= 2;
    TILE8(0 + (x << 1), 0 + (y << 1)) = 0 + index;
    TILE8(1 + (x << 1), 0 + (y << 1)) = 1 + index;
    TILE8(0 + (x << 1), 1 + (y << 1)) = 2 + index;
    TILE8(1 + (x << 1), 1 + (y << 1)) = 3 + index;
}

uint32_t xor128(void)
{
    // Simple PRNG.
    // Reference: http://en.wikipedia.org/wiki/Xorshift

    static uint32_t x = 123456789;
    static uint32_t y = 362436069;
    static uint32_t z = 521288629;
    static uint32_t w = 88675123;
    uint32_t t;
 
    t = x ^ (x << 11);
    x = y; y = z; z = w;
    return w = w ^ (w >> 19) ^ (t ^ (t >> 8));
}

void text_char(uint8_t x, uint8_t y, char c)
{
    const uint32_t base_addr = 0x90000 >> 7;

    uint16_t char_index = c - ' ';
    uint16_t addr = base_addr + (char_index << 1);
    uint16_t tile = ((addr << 1) & 0xFE) | ((addr << 2) & 0xFE00);

    TILE20(x, y) = tile;
    TILE20(x, y+1) = tile + 2;
}

void text_string(uint8_t x, uint8_t y, const char *str)
{
    char c;
    while ((c = *(str++)))
	text_char(x++, y, c);
}    

/*
 * Extremely simplified ray-caster. Currently this is not fully perspective-correct, it uses
 * an orthographic projection for the ray intersection, but we still have some (somewhat fake)
 * nonlinear scaling based on distance.
 *
 * Instead of working off of a grid-based map, for this demo we
 * have functions to check individual rays, and we'll store the result if the given
 * ray is closer than the last stored ray. So it's a bit like a 1D z-buffer :)
 */

uint16_t ray_distance = 0x7FFF;
uint8_t ray_texcoord;

void ray_render(void)
{
    uint16_t width = 0x7000 / ray_distance;
    if (width > LCD_WIDTH)
        width = LCD_WIDTH;
    
    lcd_render_1d_texture(ray_texcoord, width);
    
    ray_distance = 0x7FFF;
}

void ray_test_segment(int16_t x1, int16_t y1, int16_t x2, int16_t y2)
{
    int8_t mx = x2-x1;
    int8_t my = y2-y1;
    uint16_t a;
    uint16_t d;
    
    if (!my)
        // Edge-on segment, invisible
        return;

    /*
     * Solve for the texture coordinate first.
     */
    a = (-y1 << 6) / my;
    if (a >= 64)
        return;
    
    /*
     * If we're actually in the segment, solve distance
     */
    d = x1 - y1 * mx / my;
    
    if (d < ray_distance) {
        // New winner for the closest ray
        ray_distance = d;
        ray_texcoord = a << 1;
    }
}


/*************************************************************
 * IT IS DEMO TIME.
 */

// Rotating cube effect
void demo_cube(void)
{
    uint16_t frame;
    
    for (frame = 0; frame < 256; frame++) {
        uint8_t y = LCD_HEIGHT;
        
        /* 
         * Since this is 2.5D, our "cube" is actually a rotating square.
         * Calculate the vertices.
         */
        uint8_t angle = 0x20 + (0x3F & (frame << 1));
        const int16_t x0 = 325;
        const int16_t y0 = -64;
        
        int8_t s = sin8(angle)/2;
        int8_t c = sin8(ANGLE_90 - angle)/2;
        
	lcd_begin_frame();
        
        do {
            ray_test_segment(x0-c, y+y0-s, x0-s, y+y0+c);
            ray_test_segment(x0-s, y+y0+c, x0+c, y+y0+s);
            
            ray_render();
        } while (--y);
    }
}

// Static 16x16 tile graphics (Chroma Extra-lite)
void demo_gems(void)
{
    uint16_t frame;
    uint8_t x, y;

    // Draw a uniform grid of gems, all alike.
    for (y = 0; y < 4; y++)
	for (x = 0; x < 4; x++)
	    gems_draw_gem(x, y, 0);

    for (frame = 0; frame < 256; frame++) {
	lcd_begin_frame();
	lcd_render_tiles_16x16_8bit_8wide(0x68000 >> 13);

	// Mix it up, replace a few pseudo-random gems.
	{
	    uint32_t r = xor128();
	    uint8_t i = 4;

	    do {
		uint8_t gem = (uint8_t)r % 40;
		uint8_t x = (r >> 6) & 3;
		uint8_t y = (r >> 4) & 3;

		gems_draw_gem(x, y, gem);

		r >>= 8;
	    } while (--i);
	}
    }
}

// Dynamic 32x32 sprites
void demo_monsters(void)
{
    uint16_t frame;
    uint8_t i;

    // Init sprites
    for (i = 0; i < NUM_SPRITES; i++) {
	do {
    	    uint32_t r = xor128();

	    oam[i].x = 170 + ((r >> 0) & 63);
	    oam[i].y = 170 + ((r >> 8) & 63);
	    oam[i].xd = ((r >> 15) & 7) - 4;
	    oam[i].yd = ((r >> 23) & 7) - 4;
	} while (oam[i].xd == 0 && oam[i].yd == 0);
    }

    for (frame = 0; frame < 256; frame++) {
	lcd_begin_frame();
	lcd_render_sprites_32x32(0x88000 >> 13);

	// Update sprites
	for (i = 0; i < NUM_SPRITES; i++) {
	    oam[i].x = oam[i].x + oam[i].xd;
	    oam[i].y = oam[i].y + oam[i].yd;

	    // Bounce (Yes, very weird coord system)
	    if (oam[i].x < 160)
		oam[i].xd = -oam[i].xd;
	    if (oam[i].y < 160)
		oam[i].yd = -oam[i].yd;
	}
    }
}

// Full-screen background only
void demo_fullscreen_bg(void)
{
    uint16_t frame;

    for (frame = 0; frame < 256; frame++) {
	uint32_t bg_addr = 0x40000LU + ((uint32_t)(frame & 0xFF) << (LCD_ROW_SHIFT + 1));
	lcd_begin_frame();
	lcd_flash_copy_fullscreen(bg_addr);
    }
}

// Full-screen sprite only
void demo_owlbear_sprite(void)
{
    uint16_t frame;

    for (frame = 0; frame < 256; frame++) {
	uint8_t spr_f = (frame >> 2) & 7;
	uint32_t spr_addr = (uint32_t)spr_f << 15;
	lcd_begin_frame();
	lcd_flash_copy_fullscreen(spr_addr);
    }
}

// Chroma key
void demo_owlbear_chromakey(void)
{
    uint16_t frame;

    for (frame = 0; frame < 256; frame++) {
	uint8_t spr_f = (frame >> 1) & 7;
	uint32_t spr_addr = (uint32_t)spr_f << 15;
	uint32_t bg_addr = 0x40000LU + ((uint32_t)(frame & 0x7F) << (LCD_ROW_SHIFT + 2));
	lcd_begin_frame();
	lcd_flash_chromakey_fullscreen(spr_addr, bg_addr);
    }
}

// Some oldskool rotozooming, why not?
void demo_rotozoom(void)
{
    uint16_t frame;
    
    for (frame = 0; frame < 128; frame++) {
	uint8_t frame_l = frame;
	lcd_begin_frame();
	lcd_render_affine_64x128(0x8c000 >> 13, -frame, 0xa0 - frame);
    }
}	

// Scrollable 8x8 pixel tiles, in a 20x20 map
void demo_tile_panning(void)
{
    uint16_t frame;
    uint16_t i;

    for (i = 0; i < 400; i++)
	tilemap.words[i] = earthbound_fourside_160[i];

    for (frame = 0; frame < 256; frame++) {
	lcd_begin_frame();
	lcd_render_tiles_8x8_16bit_20wide(16 + (sin8(frame << 4) >> 5),
					  16 + (sin8(frame << 3) >> 5));
    }
}

// Text demo, with an 8x16 antialiased font
void demo_text(void)
{
    uint16_t frame;
    uint8_t x, y;
    static const __code char scroller[] =
	"Whoaaaa, it's one of those old-fashioned demoscene text scrollers, "
	"but it's going so fast that you can't even read it! What's this doing "
	"in a microcontroller anyway??? ";

    for (x = 0; x < 20; x++)
	for (y = 0; y < 20; y += 2)
	    text_char(x, y, ' ');
    
    text_string(3, 6, "Hello, world!");
    text_string(8, 9, "^_^");

    for (frame = 0; frame < 256; frame++) {
	lcd_begin_frame();
        lcd_render_tiles_8x8_16bit_20wide(12 + (sin8(frame << 1) >> 5),
					  16 + (sin8(frame << 3) >> 5));

	x = 19;
	do {
	    text_char(x, 14, scroller[(x + frame) % (sizeof scroller-1)]);
	} while (--x);
    }
}

// Sine wave 1D texture scaler
void demo_sin_scaler(void)
{
    uint16_t frame;
    uint8_t row_addr;
    
    for (frame = 0; frame < 256; frame++) {
	lcd_begin_frame();

        row_addr = 0;
        do {
            lcd_render_1d_texture(row_addr, 64 + sin8(frame*4 + row_addr)/4);
        } while (row_addr += 2);
    }
}

void main(void)
{
    hardware_init();

    /*
     * Other video modes to try:
     *
     *  - Low-res framebuffer modes
     *
     *      - 64x64, 2-bit indexed color
     *      - 32x32, 4-bit indexed color, smooth panning
     *      - 32x32, 8-bit indexed color
     *      - 32x32, 8-bit RGB
     *
     *  - "Master" tile/sprite mode: A generic and pretty featureful
     *    mode for which we can implement optimized subsets when
     *    particular features aren't in use.
     *
     *      - 8 sprites on an 8x8-pixel 20x20 tile bg
     *      - 8 normal sprites, colorkeyed 64x128 affine sprite, tile BG
     *      - 8 sprites, each with one of 2 affine transforms optionally
     *        applied, tile BG with optional affine transform
     *
     *  - Tile rotation? Try using the two reserved tile index bits
     *    to change tile orientation?
     *
     *  - "Bootstrap" mode, for an idle screen that we show when
     *    cubes are disconnected. Also used for asset upload progress.
     *
     *      - Shouldn't make use of either xdata or flash!
     *      - Could use very small tileset, 2-bit pallette, from code memory
     *
     *  - At least one mode that would support 'sliding' game elements.
     *    Either multiple tile backgrounds (with chroma key), or a sprite
     *    mode which uses a tile-based layout such that areas of the screen
     *    could be dynamically converted between sprites and backgrounds.
     *
     *  - I think we could do some cool things with a mode that supported
     *    linear (X-axis only) scaling with per-scanline control over scale
     *    factor and source address. This would allow some limited 3D rotation
     *    effects, as well as 2.5D "ray-casting" style game engines.
     */
     
    while (1) {
        demo_cube();
        demo_sin_scaler();
        demo_rotozoom();
        demo_text();
        demo_fullscreen_bg();
        demo_owlbear_sprite();
        demo_owlbear_chromakey();
        demo_gems();
        demo_tile_panning();
        demo_monsters();
    }
}
