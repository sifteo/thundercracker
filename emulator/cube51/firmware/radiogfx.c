/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * nRF Radio + Graphics Engine + ??? = Profit!
 *
 * M. Elizabeth Scott <beth@sifteo.com>
 * Copyright <c> 2011 Sifteo, Inc. All rights reserved.
 */

#include <stdint.h>
#include "hardware.h"

#define NUM_SPRITES  	2
#define CHROMA_KEY	0xF5

static __xdata union {
    uint8_t bytes[1024];
    struct {
	uint8_t tilemap[800];
	uint8_t pan_x, pan_y;
	uint8_t frame_trigger;
	struct {
	    uint8_t x, y;
	    uint8_t mask_x, mask_y;
	    uint8_t addr_h, addr_l;
	} sprites[NUM_SPRITES];
    };
} vram;

static __idata union {
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
	 * Lightly cooked accel data. I'm assuming we are doing only
	 * basic statistics on the ADC data before handing it off to
	 * the master- just the bare minimum to allow it to get useful
	 * information even if it's running our comms at a lower
	 * packet rate. So, we'll keep a running count of the number
	 * of samples we've taken since the last packet, and we'll
	 * report the minimum, maximum, and total readings. This will
	 * let the master notice shaking or sharp motion, as well as
	 * to get a more accurate filtered signal for each axis.
	 *
	 * This all comes to 10 bytes.
	 */
	uint8_t accel_min[2];
	uint8_t accel_max[2];
	uint8_t accel_count[2];
	uint16_t accel_total[2];

	/*
	 * For synchronizing LCD refreshes, the master can keep track
	 * of how many repaints the cube has performed. Ideally these
	 * repaints would be in turn sync'ed with the LCDC's hardware
	 * refresh timer. If we're tight on space, we don't need a
	 * full byte for this. Even a one-bit toggle woudl work,
	 * though we might want two bits to allow deeper queues.
	 */
	uint8_t frame_count;

	/*
	 * Need ~5 bits per sensor (5 other cubes * 4 sides + 1 idle =
	 * 21 states) So, there are plenty of bits free in here to
	 * encode things like button state.
	 */
	uint8_t neighbor[4];
    };
} ack_data;


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

void rf_isr(void) __interrupt(VECTOR_RF)
{
    /*
     * Write the STATUS register, to clear the IRQ.
     * We also get a STATUS read for free here.
     */

    RF_CSN = 0;					// Begin SPI transaction
    SPIRDAT = RF_CMD_W_REGISTER |		// Command byte
	RF_REG_STATUS;
    SPIRDAT = 0x70;				// Prefill TX FIFO with STATUS write
    while (!(SPIRSTAT & SPI_RX_READY));		// Wait for Command/STATUS byte
    SPIRDAT;					// Dummy read of STATUS byte
    while (!(SPIRSTAT & SPI_RX_READY));		// Wait for payload byte
    SPIRDAT;					// Dummy read
    RF_CSN = 1;					// End SPI transaction

    /*
     * If we had reason to check for other interrupts, we could use
     * the value of STATUS we read above to do so. But in this case,
     * we only get IRQs for incoming packets, so we're guaranteed to
     * have a packet waiting for us in the FIFO right now. Read it.
     */

    RF_CSN = 0;					// Begin SPI transaction
    SPIRDAT = RF_CMD_R_RX_PAYLOAD;		// Command byte
    SPIRDAT = 0;				// First dummy write, keep the TX FIFO full
    while (!(SPIRSTAT & SPI_RX_READY));		// Wait for Command/STATUS byte
    SPIRDAT;					// Dummy read of STATUS byte

    SPIRDAT = 0;				// Write next dummy byte
    while (!(SPIRSTAT & SPI_RX_READY));		// Wait for payload byte

    {
	register uint8_t i;
	register uint8_t __xdata *wptr;
	
	i = SPIRDAT;				// Read payload byte (Block address)
	wptr = vram.bytes; 			// Calculate offset to 31-byte block
	wptr += i << 5;
	wptr -= i;

	i = RF_PAYLOAD_MAX - 2;
	do {
	    SPIRDAT = 0;			// Write next dummy byte
	    while (!(SPIRSTAT & SPI_RX_READY));	// Wait for payload byte
	    *(wptr++) = SPIRDAT;		// Read payload byte
	} while (--i);

	while (!(SPIRSTAT & SPI_RX_READY));	// Wait for last payload byte
	*wptr = SPIRDAT;			// Read last payload byte
	RF_CSN = 1;				// End SPI transaction
    }

    /*
     * Create a new reply packet, refill the ACK queue.
     */

    RF_CSN = 0;					// Begin SPI transaction
    SPIRDAT = RF_CMD_W_ACK_PAYLD;		// Command byte

    {
	register uint8_t i = sizeof ack_data;
	register uint8_t __idata *rptr = &ack_data.bytes[0];

	do {
	    SPIRDAT = *(rptr++);                // Queue the next TX byte
	    while (!(SPIRSTAT & SPI_RX_READY));	// Wait for dummy response byte
	    SPIRDAT;		                // Read dummy byte
	} while (--i);
    }

    while (!(SPIRSTAT & SPI_RX_READY));	        // Wait for the last dummy response byte
    SPIRDAT;		                        // Read dummy byte
    RF_CSN = 1;				        // End SPI transaction

    /*
     * Reset the one-shot portions of our telemetry buffer
     */

    ack_data.accel_min[0] = 0xFF;
    ack_data.accel_min[1] = 0xFF;
    ack_data.accel_max[0] = 0x00;
    ack_data.accel_max[1] = 0x00;
    ack_data.accel_count[0] = 0;
    ack_data.accel_count[1] = 0;
    ack_data.accel_total[0] = 0;
    ack_data.accel_total[1] = 0;
}

void adc_isr(void) __interrupt(VECTOR_MISC)
{
    // Sample the hardware state
    uint16_t sample = (ADCDATH << 8) | ADCDATL;
    uint8_t con1 = ADCCON1;
    uint8_t sample_h = sample >> 4;

    // We're only using the first two channels
    const uint8_t channel_mask = 0x04;
    uint8_t channel = !!(con1 & channel_mask);

    /*
     * If we aren't yet in danger of overflowing the accumulator, add
     * this sample to it. (We're accumulating 12-bit samples in a 16-bit buffer)
     */
    if (ack_data.accel_count[channel] != 0xF) {
	ack_data.accel_total[channel] += sample;
	ack_data.accel_count[channel]++;
    }

    /*
     * Update the 8-bit min/max metrics
     */
    if (sample_h < ack_data.accel_min[channel])  ack_data.accel_min[channel] = sample_h;
    if (sample_h > ack_data.accel_max[channel])  ack_data.accel_max[channel] = sample_h;

    // Next channel
    ADCCON1 = con1 ^ channel_mask;
}

/*
 * This is a rather big master rendering function that can handle
 * panning and wrapping tile graphics, and up to two sprite overlays
 * of up to 64x64 pixels each.
 *
 * Sprites are specified in a quite low-level way, as a combination of
 * X/Y accumulators and masks. We increment the accumulators at each
 * row/column, and when the accumulator & the mask is zero, the sprite
 * is drawn. Sprite size may be chosen by changing the number of bits
 * in the mask, but all sprites have a row stride of 64 pixels.
 * Additionally, the low part of the sprite address is incremented
 * before every active scanline for that sprite. This must be
 * compensated for when setting up sprite parameters.
 *
 * XXX: The fixed row stride may be a problem if we have no good way to make use
 *      of non-64-pixel-aligned data in flash.
 *
 * XXX: There is a HUGE optimization opportunity here with regard to map addressing.
 *      The dptr manipulation code that SDCC generates here is very bad... and it's
 *      possible we might want to use some of the nRF24LE1's specific features, like
 *      the PDATA addressing mode.
 *
 * XXX: I rather unscrupulously hacked this to add full horizontal and vertical
 *      wrap-around support. There's also a lot of room for optimization here.
 *
 * XXX: Combining the tile and sprite code here was yet another giant hack. I'm sure
 *      there's a lot more to tighten up here...
 */
static void lcd_render(void)
{
    uint8_t pan_x, pan_y;		// Pixel panning
    uint8_t sah0, sah1;			// Sprite address high-byte
    uint8_t sal0, sal1;			// Sprite address low-byte
    register uint8_t smx0, smx1;	// Horizontal sprite masks
    uint8_t smy0, smy1;			// Vertical sprite masks
    uint8_t sx0, sx1;			// Per-row sprite X accumulator
    register uint8_t sxr0, sxr1;	// Register (per-pixel) sprite X accumulator
    register uint8_t spa0, spa1;	// Sprite pixel address (sxr << 2)
    uint8_t sy0, sy1;		       	// Sprite Y accumulator

    uint8_t y = LCD_HEIGHT;

    uint8_t tile_pan_y, tile_pan_bytes;
    uint8_t line_addr, first_column_addr;
    uint8_t last_tile_width, first_tile_width;
    __xdata uint8_t *map_line;

    /*
     * Inside a critical section, copy the panning registers and all
     * sprite parameters in from VRAM. We don't want these to be split
     * between old and new values if a radio packet comes in while
     * we're rendering.
     */

    IEN_EN = 0;

    pan_x = vram.pan_x;
    pan_y = vram.pan_y;

    sx0 = vram.sprites[0].x;
    sy0 = vram.sprites[0].y;
    smx0 = vram.sprites[0].mask_x;
    smy0 = vram.sprites[0].mask_y;
    sal0 = vram.sprites[0].addr_l;
    sah0 = vram.sprites[0].addr_h;

    sx1 = vram.sprites[1].x;
    sy1 = vram.sprites[1].y;
    smx1 = vram.sprites[1].mask_x;
    smy1 = vram.sprites[1].mask_y;
    sal1 = vram.sprites[1].addr_l;
    sah1 = vram.sprites[1].addr_h;

    IEN_EN = 1;
    
    /*
     * Panning calculations
     */

    line_addr = pan_y << 5;
    first_column_addr = (pan_x << 2) & 0x1C;
    last_tile_width = pan_x & 7;
    first_tile_width = 8 - last_tile_width;
    tile_pan_y = pan_y >> 3;
    tile_pan_bytes = (pan_x >> 2) & 0xFE;
    map_line = &vram.tilemap[(tile_pan_y << 5) + (pan_y & 0xF8)];

    /*
     * Rendering Macros
     */

#define MAP_LATCH_TILE()				\
    ADDR_PORT = map_line[map_x];			\
    CTRL_PORT = CTRL_FLASH_OUT | CTRL_FLASH_LAT1;	\
    ADDR_PORT = map_line[map_x+1];			\
    CTRL_PORT = CTRL_FLASH_OUT | CTRL_FLASH_LAT2;	\

#define MAP_NEXT_TILE()					\
    map_x += 2;						\
    if (map_x == 40)					\
	map_x = 0;					\

#define PIXEL_BURST(_count) {				\
	uint8_t _i = (_count);				\
	do {						\
	    ADDR_INC4();				\
	} while (--_i);					\
    }							\

#define SPRITE_ROW(i)					\
    sy##i++;						\
    if (sy##i & smy##i)					\
	sxr##i = 0x80;					\
    else {						\
	sal##i += 2;					\
	sxr##i = sx##i;					\
	spa##i = (sxr##i - 1) << 2;			\
	spr_active = 1;					\
    }							\

#define SPRITE_PIXEL(i, _ad)				\
    ADDR_PORT = sal##i;					\
    CTRL_PORT = CTRL_FLASH_OUT | CTRL_FLASH_LAT1;	\
    ADDR_PORT = sah##i;					\
    CTRL_PORT = CTRL_FLASH_OUT | CTRL_FLASH_LAT2;	\
    ADDR_PORT = spa##i += 4;				\
    if (BUS_PORT != CHROMA_KEY) {			\
	ADDR_INC4();					\
    } else {						\
	MAP_LATCH_TILE();				\
	ADDR_PORT = line_addr + _ad;			\
	ADDR_INC4();					\
    }							\
    sxr##i++;						\

    /*
     * Rendering Loop
     */

    do {
	uint8_t map_x = tile_pan_bytes;
	uint8_t spr_active = 0;

	SPRITE_ROW(0);
	SPRITE_ROW(1);

	if (spr_active) {
	    /*
	     * This row has sprites! Use a more general rendering loop.
	     */

	    uint8_t x = 15;

	    // XXX: First partial tile
	    MAP_LATCH_TILE();
	    MAP_NEXT_TILE();
	    ADDR_PORT = line_addr + first_column_addr;
	    PIXEL_BURST(first_tile_width);

	    sxr0 += first_tile_width;
	    sxr1 += first_tile_width;
	    spa0 += first_tile_width << 2;
	    spa1 += first_tile_width << 2;

	    // There are always 15 full tiles on-screen
	    do {
		if ((sxr0 & smx0) && ((sxr0 + 7) & smx0)) {
		    // All 8 pixels are non-sprite

		    MAP_LATCH_TILE();
		    MAP_NEXT_TILE();
		    ADDR_PORT = line_addr;
		    ADDR_INC32();

		    sxr0 += 8;
		    sxr1 += 8;
		    spa0 += 32;
		    spa1 += 32;

		} else {
		    // A mixture of sprite and tile pixels

		    SPRITE_PIXEL(0, 0x0);
		    SPRITE_PIXEL(0, 0x4);
		    SPRITE_PIXEL(0, 0x8);
		    SPRITE_PIXEL(0, 0xc);
		    SPRITE_PIXEL(0, 0x10);
		    SPRITE_PIXEL(0, 0x14);
		    SPRITE_PIXEL(0, 0x18);
		    SPRITE_PIXEL(0, 0x1c);

		    MAP_NEXT_TILE();
		}


	    } while (--x);

	    // XXX: Last partial tile
	    if (last_tile_width) {
		MAP_LATCH_TILE();
		MAP_NEXT_TILE();
		ADDR_PORT = line_addr;
		PIXEL_BURST(last_tile_width);
	    }

	} else {
	    /*
	     * No sprites. Use a scanline renderer optimized for tiles only.
	     */

	    uint8_t x = 15;

	    // First tile on the line, (1 <= width <= 8)
	    MAP_LATCH_TILE();
	    MAP_NEXT_TILE();
	    ADDR_PORT = line_addr + first_column_addr;
	    PIXEL_BURST(first_tile_width);

	    // There are always 15 full tiles on-screen
	    do {
		MAP_LATCH_TILE();
		MAP_NEXT_TILE();
		ADDR_PORT = line_addr;
		ADDR_INC32();
	    } while (--x);

	    // Might be one more partial tile, (0 <= width <= 7)
	    if (last_tile_width) {
		MAP_LATCH_TILE();
		MAP_NEXT_TILE();
		ADDR_PORT = line_addr;
		PIXEL_BURST(last_tile_width);
	    }
	}

	// Fixup the line address for our next scanline
	line_addr += 32;
	if (!line_addr) {
	    map_line += 40;
	    if (map_line > &vram.tilemap[799])
		map_line -= 800;
	}

    } while (--y);
}

void lcd_cmd_byte(uint8_t b)
{
    CTRL_PORT = CTRL_LCD_CMD;
    BUS_DIR = 0;
    BUS_PORT = b;
    ADDR_INC2();
    BUS_DIR = 0xFF;
    CTRL_PORT = CTRL_IDLE;
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

    // Set up continuous 12-bit, 4 ksps A/D conversion with interrupt
    // (Max precision, and enough speed to keep our telemetry buffers fed.)
    ADCCON3 = 0xE0;
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
