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

// Draw a ROM image at the current position
void draw_image(const __code uint8_t *image);

// Current drawing state. Not preserved across graphics_render().
extern uint16_t draw_xy;

// Pre-compute address of an XY coordinate. (tiles -> bytes)
#define XY(_x, _y)      (((_x)<<1) + (_y) * (_SYS_VRAM_BG0_WIDTH * 2))

#endif
