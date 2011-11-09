/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * Thundercracker cube firmware
 *
 * M. Elizabeth Scott <beth@sifteo.com>
 * Copyright <c> 2011 Sifteo, Inc. All rights reserved.
 */
 
/*
 * Manually overlaid data memory.
 *
 * Many of our global variables are defined here, so that we can
 * control their memory layout. We can reuse the same memory for
 * video modes that we know are mutually exclusive, for example,
 * or for operations (like draw_*) that are mutually exclusive
 * with graphics rendering.
 *
 * Overlay contents below are grouped by size. We try to keep word
 * vars with word vars, and byte vars with byte vars. Bits are
 * declared in our separate bit-addressable segment.
 */

static void overlay_memory() __naked {
    __asm

	.area DSEG    (DATA)

_x_bg0_first_w::
	.ds 1
_x_bg0_last_w::
	.ds 1
_x_bg0_first_addr::
	.ds 1
_x_bg0_wrap::
	.ds 1
_y_bg0_addr_l::
	.ds 1
_y_bg0_map::
_draw_xy::
	.ds 2
_y_spr_line::
_draw_attr::
	.ds 1
_y_spr_line_limit::
	.ds 1
_y_spr_active::
	.ds 1
_y_bg1_map::
	.ds 2
_x_bg1_shift::
	.ds 1
_x_bg1_first_addr::
	.ds 1
_x_bg1_last_addr::
	.ds 1
_y_bg1_addr_l::
	.ds 1
_y_bg1_bit_index::
	.ds 1
_x_spr::            ; 20 bytes
_bg2_state::        ; 14 bytes
	.ds 20

	.area BSEG    (BIT)

_x_bg1_rshift::
	.ds 1
_x_bg1_lshift::
	.ds 1
_x_bg1_offset_bit0::
	.ds 1
_x_bg1_offset_bit1::
	.ds 1
_x_bg1_offset_bit2::
	.ds 1
_y_bg1_empty::
	.ds 1

    __endasm ;
}
