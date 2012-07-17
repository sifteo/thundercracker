/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * Thundercracker cube firmware
 *
 * M. Elizabeth Scott <beth@sifteo.com>
 * Copyright <c> 2011 Sifteo, Inc. All rights reserved.
 */

/*
 * Drawing module: For producing graphics locally on the cube, using a
 * ROM-based video mode. The "images" here are tiny uncompressed tile
 * maps.
 */

#ifndef _DRAW_H
#define _DRAW_H

// Initialize VRAM
void draw_clear();

// Done with ROM drawing, get ready to take commands from the master
void draw_exit();

// Draw a ROM image at the current position. XY coord in DPTR1
void draw_image(const __code uint8_t *image);

// Pre-compute address of an XY coordinate. (tiles -> bytes)
#define XY(_x, _y)      (((_x)<<1) + (_y) * (_SYS_VRAM_BG0_WIDTH * 2))

// Macro to set XY
#pragma sdcc_hash +
#define DRAW_XY(_x, _y)                     __endasm ;\
    __asm mov   _DPL1, #(XY(_x,_y) >> 0)    __endasm ;\
    __asm mov   _DPH1, #(XY(_x,_y) >> 8)    __endasm ;\
    __asm
            
#endif
