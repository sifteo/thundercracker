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
  
#define XSPR_TILE       0
#define XSPR_MASK       2
#define XSPR_POS        3

struct x_sprite_t {
    uint16_t tile;
    uint8_t mask;
    int8_t pos;
};

// Horizontal state for each sprite
extern struct x_sprite_t __idata x_spr[_SYS_VRAM_SPRITES];

extern uint8_t   y_spr_line;            // Current rendering line, zero-based
extern uint8_t   y_spr_line_limit;      // Rendering ends at this line
extern uint8_t   y_spr_active;          // Bitmap of sprites that are active on this line

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
