/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * Thundercracker cube firmware
 *
 * M. Elizabeth Scott <beth@sifteo.com>
 * Copyright <c> 2011 Sifteo, Inc. All rights reserved.
 */

#include <string.h>
#include "hardware.h"
#include "lcd.h"
#include "flash.h"
#include "radio.h"

// Temporary bank used by some graphics modes
#define GFX_BANK  2


/***********************************************************************
 * Common Macros
 **********************************************************************/

#pragma sdcc_hash +
#define MODE_RETURN() {						\
		__asm	inc	(_ack_data + RF_ACK_FRAME)	__endasm ; \
		__asm	orl	_ack_len, #RF_ACK_LEN_FRAME	__endasm ; \
		return;						\
	}

// Output a nonzero number of of pixels, not known at compile-time
#define PIXEL_BURST(_count) {				\
	register uint8_t _i = (_count);			\
	do {						\
	    ADDR_INC4();				\
	} while (--_i);					\
    }

// Output one pixel with static colors from two registers
#define PIXEL_FROM_REGS(l, h)					__endasm; \
    __asm mov	BUS_PORT, l					__endasm; \
    __asm inc	ADDR_PORT					__endasm; \
    __asm inc	ADDR_PORT					__endasm; \
    __asm mov	BUS_PORT, h					__endasm; \
    __asm inc	ADDR_PORT					__endasm; \
    __asm inc	ADDR_PORT					__endasm; \
    __asm

// Repeat the same register value for both color bytes
#define PIXEL_FROM_REG(l)					__endasm; \
    __asm mov	BUS_PORT, l					__endasm; \
    __asm inc	ADDR_PORT					__endasm; \
    __asm inc	ADDR_PORT					__endasm; \
    __asm inc	ADDR_PORT					__endasm; \
    __asm inc	ADDR_PORT					__endasm; \
    __asm

// Load a 16-bit tile address from DPTR without incrementing
#pragma sdcc_hash +
#define ADDR_FROM_DPTR0() {					\
    __asm movx	a, @dptr					__endasm; \
    __asm mov	ADDR_PORT, a					__endasm; \
    __asm inc	dptr						__endasm; \
    __asm mov	CTRL_PORT, #CTRL_FLASH_OUT | CTRL_FLASH_LAT1	__endasm; \
    __asm movx	a, @dptr					__endasm; \
    __asm mov	ADDR_PORT, a					__endasm; \
    __asm dec	dpl						__endasm; \
    __asm mov	CTRL_PORT, #CTRL_FLASH_OUT | CTRL_FLASH_LAT2	__endasm; \
    }

// Load a 16-bit tile address from DPTR1 without incrementing
#pragma sdcc_hash +
#define ADDR_FROM_DPTR1() {					\
    __asm movx	a, @dptr					__endasm; \
    __asm mov	ADDR_PORT, a					__endasm; \
    __asm inc	dptr						__endasm; \
    __asm mov	CTRL_PORT, #CTRL_FLASH_OUT | CTRL_FLASH_LAT1	__endasm; \
    __asm movx	a, @dptr					__endasm; \
    __asm mov	ADDR_PORT, a					__endasm; \
    __asm dec	_DPL1						__endasm; \
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

// Assembly macro wrappers
#define ASM_ADDR_INC4()			__endasm; ADDR_INC4(); __asm
#define ASM_ADDR_INC32()		__endasm; ADDR_INC32(); __asm
#define ASM_DPTR_INC2()			__endasm; DPTR_INC2(); __asm
#define ASM_ADDR_FROM_DPTR0()		__endasm; ADDR_FROM_DPTR0(); __asm
#define ASM_ADDR_FROM_DPTR1()		__endasm; ADDR_FROM_DPTR1(); __asm
#define ASM_ADDR_FROM_DPTR_INC()	__endasm; ADDR_FROM_DPTR_INC(); __asm


/***********************************************************************
 * _SYS_VM_POWERDOWN
 **********************************************************************/

static void vm_powerdown(void)
{
    lcd_sleep();
    MODE_RETURN();
}


/***********************************************************************
 * _SYS_VM_SOLID
 **********************************************************************/

/*
 * Copy vram.color to the LCD bus, and repeat for every pixel.
 */

static void vm_solid(void)
{
    lcd_begin_frame();
    LCD_WRITE_BEGIN();

    __asm
	mov	dptr, #_SYS_VA_NUM_LINES
	movx	a, @dptr
	mov	r1, a

	mov	dptr, #_SYS_VA_COLORMAP
	movx	a, @dptr
	mov	r0, a
	inc	dptr
	movx	a, @dptr

1$:	mov	r2, #LCD_WIDTH
2$:

        PIXEL_FROM_REGS(r0, a)

	djnz	r2, 2$
	djnz	r1, 1$
    __endasm ;

    LCD_WRITE_END();
    lcd_end_frame();
    MODE_RETURN();
}


/***********************************************************************
 * _SYS_VM_FB32
 **********************************************************************/

/*
 * 16-color 32x32 framebuffer mode.
 *
 * 16-entry colormap, 16 bytes per line.
 *
 * To support fast color lookups without copying the whole LUT into
 * internal memory, we use both DPTRs here.
 */

void vm_fb32_line(uint16_t src)
{
    src = src;
    __asm
	mov	_DPH1, #(_SYS_VA_COLORMAP >> 8)
	
	mov	r4, #16		; Loop over 16 horizontal bytes per line
4$:

	movx	a, @dptr
	inc	dptr
	mov	r5, a
	mov	_DPS, #1

	; Low nybble

	anl	a, #0xF
	rl	a
	mov	_DPL1, a
	movx	a, @dptr
	mov	r0, a
	inc	dptr
	movx	a, @dptr

    	PIXEL_FROM_REGS(r0, a)
	PIXEL_FROM_REGS(r0, a)
	PIXEL_FROM_REGS(r0, a)
	PIXEL_FROM_REGS(r0, a)

	; High nybble

	mov	a, r5
	anl	a, #0xF0
	swap	a
	rl	a
	mov	_DPL1, a
	movx	a, @dptr
	mov	r0, a
	inc	dptr
	movx	a, @dptr

    	PIXEL_FROM_REGS(r0, a)
	PIXEL_FROM_REGS(r0, a)
	PIXEL_FROM_REGS(r0, a)
	PIXEL_FROM_REGS(r0, a)

	mov	_DPS, #0
	djnz	r4, 4$		; Next byte

    __endasm ;

}

static void vm_fb32(void)
{
    uint8_t y = vram.num_lines >> 2;
    uint16_t src = 0;

    lcd_begin_frame();

    do {
	LCD_WRITE_BEGIN();
	vm_fb32_line(src);
	vm_fb32_line(src);
	vm_fb32_line(src);
	vm_fb32_line(src);
	src += 16;
	LCD_WRITE_END();

	flash_handle_fifo();
    } while (--y);    

    lcd_end_frame();
    MODE_RETURN();
}


/***********************************************************************
 * _SYS_VM_FB64
 **********************************************************************/

/*
 * 2-color 64x64 framebuffer mode.
 *
 * Two-entry colormap, 8 bytes per line.
 */

void vm_fb64_line(uint16_t ptr)
{
    ptr = ptr;
    __asm

	; Copy colormap[0] and colormap[1] to r4-7

	mov	_DPS, #1
	mov	dptr, #_SYS_VA_COLORMAP
	movx	a, @dptr
	mov	r4, a
	inc	dptr
	movx	a, @dptr
	mov	r5, a
	inc	dptr
	movx	a, @dptr
	mov	r6, a
	inc	dptr
	movx	a, @dptr
	mov	r7, a
	mov	_DPS, #0

	mov	r2, #8		; Loop over 8 horizontal bytes per line
4$:	movx	a, @dptr
	inc	dptr
	mov	r3, #8		; Loop over 8 pixels per byte
5$:	rrc	a		; Shift out one pixel
	jc	6$

        PIXEL_FROM_REGS(r4, r5)	; colormap[0]
	PIXEL_FROM_REGS(r4, r5)
	djnz	r3, 5$		; Next pixel
	djnz	r2, 4$		; Next byte
	sjmp	7$

6$:	PIXEL_FROM_REGS(r6, r7)	; colormap[1]
	PIXEL_FROM_REGS(r6, r7)
	djnz	r3, 5$		; Next pixel
	djnz	r2, 4$		; Next byte

7$:

    __endasm ;
}


static void vm_fb64(void)
{
    uint8_t y = vram.num_lines >> 1;
    uint16_t src = 0;

    lcd_begin_frame();

    do {
        LCD_WRITE_BEGIN();
	vm_fb64_line(src);
	vm_fb64_line(src);
	src += 8;
        LCD_WRITE_END();

	flash_handle_fifo();
    } while (--y);    

    lcd_end_frame();
    MODE_RETURN();
}


/***********************************************************************
 * _SYS_VM_FB128
 **********************************************************************/

/*
 * 2-color 128x48 framebuffer mode.
 *
 * Yes, this is too small to fit the screen :)
 *
 * If num_lines > 48, this buffer will repeat itself. That effect may
 * be useful perhaps, but this is actually intended to be used for
 * split-screen or letterboxed effects, particularly variable-spacing
 * text.
 *
 * Two-entry colormap, 16 bytes per line.
 */

void vm_fb128_line(uint16_t ptr)
{
    ptr = ptr;
    __asm

	; Copy colormap[0] and colormap[1] to r4-7

	mov	_DPS, #1
	mov	dptr, #_SYS_VA_COLORMAP
	movx	a, @dptr
	mov	r4, a
	inc	dptr
	movx	a, @dptr
	mov	r5, a
	inc	dptr
	movx	a, @dptr
	mov	r6, a
	inc	dptr
	movx	a, @dptr
	mov	r7, a
	mov	_DPS, #0

	mov	r2, #16		; Loop over 16 horizontal bytes per line
4$:	movx	a, @dptr
	inc	dptr
	mov	r3, #8		; Loop over 8 pixels per byte
5$:	rrc	a		; Shift out one pixel
	jc	6$

        PIXEL_FROM_REGS(r4, r5)	; colormap[0]
	djnz	r3, 5$		; Next pixel
	djnz	r2, 4$		; Next byte
	sjmp	7$

6$:	PIXEL_FROM_REGS(r6, r7)	; colormap[1]
	djnz	r3, 5$		; Next pixel
	djnz	r2, 4$		; Next byte

7$:

    __endasm ;
}


static void vm_fb128(void)
{
    uint8_t y = vram.num_lines;
    uint16_t src = 0;

    lcd_begin_frame();

    do {
        LCD_WRITE_BEGIN();
	vm_fb128_line(src);
        LCD_WRITE_END();

	src += 16;
	if ((src >> 8) == (_SYS_VA_COLORMAP >> 8))
	   src = 0;

	flash_handle_fifo();
    } while (--y);    

    lcd_end_frame();
    MODE_RETURN();
}


/***********************************************************************
 * _SYS_VM_BG0
 **********************************************************************/

static uint8_t x_bg0_first_w;		// Width of first displayed background tile, [1, 8]
static uint8_t x_bg0_last_w;		// Width of first displayed background tile, [0, 7]
static uint8_t x_bg0_first_addr;	// Low address offset for first displayed tile
static uint8_t x_bg0_wrap;		// Load value for a dec counter to the next X map wraparound

static uint8_t y_bg0_addr_l;		// Low part of tile addresses, inc by 32 each line
static uint16_t y_bg0_map;		// Map address for the first tile on this line

// Called once per tile, to check for horizontal map wrapping
#define BG0_WRAP_CHECK() {			  	\
	if (!--bg0_wrap)				\
	    DPTR -= RF_VRAM_STRIDE *2;			\
    }

static void vm_bg0_line(void)
{
    /*
     * Scanline renderer, draws a single tiled background layer.
     */

    uint8_t x = 15;
    uint8_t bg0_wrap = x_bg0_wrap;

    DPTR = y_bg0_map;

    // First partial or full tile
    ADDR_FROM_DPTR_INC();
    BG0_WRAP_CHECK();
    ADDR_PORT = y_bg0_addr_l + x_bg0_first_addr;
    PIXEL_BURST(x_bg0_first_w);

    // There are always 15 full tiles on-screen
    do {
	ADDR_FROM_DPTR_INC();
	BG0_WRAP_CHECK();
	ADDR_PORT = y_bg0_addr_l;
	ADDR_INC32();
    } while (--x);

    // Might be one more partial tile
    if (x_bg0_last_w) {
	ADDR_FROM_DPTR_INC();
	BG0_WRAP_CHECK();
	ADDR_PORT = y_bg0_addr_l;
	PIXEL_BURST(x_bg0_last_w);
    }

    // Release the bus
    CTRL_PORT = CTRL_IDLE;
}

static void vm_bg0_setup(void)
{
    /*
     * Once-per-frame setup for BG0
     */
     
    uint8_t pan_x, pan_y;
    uint8_t tile_pan_x, tile_pan_y;

    cli();
    pan_x = vram.bg0_x;
    pan_y = vram.bg0_y;
    sti();

    tile_pan_x = pan_x >> 3;
    tile_pan_y = pan_y >> 3;

    y_bg0_addr_l = pan_y << 5;
    y_bg0_map = tile_pan_y << 2;		// Y tile * 2
    y_bg0_map += tile_pan_y << 5;		// Y tile * 16
    y_bg0_map += tile_pan_x << 1;		// X tile * 2;

    x_bg0_last_w = pan_x & 7;
    x_bg0_first_w = 8 - x_bg0_last_w;
    x_bg0_first_addr = (pan_x << 2) & 0x1C;
    x_bg0_wrap = _SYS_VRAM_BG0_WIDTH - tile_pan_x;
}

static void vm_bg0_next(void)
{
    /*
     * Advance BG0 state to the next line
     */

    y_bg0_addr_l += 32;
    if (!y_bg0_addr_l) {
	// Next tile, with vertical wrap
	y_bg0_map += _SYS_VRAM_BG0_WIDTH * 2;
	if (y_bg0_map >= _SYS_VRAM_BG0_WIDTH * _SYS_VRAM_BG0_WIDTH * 2)
	    y_bg0_map -= _SYS_VRAM_BG0_WIDTH * _SYS_VRAM_BG0_WIDTH * 2;
    }
}

static void vm_bg0(void)
{
    uint8_t y = vram.num_lines;

    lcd_begin_frame();
    vm_bg0_setup();

    do {
	vm_bg0_line();
	vm_bg0_next();
	flash_handle_fifo();
    } while (--y);    

    lcd_end_frame();
    MODE_RETURN();
}


/***********************************************************************
 * _SYS_VM_BG0_ROM
 **********************************************************************/

/*
 * This mode has a VRAM layout identical to _SYS_VM_BG0, but the tile
 * source data is coming from ROM rather than from Flash.  The tile
 * indices are actually packed words containing a ROM address, a tile
 * mode (1-bit or 2-bit), and a 4-bit palette selector.
 *
 * This mode is available for use by games, as just another video mode.
 * But we also use it internally, when no master is connected. This mode
 * is used by the local drawing module, draw.c
 *
 * This implementation is fairly highly optimized, in part because it takes
 * a lot of effort just to get up to a 30 FPS single-layer display when
 * we have to clock out pixel data in software rather than bursting it from
 * flash. Performance in this mode is actually pretty important, since the
 * less time we spend drawing during a loading or idle screen, the more time
 * we have to be writing to flash or going into low-power sleep mode.
 *
 * See tilerom/README for more info.
 *
 * Swizzling map:
 *
 *   Map:      76543210 fedcba98
 *   DPL:      7654321i             <- one bit of line-index
 *   DPH:                   i21i    <- two bits of line-index
 *   Palette:           7654....	<- one replicated palette-select nybble
 *   Mode:                  0       <- selects 1 or 2 planes
 *
 * Registers, main bank:
 *
 *   r0: Scratch
 *   r1: X wrap counter
 *   r2: Plane 1 shift register
 *   r3: Palette base
 *   r4: Pixel loop
 *   r5: Tile loop / Pixel skip count
 *   r6: Byte/line DPL
 *   r7: Byte/line DPH
 *
 * GFX_BANK is used to hold a local copy of the current palette.
 * It must be reloaded any time r3 is changed:
 *
 *   r0: Color 0, LSB/MSB
 *   r1: Plane 0 shift register
 *   r2: Color 1, LSB
 *   r3: Color 1, MSB
 *   r4: Color 2, LSB
 *   r5: Color 2, MSB
 *   r6: Color 3, LSB
 *   r7: Color 3, MSB
 */

// Data in tilerom.c
extern __code uint8_t rom_palettes[];
extern __code uint8_t rom_tiles[];

static void vm_bg0_rom_next_tile(void) __naked __using(GFX_BANK)
{
    /*
     * Load the next single tile from BG0. This sets up
     * r3, r6, and r7, and it loads the raw high-byte of the index
     * into r0.
     *
     * Additionally, if the palette has changed, we load a new
     * ROM palette. We return with the correct palette in GFX_BANK.
     *
     * Returns with DPTR1 active. Register bank is undefined.
     * (May be GFX_BANK or default bank)
     */

    __asm

	movx	a, @dptr		; Tile map, low byte
	inc	dptr
	anl     6, #1			; Combine with LSB from r6
	orl	6, a

	movx	a, @dptr		; Tile map, high byte
	inc	dptr
	mov	r0, a			; Save raw copy
	anl	a, #0x06
	anl	7, #0xF9		; Combine with line-index and base address
	orl	7, a

	djnz	r1, 2$			; X wrap check
	mov	a, dpl
	add	a, #(-RF_VRAM_STRIDE*2)
	mov	dpl, a
	mov	a, dph
	addc	a, #((-RF_VRAM_STRIDE*2) >> 8)
	mov	dph, a
2$:

	mov	_DPS, #1		; Switch to DPTR1. (DPTR is used only for the tile map)

	; Load the palette, only if it has changed since the last tile.
	; We bank on having relatively few palette changes, so that we
	; can amortize the relatively high cost of reading palette data
	; from code memory. It is very slow to do this on each pixel,
	; even just for the single palette index we need.

	mov	a, r0
	anl	a, #0xf0		; Mask off four palette-select bits
	xrl	a, r3			;    Compare with r3
	jz	1$			;    Palette has not changed
	xrl	a, r3			;    Make this the new current palette
	mov	r3, a

	mov	psw, #(GFX_BANK << 3)
	mov	dptr, #_rom_palettes

	jmp	@a+dptr			; Tail call to generated code, loads r0-r7.
	
1$:	ret				; No palette change

    __endasm ; 
}

static void vm_bg0_rom_tiles_fast(void) __naked __using(GFX_BANK)
{
    /*
     * Fast burst of whole tiles, with tile count in r5.
     *
     * This handles the bulk of the frame rendering (everything but
     * initial and final partial-tiles), so it's where the heavy duty
     * optimizations are made.
     */

    __asm

	; Tile loop
2$:

	lcall	_vm_bg0_rom_next_tile
	mov	psw, #0
	mov	_DPL1, r6
	mov	_DPH1, r7

	; Fetch Plane 0 byte. This is necessary for both the 2-color and 4-color paths.

	clr 	a			; Grab tile bitmap byte
	movc	a, @a+dptr
	mov	(GFX_BANK*8+1), a	;    Store in Plane 0 register

	; Bit unpacking loop

	mov	a, r0			; Mode bit:
	jnb	acc.3, 13$		;    Are we using 2-color mode?
	sjmp	3$			;    4-color mode
	
8$:
	mov	psw, #0			; Restore bank
	mov	_DPS, #0		; Must restore DPTR
	djnz	r5, 2$			; Next tile
	ret

	; ---- 4-color mode

3$:
	mov	a, #2			; Offset by one tile (Undefined across 128-tile boundaries)
	movc	a, @a+dptr		; Grab second bitmap byte
	mov	r2, a			;    Store in Plane 1 register
	mov	r4, #8			; Loop over 8 bytes
	mov	psw, #(GFX_BANK << 3)

4$:
	mov	a, ar2			; Shift out LSB on Plane 1
	rrc	a
	mov	ar2, a
	jc	9$

	mov	a, r1			; Plane 1 = 0, test Plane 0
	rrc	a
	mov	r1, a
	jc	10$
	PIXEL_FROM_REG(r0)
	djnz	ar4, 4$
	sjmp	8$
10$:	PIXEL_FROM_REGS(r2,r3)
	djnz	ar4, 4$
	sjmp	8$

9$:
	mov	a, r1			; Plane 1 = 1, test Plane 0
	rrc	a
	mov	r1, a
	jc	12$
	PIXEL_FROM_REGS(r4,r5)
	djnz	ar4, 4$
	sjmp	8$
12$:	PIXEL_FROM_REGS(r6,r7)
	djnz	ar4, 4$
	sjmp	8$

	; ---- 2-color loop (unrolled)

	; This is optimized for runs of index 0.  Since we require index 0
	; to have identical MSB and LSB, we can use this to avoid reloading
	; BUS_PORT at all, unless we hit a '1' bit.

13$:
	mov	psw, #(GFX_BANK << 3)
	mov	a, r1
	mov	BUS_PORT, r0		; Default to index 0
	jz	14$			; Blank-tile loop

	rrc	a			; Index 0 ladder
	jc	30$
	ASM_ADDR_INC4()
	rrc	a
	jc	31$
21$:	ASM_ADDR_INC4()
	rrc	a
	jc	32$
22$:	ASM_ADDR_INC4()
	rrc	a
	jc	33$
23$:	ASM_ADDR_INC4()
	rrc	a
	jc	34$
24$:	ASM_ADDR_INC4()
	rrc	a
	jc	35$
25$:	ASM_ADDR_INC4()
	rrc	a
	jc	36$
26$:	ASM_ADDR_INC4()
	rrc	a
	jc	37$
27$:	ASM_ADDR_INC4()
	ljmp	8$

14$:	ljmp	15$			; Longer jump to blank-tile loop

30$:	PIXEL_FROM_REGS(r2,r3)		; Index 1 ladder
	rrc	a
	jnc	41$
31$:	PIXEL_FROM_REGS(r2,r3)
	rrc	a
	jnc	42$
32$:	PIXEL_FROM_REGS(r2,r3)
	rrc	a
	jnc	43$
33$:	PIXEL_FROM_REGS(r2,r3)
	rrc	a
	jnc	44$
34$:	PIXEL_FROM_REGS(r2,r3)
	rrc	a
	jnc	45$
35$:	PIXEL_FROM_REGS(r2,r3)
	rrc	a
	jnc	46$
36$:	PIXEL_FROM_REGS(r2,r3)
	rrc	a
	jnc	47$
37$:	PIXEL_FROM_REGS(r2,r3)
	ljmp	8$

41$:	mov	BUS_PORT, r0		; Transition 1 -> 0 ladder
	ljmp	21$
42$:	mov	BUS_PORT, r0
	ljmp	22$
43$:	mov	BUS_PORT, r0
	ljmp	23$
44$:	mov	BUS_PORT, r0
	ljmp	24$
45$:	mov	BUS_PORT, r0
	ljmp	25$
46$:	mov	BUS_PORT, r0
	ljmp	26$
47$:	mov	BUS_PORT, r0
	ljmp	27$

15$:	ASM_ADDR_INC32()		; Blank byte, no comparisons
	ljmp	8$

    __endasm ;
}

static void vm_bg0_rom_tile_partial(void) __naked __using(GFX_BANK)
{
    /*
     * Output a single tile. Skips the first r5 bits, then outputs r4 bits.
     * r5 may be zero, r4 must not be.
     *
     * This is only used for at most two tiles per line.
     */

    __asm

	lcall	_vm_bg0_rom_next_tile
	mov	psw, #0
	mov	_DPL1, r6
	mov	_DPH1, r7

	clr 	a			; Grab tile bitmap byte
	movc	a, @a+dptr
	mov	(GFX_BANK*8+1), a	;    Store in Plane 0 register

	mov	a, #2			; Offset by one tile (Undefined across 128-tile boundaries)
	movc	a, @a+dptr		; Grab second bitmap byte
	mov	r2, a			;    Store in Plane 1 register

11$:	cjne	r5, #0, 10$		; Bit skip loop

	mov	a, r0			; Mode bit:
	mov	psw, #(GFX_BANK << 3)	;    Switch to GFX_BANK on our way out...
	jb	acc.3, 4$		;    Are we using 4-color mode?
	
	; ---- 2-color mode

7$:
	mov	a, r1			; Plane 1 = 0, test Plane 0
	rrc	a
	mov	r1, a
	jc	8$
	PIXEL_FROM_REG(r0)
	djnz	ar4, 7$
	sjmp	5$
8$:	PIXEL_FROM_REGS(r2,r3)
	djnz	ar4, 7$

	; ---- Cleanup

5$:
	mov	psw, #0			; Restore bank
	mov	_DPS, #0		; Must restore DPTR
	ret

	; ---- Bottom half of bit skip loop

10$:	dec    r5

	mov    a, (GFX_BANK*8+1)	; Plane 0
	rr     a
	mov    (GFX_BANK*8+1), a

	mov    a, r2			; Plane 1
	rr     a
	mov    r2, a

	sjmp   11$

	; ---- 4-color mode

4$:
	mov	a, ar2			; Shift out LSB on Plane 1
	rrc	a
	mov	ar2, a
	jc	2$

	mov	a, r1			; Plane 1 = 0, test Plane 0
	rrc	a
	mov	r1, a
	jc	3$
	PIXEL_FROM_REG(r0)
	djnz	ar4, 4$
	sjmp	5$
3$:	PIXEL_FROM_REGS(r2,r3)
	djnz	ar4, 4$
	sjmp	5$

2$:
	mov	a, r1			; Plane 1 = 1, test Plane 0
	rrc	a
	mov	r1, a
	jc	6$
	PIXEL_FROM_REGS(r4,r5)
	djnz	ar4, 4$
	sjmp	5$
6$:	PIXEL_FROM_REGS(r6,r7)
	djnz	ar4, 4$
	sjmp	5$

    __endasm ;
}

static void vm_bg0_rom_line(void)
{
    LCD_WRITE_BEGIN();

    /*
     * The main BG0 code keeps track of the start address in the map,
     * updating it for each line. During this function, we keep the source
     * address in DPTR at all times.
     */

    DPTR = y_bg0_map;

    /*
     * Reset the palette cache, by setting an invalid palette base address.
     * This will never match a tile's actual palette base, so we'll reload
     * on the first tile.
     */

    __asm
	mov	r3, #0xFF
    __endasm ;

    /*
     * We keep the wrap counter stored in a register, for fast x-wrap checks.
     */

    __asm
        mov	r1, _x_bg0_wrap
    __endasm ;

    /*
     * Set up per-line DPTR values. The three useful bits from
     * y_bg0_addr_l are kind of sprayed out all over the DPTR word, in
     * an effort to keep the actual tile address bits mapping 1:1
     * without any slow bit shifting.
     *
     * So, we have some awkward per-line setup here to do. Bits 7:5 in
     * y_bg0_addr_l need to map to bits 3 and 0 in r7, and bit 0 in r6
     * respectively.
     *
     *   y_bg0_addr_l:   765xxxxx
     *             r6:   xxxxxxx5
     *             r7:   xxxx7xx6
     *
     * All 'x' bits here are guaranteed to be zero.
     */

    __asm
	mov	a, _y_bg0_addr_l	; 765xxxxx
	swap	a			; xxxx765x
	rr	a			; xxxxx765
	mov	r6, a
	anl	ar6, #1
	clr	c
	rrc	a			; xxxxxx76 5
	clr	c
	rrc	a			; xxxxxxx7 6
	rl	a			; xxxxxx7x 6
	rl	a			; xxxxx7xx 6
	rlc	a			; xxxx7xx6 x
	orl	a, #(_rom_tiles >> 8)	; Add base address
	mov	r7, a
    __endasm ;

    /*
     * Segment the line into a fast full-tile burst, and up to two slower partial tiles.
     */

    __asm

	; First tile, may be skipping up to 7 pixels from the beginning

	mov	r4, _x_bg0_first_w
	mov	r5, _x_bg0_last_w
	lcall	_vm_bg0_rom_tile_partial

	; Always have a run of 15 full tiles

	mov	r5, #15
	lcall	_vm_bg0_rom_tiles_fast

	; May have a final partial tile

	mov	a, _x_bg0_last_w
	jz	1$
	inc	r1		; Negate the x-wrap check for this tile
	mov	r4, a		; Width computed earlier
	mov	r5, #0		; No skipped bits
	lcall	_vm_bg0_rom_tile_partial
1$:

    __endasm ;

    LCD_WRITE_END();
    CTRL_PORT = CTRL_IDLE;
}

static void vm_bg0_rom(void)
{
    uint8_t y = vram.num_lines;

    lcd_begin_frame();
    vm_bg0_setup();

    do {
	vm_bg0_rom_line();
	vm_bg0_next();
	flash_handle_fifo();
    } while (--y);    

    lcd_end_frame();
    MODE_RETURN();
}


/***********************************************************************
 * _SYS_VM_BG0_BG1
 **********************************************************************/

/*
 * Background BG0, plus a BG1 "overlay" which is a smaller-than-screen-sized
 * tile grid that is allocated configurably using a separate bit vector.
 *
 * BG1 has some important differences relative to BG0:
 * 
 *   - It is screen-sized (16x16) rather than 18x18. BG1 is not intended
 *     for 'endless' scrolling, like BG0 is.
 *
 *   - When panned, it does not wrap at the map edge. Pixels from 128
 *     to 255 are transparent in both axes, so the overlay can be smoothly
 *     scrolled into and out of frame.
 *
 *   - Tile addresses in BG1 cannot be computed using a simple multiply,
 *     they must be accumulated by counting '1' bits in the allocation
 *     bitmap.
 */

/*
 *   c: Current BG1 tile bit
 *  r0: Scratch
 *  r5: Tile count
 *  r6: BG1 tile bitmap (next)
 *  r7: BG1 tile bitmap (current)
 */

static uint8_t x_bg1_first_w;		// Width of first displayed background tile, [1, 8]
static uint8_t x_bg1_last_w;		// Width of first displayed background tile, [0, 7]
static uint8_t x_bg1_first_addr;	// Low address offset for first displayed tile
static uint8_t x_bg1_shift;		// Amount to shift bitmap by at the start of the line, plus one

static uint8_t y_bg1_addr_l;		// Low part of tile addresses, inc by 32 each line
static uint8_t y_bg1_word;		// Low part of bitmap address for this line
static uint16_t y_bg1_map;		// Map address for the first tile on this line

// Shift the next tile bit into C
#define BG1_NEXT_BIT()						__endasm; \
    __asm mov	a, r6						__endasm; \
    __asm clr	c						__endasm; \
    __asm rrc   a						__endasm; \
    __asm mov	r6, a						__endasm; \
    __asm mov	a, r7						__endasm; \
    __asm rrc	a						__endasm; \
    __asm mov	r7, a						__endasm; \
    __asm

// Load bitmap from globals. If 'a' is zero on exit, no bits are set.
#define BG1_LOAD_BITS()						__endasm; \
    __asm mov	_DPL, _y_bg1_word				__endasm; \
    __asm mov	_DPH, #(_SYS_VA_BG1_BITMAP >> 8)		__endasm; \
    __asm movx	a, @dptr					__endasm; \
    __asm mov	r7, a						__endasm; \
    __asm inc	dptr						__endasm; \
    __asm movx	a, @dptr					__endasm; \
    __asm mov	r6, a						__endasm; \
    __asm orl	a, r7						__endasm; \
    __asm

#define BG0_BG1_LOAD_MAPS()					__endasm; \
    __asm mov	_DPL, _y_bg0_map				__endasm; \
    __asm mov	_DPH, (_y_bg0_map+1)				__endasm; \
    __asm mov	_DPL1, _y_bg1_map				__endasm; \
    __asm mov	_DPH1, (_y_bg1_map+1)				__endasm; \
    __asm

#define CHROMA_PREP()						__endasm; \
    __asm mov	a, #_SYS_CHROMA_KEY				__endasm; \
    __asm

#define CHROMA_J_OPAQUE(lbl)					__endasm; \
    __asm cjne	a, BUS_PORT, lbl				__endasm; \
    __asm

// Next BG0 tile (while in BG0 state)
#define ASM_BG0_NEXT()						__endasm; \
    __asm inc	dptr						__endasm; \
    __asm inc	dptr						__endasm; \
    __asm ASM_ADDR_FROM_DPTR0()					__endasm; \
    __asm mov	ADDR_PORT, _y_bg0_addr_l			__endasm; \
    __asm

// State transition, BG0 pixel to BG1 pixel, at nonzero X offset
#define STATE_BG0_TO_BG1(x)					__endasm; \
    __asm inc	_DPS						__endasm; \
    __asm ASM_ADDR_FROM_DPTR1()					__endasm; \
    __asm mov	a, _y_bg1_addr_l				__endasm; \
    __asm add	a, #((x) * 4)					__endasm; \
    __asm mov	ADDR_PORT, a					__endasm; \
    __asm CHROMA_PREP()						__endasm; \
    __asm

// State transition, BG0 pixel to BG1 pixel, at zero X offset
#define STATE_BG0_TO_BG1_0()					__endasm; \
    __asm inc	_DPS						__endasm; \
    __asm ASM_ADDR_FROM_DPTR1()					__endasm; \
    __asm mov	ADDR_PORT, _y_bg1_addr_l			__endasm; \
    __asm CHROMA_PREP()						__endasm; \
    __asm

// State transition, BG1 pixel to BG0 pixel, at nonzero X offset
#define STATE_BG1_TO_BG0(x)					__endasm; \
    __asm dec	_DPS						__endasm; \
    __asm ASM_ADDR_FROM_DPTR0()					__endasm; \
    __asm mov	a, _y_bg0_addr_l				__endasm; \
    __asm add	a, #((x) * 4)					__endasm; \
    __asm mov	ADDR_PORT, a					__endasm; \
    __asm

// State transition, BG1 pixel to BG0 pixel, at zero X offset
#define STATE_BG1_TO_BG0_0()					__endasm; \
    __asm dec	_DPS						__endasm; \
    __asm ASM_ADDR_FROM_DPTR0()					__endasm; \
    __asm mov	ADDR_PORT, _y_bg0_addr_l			__endasm; \
    __asm


static void vm_bg0_bg1_tiles_fast_p0(void) __naked
{
    /*
     * Render 'r5' full tiles, with BG0 and BG1 visible.
     * Handles cases where the panning delta between BG0 and BG1 is zero.
     *
     * This is an unrolled state machine, with state ladders for
     * BG0 pixels and BG1 pixels. Our state vector consists of
     * layer (BG0/BG1) and X pixel. On entry, we must already be set
     * up for the first pixel of BG0.
     */

    __asm

	; BG0 ladder

10$:	jc	1$
	ASM_ADDR_INC4()				; BG0 Pixel 0
	ASM_ADDR_INC4()				; BG0 Pixel 1
	ASM_ADDR_INC4()				; BG0 Pixel 2
	ASM_ADDR_INC4()				; BG0 Pixel 3
	ASM_ADDR_INC4()				; BG0 Pixel 4
	ASM_ADDR_INC4()				; BG0 Pixel 5
	ASM_ADDR_INC4()				; BG0 Pixel 6
	ASM_ADDR_INC4()				; BG0 Pixel 7

11$:	ASM_BG0_NEXT()
	BG1_NEXT_BIT()
	djnz	r5, 10$
	ret

	; BG1 ladder

1$:	STATE_BG0_TO_BG1_0()			; Entry BG0->BG1

40$:	CHROMA_J_OPAQUE(30$)			; Key BG1 Pixel 0
	STATE_BG1_TO_BG0_0()			;   BG1 -> BG0
	ASM_ADDR_INC4()				;   Write BG0
	STATE_BG0_TO_BG1_0()			;   BG0 -> BG1
	sjmp	41$
30$:	ASM_ADDR_INC4()				;   Write BG1
41$:	CHROMA_J_OPAQUE(31$)			; Key BG1 Pixel 1
	STATE_BG1_TO_BG0(1)			;   BG1 -> BG0
	ASM_ADDR_INC4()				;   Write BG0
	STATE_BG0_TO_BG1(1)			;   BG0 -> BG1
	sjmp	42$
31$:	ASM_ADDR_INC4()				;   Write BG1
42$:	CHROMA_J_OPAQUE(32$)			; Key BG1 Pixel 2
	STATE_BG1_TO_BG0(2)			;   BG1 -> BG0
	ASM_ADDR_INC4()				;   Write BG0
	STATE_BG0_TO_BG1(2)			;   BG0 -> BG1
	sjmp	43$
32$:	ASM_ADDR_INC4()				;   Write BG1
43$:	CHROMA_J_OPAQUE(33$)			; Key BG1 Pixel 3
	STATE_BG1_TO_BG0(3)			;   BG1 -> BG0
	ASM_ADDR_INC4()				;   Write BG0
	STATE_BG0_TO_BG1(3)			;   BG0 -> BG1
	sjmp	44$
33$:	ASM_ADDR_INC4()				;   Write BG1
44$:	CHROMA_J_OPAQUE(34$)			; Key BG1 Pixel 4
	STATE_BG1_TO_BG0(4)			;   BG1 -> BG0
	ASM_ADDR_INC4()				;   Write BG0
	STATE_BG0_TO_BG1(4)			;   BG0 -> BG1
	sjmp	45$
34$:	ASM_ADDR_INC4()				;   Write BG1
45$:	CHROMA_J_OPAQUE(35$)			; Key BG1 Pixel 5
	STATE_BG1_TO_BG0(5)			;   BG1 -> BG0
	ASM_ADDR_INC4()				;   Write BG0
	STATE_BG0_TO_BG1(5)			;   BG0 -> BG1
	sjmp	46$
35$:	ASM_ADDR_INC4()				;   Write BG1
46$:	CHROMA_J_OPAQUE(36$)			; Key BG1 Pixel 6
	STATE_BG1_TO_BG0(6)			;   BG1 -> BG0
	ASM_ADDR_INC4()				;   Write BG0
	STATE_BG0_TO_BG1(6)			;   BG0 -> BG1
	sjmp	47$
36$:	ASM_ADDR_INC4()				;   Write BG1
47$:	CHROMA_J_OPAQUE(37$)			; Key BG1 Pixel 7
	STATE_BG1_TO_BG0(7)			;   BG1 -> BG0
	ASM_ADDR_INC4()				;   Write BG0
	STATE_BG0_TO_BG1(7)			;   BG0 -> BG1
	ASM_DPTR_INC2()				; Next BG1 Tile
	dec	_DPS				;   BG1 -> BG0
	ljmp	11$				;   Return to BG0 ladder
37$:	ASM_ADDR_INC4()				; Write BG1
	ASM_DPTR_INC2()				; Next BG1 Tile
	dec	_DPS				;   BG1 -> BG0
	ljmp	11$				;   Return to BG0 ladder

    __endasm ;
}

static void vm_bg0_bg1_line(void)
{
    /*
     * Scanline renderer: BG0 and BG1 visible.
     */

    __asm

	BG1_LOAD_BITS()
	jnz	1$
	ljmp	_vm_bg0_line		; No BG1 bits on this line
1$:					    

	BG0_BG1_LOAD_MAPS()
	ASM_ADDR_FROM_DPTR0()
	mov	ADDR_PORT, _y_bg0_addr_l
	BG1_NEXT_BIT()
	mov	r5, #16
	lcall	_vm_bg0_bg1_tiles_fast_p0

	mov	CTRL_PORT, #CTRL_IDLE

    __endasm ;
}

static void vm_bg1_setup(void)
{
    /*
     * Once-per-frame setup for BG1
     */
     
    uint8_t pan_x, pan_y;
    uint8_t tile_pan_x, tile_pan_y;

    cli();
    pan_x = vram.bg1_x;
    pan_y = vram.bg1_y;
    sti();

    tile_pan_x = pan_x >> 3;
    tile_pan_y = pan_y >> 3;

    y_bg1_addr_l = pan_y << 5;
    y_bg1_word = (tile_pan_y << 1) + (_SYS_VA_BG1_BITMAP & 0xFF);

    x_bg1_shift = tile_pan_x + 1;
    x_bg1_last_w = pan_x & 7;
    x_bg1_first_w = 8 - x_bg1_last_w;
    x_bg1_first_addr = (pan_x << 2) & 0x1C;

    /*
     * XXX: To find the initial value of y_bg1_map, we have to scan
     * for '1' bits in the bitmap.
     *
     * We need to set both y_bg1_map and DPTR1, since we don't know
     * whether the first line will end up loading DPTR1 or not. If
     * the first line has no BG1 tiles, we still need to have the
     * right pointer resident in DPTR1, since vm_bg1_next() will
     * still try to save it.
     */

    y_bg1_map = _SYS_VA_BG1_TILES;
    DPH1 = _SYS_VA_BG1_TILES >> 8;
    DPL1 = _SYS_VA_BG1_TILES & 0xFF;
}

static void vm_bg1_next(void)
{
    /*
     * Advance BG1 state to the next line
     */

    y_bg1_addr_l += 32;
    if (!y_bg1_addr_l) {
	y_bg1_word += 2;

	/*
	 * If we're advancing, save the DPTR1 advancement that we performed
	 * during this line. Otherwise, it gets discarded at the next line.
	 */
	__asm
	    mov	  _y_bg1_map, _DPL1
	    mov	  (_y_bg1_map+1), _DPH1
        __endasm ;
    }
}

void vm_bg0_bg1(void)
{
    uint8_t y = vram.num_lines;

    lcd_begin_frame();
    vm_bg0_setup();
    vm_bg1_setup();
    
    do {
	vm_bg0_bg1_line();

	vm_bg0_next();
	vm_bg1_next();
	flash_handle_fifo();
    } while (--y);    

    lcd_end_frame();
    MODE_RETURN();
}


/***********************************************************************
 * _SYS_VM_BG0_SPR_BG1
 **********************************************************************/

/*
 * Backgrounds BG0 and BG1, with linear sprites in-between.
 */

void vm_bg0_spr_bg1(void)
{
    MODE_RETURN();
}

/*
 * XXXX Junk that needs to be rewritten...
 */
#if 0

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
#endif


/***********************************************************************
 * Graphics mode dispatch
 **********************************************************************/

void graphics_render(void) __naked
{
    /*
     * Check the toggle bit (rendering trigger), in bit 1 of
     * vram.flags. If it matches the LSB of frame_count, we have
     * nothing to do.
     */

    __asm
	mov	dptr, #_SYS_VA_FLAGS
	movx	a, @dptr
	jb	acc.3, 1$			; Handle _SYS_VF_CONTINUOUS
	rr	a
	xrl	a, (_ack_data + RF_ACK_FRAME)	; Compare _SYS_VF_TOGGLE with frame_count LSB
	rrc	a
	jnc	3$				; Return if no toggle
1$:
    __endasm ;

    /*
     * Video mode jump table.
     *
     * This acts as a tail-call. The mode-specific function returns
     * on our behalf, after acknowledging the frame.
     */

    __asm
	mov	dptr, #_SYS_VA_MODE
	movx	a, @dptr
	anl	a, #_SYS_VM_MASK
	mov	dptr, #2$
	jmp	@a+dptr
2$:
	ljmp	_vm_powerdown	; 0x00
	nop
        ljmp	_vm_bg0_rom	; 0x04
	nop
	ljmp	_vm_solid	; 0x08
	nop
	ljmp	_vm_fb32	; 0x0c
	nop
	ljmp	_vm_fb64	; 0x10
	nop
	ljmp	_vm_fb128	; 0x14
	nop
	ljmp	_vm_bg0		; 0x18
	nop
	ljmp	_vm_bg0_bg1	; 0x1c
	nop
	ljmp	_vm_bg0_spr_bg1	; 0x20
	nop
	ljmp	_vm_powerdown	; 0x24 (unused)
	nop
	ljmp	_vm_powerdown	; 0x28 (unused)
	nop
	ljmp	_vm_powerdown	; 0x2c (unused)
	nop
	ljmp	_vm_powerdown	; 0x30 (unused)
	nop
	ljmp	_vm_powerdown	; 0x34 (unused)
	nop
	ljmp	_vm_powerdown	; 0x38 (unused)
	nop
	ljmp	_vm_powerdown	; 0x3c (unused)

3$:	ret

    __endasm ;
}
