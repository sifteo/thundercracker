/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * Thundercracker cube firmware
 *
 * M. Elizabeth Scott <beth@sifteo.com>
 * Copyright <c> 2011 Sifteo, Inc. All rights reserved.
 */

#ifndef _GRAPHICS_H
#define _GRAPHICS_H

#include <stdint.h>

void graphics_init();
void graphics_render();

#define NUM_SPRITES  		2
#define CHROMA_KEY		0xF5

#define VRAM_LATCHED		800
#define VRAM_LATCHED_SIZE	(NUM_SPRITES * 6 + 2)

#define LVRAM_PAN_X		0
#define LVRAM_PAN_Y		1
#define LVRAM_SPRITES		3

#define LVRAM_SPR_X(i)		(LVRAM_SPRITES + (i)*6 + 0)
#define LVRAM_SPR_Y(i)		(LVRAM_SPRITES + (i)*6 + 1)
#define LVRAM_SPR_MX(i)		(LVRAM_SPRITES + (i)*6 + 2)
#define LVRAM_SPR_MY(i)		(LVRAM_SPRITES + (i)*6 + 3)
#define LVRAM_SPR_AH(i)		(LVRAM_SPRITES + (i)*6 + 4)
#define LVRAM_SPR_AL(i)		(LVRAM_SPRITES + (i)*6 + 5)

struct sprite_info {
    uint8_t x, y;
    uint8_t mask_x, mask_y;
    uint8_t addr_h, addr_l;
};

struct latched_vram {
    // The small portion of VRAM that we latch before every frame
    uint8_t pan_x, pan_y;
    struct sprite_info sprites[NUM_SPRITES];
};

static __xdata __at 0x0000 union {
    uint8_t bytes[1024];
    struct {
	uint8_t tilemap[800];
	struct latched_vram latched;
	uint8_t frame_trigger;
    };
} vram;

#endif
