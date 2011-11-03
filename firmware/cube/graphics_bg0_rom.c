/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * Thundercracker cube firmware
 *
 * M. Elizabeth Scott <beth@sifteo.com>
 * Copyright <c> 2011 Sifteo, Inc. All rights reserved.
 */

#include "graphics_bg0.h"

// Soak up the space between BG1 and the Tile ROM
#pragma codeseg BG1LINE

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
 *   Palette:           7654....        <- one replicated palette-select nybble
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

        movx    a, @dptr                ; Tile map, low byte
        inc     dptr
        anl     6, #1                   ; Combine with LSB from r6
        orl     6, a

        movx    a, @dptr                ; Tile map, high byte
        inc     dptr
        mov     r0, a                   ; Save raw copy
        anl     a, #0x06
        anl     7, #0xF9                ; Combine with line-index and base address
        orl     7, a

        ASM_X_WRAP_CHECK(2$)

        mov     _DPS, #1                ; Switch to DPTR1. (DPTR is used only for the tile map)

        ; Load the palette, only if it has changed since the last tile.
        ; We bank on having relatively few palette changes, so that we
        ; can amortize the relatively high cost of reading palette data
        ; from code memory. It is very slow to do this on each pixel,
        ; even just for the single palette index we need.

        mov     a, r0
        anl     a, #0xf0                ; Mask off four palette-select bits
        xrl     a, r3                   ;    Compare with r3
        jz      1$                      ;    Palette has not changed
        xrl     a, r3                   ;    Make this the new current palette
        mov     r3, a

        mov     psw, #(GFX_BANK << 3)
        mov     dptr, #_rom_palettes

        jmp     @a+dptr                 ; Tail call to generated code, loads r0-r7.
        
1$:     ret                             ; No palette change

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

        lcall   _vm_bg0_rom_next_tile
        mov     psw, #0
        mov     _DPL1, r6
        mov     _DPH1, r7

        ; Fetch Plane 0 byte. This is necessary for both the 2-color and 4-color paths.

        clr     a                       ; Grab tile bitmap byte
        movc    a, @a+dptr
        mov     (GFX_BANK*8+1), a       ;    Store in Plane 0 register

        ; Bit unpacking loop

        mov     a, r0                   ; Mode bit:
        jnb     acc.3, 13$              ;    Are we using 2-color mode?
        sjmp    3$                      ;    4-color mode
        
8$:
        mov     psw, #0                 ; Restore bank
        mov     _DPS, #0                ; Must restore DPTR
        djnz    r5, 2$                  ; Next tile
        ret

        ; ---- 4-color mode

3$:
        mov     a, #2                   ; Offset by one tile (Undefined across 128-tile boundaries)
        movc    a, @a+dptr              ; Grab second bitmap byte
        mov     r2, a                   ;    Store in Plane 1 register
        mov     r4, #8                  ; Loop over 8 bytes
        mov     psw, #(GFX_BANK << 3)

4$:
        mov     a, ar2                  ; Shift out LSB on Plane 1
        rrc     a
        mov     ar2, a
        jc      9$

        mov     a, r1                   ; Plane 1 = 0, test Plane 0
        rrc     a
        mov     r1, a
        jc      10$
        PIXEL_FROM_REG(r0)
        djnz    ar4, 4$
        sjmp    8$
10$:    PIXEL_FROM_REGS(r2,r3)
        djnz    ar4, 4$
        sjmp    8$

9$:
        mov     a, r1                   ; Plane 1 = 1, test Plane 0
        rrc     a
        mov     r1, a
        jc      12$
        PIXEL_FROM_REGS(r4,r5)
        djnz    ar4, 4$
        sjmp    8$
12$:    PIXEL_FROM_REGS(r6,r7)
        djnz    ar4, 4$
        sjmp    8$

        ; ---- 2-color loop (unrolled)

        ; This is optimized for runs of index 0.  Since we require index 0
        ; to have identical MSB and LSB, we can use this to avoid reloading
        ; BUS_PORT at all, unless we hit a '1' bit.

13$:
        mov     psw, #(GFX_BANK << 3)
        mov     a, r1
        mov     BUS_PORT, r0            ; Default to index 0
        jz      14$                     ; Blank-tile loop

        rrc     a                       ; Index 0 ladder
        jc      30$
        ASM_ADDR_INC4()
        rrc     a
        jc      31$
21$:    ASM_ADDR_INC4()
        rrc     a
        jc      32$
22$:    ASM_ADDR_INC4()
        rrc     a
        jc      33$
23$:    ASM_ADDR_INC4()
        rrc     a
        jc      34$
24$:    ASM_ADDR_INC4()
        rrc     a
        jc      35$
25$:    ASM_ADDR_INC4()
        rrc     a
        jc      36$
26$:    ASM_ADDR_INC4()
        rrc     a
        jc      37$
27$:    ASM_ADDR_INC4()
        ljmp    8$

14$:    ljmp    15$                     ; Longer jump to blank-tile loop

30$:    PIXEL_FROM_REGS(r2,r3)          ; Index 1 ladder
        rrc     a
        jnc     41$
31$:    PIXEL_FROM_REGS(r2,r3)
        rrc     a
        jnc     42$
32$:    PIXEL_FROM_REGS(r2,r3)
        rrc     a
        jnc     43$
33$:    PIXEL_FROM_REGS(r2,r3)
        rrc     a
        jnc     44$
34$:    PIXEL_FROM_REGS(r2,r3)
        rrc     a
        jnc     45$
35$:    PIXEL_FROM_REGS(r2,r3)
        rrc     a
        jnc     46$
36$:    PIXEL_FROM_REGS(r2,r3)
        rrc     a
        jnc     47$
37$:    PIXEL_FROM_REGS(r2,r3)
        ljmp    8$

41$:    mov     BUS_PORT, r0            ; Transition 1 -> 0 ladder
        ljmp    21$
42$:    mov     BUS_PORT, r0
        ljmp    22$
43$:    mov     BUS_PORT, r0
        ljmp    23$
44$:    mov     BUS_PORT, r0
        ljmp    24$
45$:    mov     BUS_PORT, r0
        ljmp    25$
46$:    mov     BUS_PORT, r0
        ljmp    26$
47$:    mov     BUS_PORT, r0
        ljmp    27$

15$:    lcall   _addr_inc32     ; Blank byte, no comparisons
        ljmp    8$

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

        lcall   _vm_bg0_rom_next_tile
        mov     psw, #0
        mov     _DPL1, r6
        mov     _DPH1, r7

        clr     a                       ; Grab tile bitmap byte
        movc    a, @a+dptr
        mov     (GFX_BANK*8+1), a       ;    Store in Plane 0 register

        mov     a, #2                   ; Offset by one tile (Undefined across 128-tile boundaries)
        movc    a, @a+dptr              ; Grab second bitmap byte
        mov     r2, a                   ;    Store in Plane 1 register

11$:    cjne    r5, #0, 10$             ; Bit skip loop

        mov     a, r0                   ; Mode bit:
        mov     psw, #(GFX_BANK << 3)   ;    Switch to GFX_BANK on our way out...
        jb      acc.3, 4$               ;    Are we using 4-color mode?
        
        ; ---- 2-color mode

7$:
        mov     a, r1                   ; Plane 1 = 0, test Plane 0
        rrc     a
        mov     r1, a
        jc      8$
        PIXEL_FROM_REG(r0)
        djnz    ar4, 7$
        sjmp    5$
8$:     PIXEL_FROM_REGS(r2,r3)
        djnz    ar4, 7$

        ; ---- Cleanup

5$:
        mov     psw, #0                 ; Restore bank
        mov     _DPS, #0                ; Must restore DPTR
        ret

        ; ---- Bottom half of bit skip loop

10$:    dec    r5

        mov    a, (GFX_BANK*8+1)        ; Plane 0
        rr     a
        mov    (GFX_BANK*8+1), a

        mov    a, r2                    ; Plane 1
        rr     a
        mov    r2, a

        sjmp   11$

        ; ---- 4-color mode

4$:
        mov     a, ar2                  ; Shift out LSB on Plane 1
        rrc     a
        mov     ar2, a
        jc      2$

        mov     a, r1                   ; Plane 1 = 0, test Plane 0
        rrc     a
        mov     r1, a
        jc      3$
        PIXEL_FROM_REG(r0)
        djnz    ar4, 4$
        sjmp    5$
3$:     PIXEL_FROM_REGS(r2,r3)
        djnz    ar4, 4$
        sjmp    5$

2$:
        mov     a, r1                   ; Plane 1 = 1, test Plane 0
        rrc     a
        mov     r1, a
        jc      6$
        PIXEL_FROM_REGS(r4,r5)
        djnz    ar4, 4$
        sjmp    5$
6$:     PIXEL_FROM_REGS(r6,r7)
        djnz    ar4, 4$
        sjmp    5$

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
        mov     r3, #0xFF
    __endasm ;

    /*
     * We keep the wrap counter stored in a register, for fast x-wrap checks.
     */

    __asm
        mov     r1, _x_bg0_wrap
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
        mov     a, _y_bg0_addr_l        ; 765xxxxx
        swap    a                       ; xxxx765x
        rr      a                       ; xxxxx765
        mov     r6, a
        anl     ar6, #1
        clr     c
        rrc     a                       ; xxxxxx76 5
        clr     c
        rrc     a                       ; xxxxxxx7 6
        rl      a                       ; xxxxxx7x 6
        rl      a                       ; xxxxx7xx 6
        rlc     a                       ; xxxx7xx6 x
        orl     a, #(_rom_tiles >> 8)   ; Add base address
        mov     r7, a
    __endasm ;

    /*
     * Segment the line into a fast full-tile burst, and up to two slower partial tiles.
     */

    __asm

        ; First tile, may be skipping up to 7 pixels from the beginning

        mov     r4, _x_bg0_first_w
        mov     r5, _x_bg0_last_w
        lcall   _vm_bg0_rom_tile_partial

        ; Always have a run of 15 full tiles

        mov     r5, #15
        lcall   _vm_bg0_rom_tiles_fast

        ; May have a final partial tile

        mov     a, _x_bg0_last_w
        jz      1$
        inc     r1              ; Negate the x-wrap check for this tile
        mov     r4, a           ; Width computed earlier
        mov     r5, #0          ; No skipped bits
        lcall   _vm_bg0_rom_tile_partial
1$:

    __endasm ;

    LCD_WRITE_END();
}

void vm_bg0_rom(void)
{
    uint8_t y = vram.num_lines;

    lcd_begin_frame();
    vm_bg0_setup();

    do {
        vm_bg0_rom_line();
        vm_bg0_next();
    } while (--y);    

    lcd_end_frame();
}
