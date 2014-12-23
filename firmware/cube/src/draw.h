/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * Thundercracker cube firmware
 *
 * M. Elizabeth Scott <beth@sifteo.com>
 * Copyright <c> 2011 Sifteo, Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
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
