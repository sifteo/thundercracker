/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * Thundercracker cube firmware
 *
 * M. Elizabeth Scott <beth@sifteo.com>
 * Copyright <c> 2011 Sifteo, Inc. All rights reserved.
 */

#ifndef _GRAPHICS_SPRITE_H
#define _GRAPHICS_SPRITE_H

#include "graphics_bg1.h"

/*
 * Shared state
 */
  
#define XSPR_MASK(id)       (_x_spr + (0 + (id) * 5))
#define XSPR_POS(id)        (_x_spr + (1 + (id) * 5))
#define XSPR_LAT1(id)       (_x_spr + (2 + (id) * 5))
#define XSPR_LAT2(id)       (_x_spr + (3 + (id) * 5))
#define XSPR_LINE_ADDR(id)  (_x_spr + (4 + (id) * 5))

struct x_sprite_t {
    int8_t mask;
    int8_t pos;
    uint8_t lat1;
    uint8_t lat2;
    uint8_t line_addr;
};

// Horizontal state for each sprite
extern struct x_sprite_t x_spr[_SYS_SPRITES_PER_LINE];

extern uint8_t   y_spr_line;            // Current rendering line, zero-based
extern uint8_t   y_spr_line_limit;      // Rendering ends at this line
extern uint8_t   y_spr_active;          // Number of sprites in x_spr[]

/*
 * Scanline renderer registers
 */
 
#define R_SPR_ACTIVE    r6

/*
 * Shared code
 */
 
void vm_spr_setup();
void vm_spr_next();
void vm_bg0_spr_line();
void vm_bg0_spr_bg1_line();

/*
 * Macros
 */

#endif
