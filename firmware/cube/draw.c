/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * Thundercracker cube firmware
 *
 * M. Elizabeth Scott <beth@sifteo.com>
 * Copyright <c> 2011 Sifteo, Inc. All rights reserved.
 */

#include <stdint.h>
#include "lcd.h"
#include "draw.h"

uint16_t draw_xy;
uint8_t draw_attr;

void draw_clear()
{
    // Clear all of VRAM (1 kB)

    __asm
	mov	dptr, #0
1$:	clr	a

	movx	@dptr, a
	inc	dptr
	movx	@dptr, a
	inc	dptr
	movx	@dptr, a
	inc	dptr
	movx	@dptr, a
	inc	dptr

	mov	a, #(1024 >> 8)
	cjne	a, dph, 1$
    __endasm ;

    vram.num_lines = 128;
    vram.mode = _SYS_VM_BG0_ROM;
    
    /*
     * We don't actually render continuously, when we're idle, but this flag
     * instructs graphics_render() to actually render a frame every time it's
     * called. This is what we want during idle mode, since we only call
     * graphics_render() when it's actually necessary.
     */
    vram.flags = _SYS_VF_CONTINUOUS;

    // Reset state
    draw_xy = XY(0, 0);
    draw_attr = ATTR_NONE;
}

void draw_image(const __code uint8_t *image)
{
    image = image;
    __asm
	mov	_DPL1, _draw_xy
	mov	_DPH1, (_draw_xy + 1)

	; Read header

	clr	a		; Width
	movc	a, @a+DPTR
	mov	r1, a
	inc	dptr

	clr	a		; Height
	movc	a, @a+DPTR
	mov	r2, a
	inc	dptr

	clr	a		; MSB / Mode
	movc	a, @a+DPTR
	mov	r3, a
	inc	dptr

	
1$:				; Loop over all lines
	mov	ar0, r1		; Loop over tiles in this line
2$:

	clr	a		; Read LSB
	movc	a, @a+DPTR
	inc	dptr
	clr	c		; Tile index to low 7 bits, with extra bit in C
	rlc	a

	mov	_DPS, #1	; Write low byte to destination
	movx	@dptr, a
	inc	dptr

	mov	a, r3		; Test mode: Is the source 8-bit or 16-bit?
	inc	a
	jz	3$		; Jump if 16-bit

	; 8-bit source, MSB comes from _draw_attr

	mov	a, r3		; Start with common MSB
	rlc	a		; Extra bit from C
	rl	a		; Index to 7:7
	xrl	a, _draw_attr	; XOR in draw_attr
	movx	@dptr, a	; Write to high byte
	inc	dptr
	mov	_DPS, #0	; Next source byte
	djnz	r0, 2$		; Next tile
	sjmp	4$

	; 16-bit source
3$:
	mov	_DPS, #0	; Get another source byte
	clr	a
	movc	a, @a+DPTR
	inc	dptr
	mov	_DPS, #1

	rlc	a		; Rotate in extra bit from C
	rl	a		; Index to 7:7
	xrl	a, _draw_attr	; XOR in the draw_attr
	movx	@dptr, a	; Write to high byte
	inc	dptr
	mov	_DPS, #0	; Next source byte

	djnz	r0, 2$		; Next tile
4$:

	; Next line, adjust destination address by (PITCH - width)

	clr    c
	mov    a, #_SYS_VRAM_BG0_WIDTH
	subb   a, r1
	rl     a

	add    a, _DPL1
	mov    _DPL1, a
	mov    a, _DPH1
	addc   a, #0
	mov    _DPH1, a

	djnz	r2, 1$

	mov	_draw_xy, _DPL1
	mov	(_draw_xy + 1), _DPH1

    __endasm ;	
}

void draw_string(const __code char *str)
{
    str = str;
    __asm
	mov	_DPL1, _draw_xy
	mov	_DPH1, (_draw_xy + 1)

1$:
	clr	a		; Read string byte
	movc	a, @a+DPTR
	jz	2$		; NUL terminator?

	add	a, #-0x20	; Font starts at 0x20
	clr	c		; Tile index to low 7 bits, with extra bit in C
	rlc	a

	mov	_DPS, #1	; Write low byte to destination
	movx	@dptr, a
	inc	dptr

	mov	a, _draw_attr	; Add C to _draw_attr
	addc	a, #0
	movx	@dptr, a	; Write to high byte
	inc	dptr

	mov	_DPS, #0	; Next string byte
	inc	dptr
	sjmp	1$

2$:
	mov	_draw_xy, _DPL1
	mov	(_draw_xy + 1), _DPH1

    __endasm ;
}

void draw_hex(uint8_t value)
{
    value = value;
    __asm
	mov	a, DPL
	mov	_DPL, _draw_xy
	mov	_DPH, (_draw_xy + 1)

	mov	r0, a		; Store low nybble for later	
	swap	a		; Start with the high nybble
	mov	r1, #2		; Loop over two nybbles
2$:

	anl	a, #0xF
	clr 	ac
	clr	c
	da	a		; If > 9, add 6. This almost covers the gap between '9' and 'A'
	jnb	acc.4, 1$	;   ... at least we can test efficiently whether the nybble was >9 now
	inc	a		;   and add one more. This closes the total gap (7 characters)
1$:	add	a, #0x10	; Add index for '0'
	rl	a		; Index to 7:7

	movx	@dptr, a	; Draw, with current address and attribute
	inc	dptr
	mov	a, _draw_attr
	movx	@dptr, a
	inc	dptr

	mov	a, r0		; Restore saved nybble
	djnz	r1, 2$		; Next!

	mov	_draw_xy, _DPL
	mov	(_draw_xy + 1), _DPH
    __endasm ;
}
