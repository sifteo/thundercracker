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
 *
 * We also overlay variables onto unused SFRs. This has to be
 * done at the original declaration; so this file can't declare
 * the overlaid SFRs, but this comment attempts to keep a canonical
 * list of such overlaid registers:
 *
 *   C9     MPAGE      x_bg0_wrap
 *   B4     RTC2CMP0   x_bg0_first_addr
 *   B5     RTC2CMP1   y_spr_line
 *   A1     PWMDC0     y_spr_line_limit
 *   A2     PWMDC1     y_bg0_addr_l
 *   C2     CCL1       x_bg1_first_addr
 *   C3     CCH1       x_bg1_last_addr
 *   C4     CCL2       y_bg1_addr_l
 *   C5     CCH2       y_bg1_bit_index
 *   C6     CCL3       x_bg1_shift
 *   C7     CCH3       y_spr_active
 *   DD     CCPDATIA   x_bg0_first_w
 *   DE     CCPDATIB   x_bg0_last_w
 */

static void overlay_memory() __naked {
    __asm

    .area DSEG    (DATA)

; ----------------------------------

_y_bg0_map::        ; 2 bytes
    .ds 2
_y_bg1_map::        ; 2 bytes
    .ds 2
_fls_state::        ; 1 byte
    .ds 1
_fls_tail::         ; 1 byte
    .ds 1
_fls_st::           ; 5 bytes
    .ds 5
_x_bg0_first_addr:: ; 1 byte
    .ds 1
_y_spr_line::       ; 1 byte
    .ds 1

; ----------------------------------

_x_spr::            ;     20 bytes
_lcd_window_x::     ;     1 byte
    .ds 1           ; +0
_lcd_window_y::     ;     1 byte
    .ds 1           ; +1
_bg2_state::        ;     14 bytes
    .ds 2           ; +2
_disc_logo_x::      ;     2 bytes
    .ds 2           ; +4
_disc_logo_y::      ;     2 bytes
    .ds 2           ; +6
_disc_logo_dx::     ;     2 bytes
    .ds 2           ; +8
_disc_logo_dy::     ;     2 bytes
    .ds 2           ; +10
_disc_score::       ;     1 byte
    .ds 1           ; +12
_disc_sleep_timer:: ;     1 byte
    .ds 1           ; +13
_disc_hop_timer::   ;     1 byte
    .ds 1           ; +14
_disc_nb_base::     ;     1 byte
    .ds 1           ; +15
    .ds 20-16       ; +16

; ----------------------------------

_fls_lut::          ;     32 bytes
_radio_query::      ;     32 bytes
    .ds 32          ; +0

; ----------------------------------

    .area BSEG    (BIT)

_x_bg1_rshift::
    .ds 1
_x_bg1_lshift::
    .ds 1
_disc_battery_draw::
_x_bg1_offset_bit0::
    .ds 1
_disc_bounce_type::
_x_bg1_offset_bit1::
    .ds 1
_disc_has_trophy::
_x_bg1_offset_bit2::
    .ds 1
_y_bg1_empty::
    .ds 1
_radio_connected::
    .ds 1
_radio_idle_hop::
    .ds 1

; ----------------------------------

    __endasm ;
}
