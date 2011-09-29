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

// Draw a ROM image at the current position
void draw_image(const __code uint8_t *image);

// Draw an ASCII string, advancing draw_xy as we go.
void draw_string(const __code char *str);

// Draw a hexadecimal byte
void draw_hex(uint8_t value);

// Current drawing state
extern uint16_t draw_xy;
extern uint8_t draw_attr;

// Pre-compute address of an XY coordinate. (tiles -> bytes)
#define XY(_x, _y)      (((_x)<<1) + (_y) * (_SYS_VRAM_BG0_WIDTH * 2))

/*
 * Attribute flags. These bytes are XOR'ed with the most significant byte of
 * an image or character index. They can be used to shift the color palette.
 */
#define ATTR_NONE       0x00
#define ATTR_BLUE       0x10
#define ATTR_ORANGE     0x20
#define ATTR_INVORANGE  0x30
#define ATTR_RED        0x40
#define ATTR_GRAY       0x50
#define ATTR_INV        0x60
#define ATTR_INVGRAY    0x70
#define ATTR_LTBLUE     0x80
#define ATTR_LTORANGE   0x90

#endif
