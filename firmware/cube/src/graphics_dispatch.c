/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * Thundercracker cube firmware
 *
 * M. Elizabeth Scott <beth@sifteo.com>
 * Copyright <c> 2011 Sifteo, Inc. All rights reserved.
 */

#include "graphics.h"
#include "sensors.h"
#include "power.h"

uint8_t next_ack;


void graphics_render(void) __naked
{
    /*
     * Check the toggle bit (rendering trigger), in bit 1 of
     * vram.flags. If it matches the LSB of frame_count, we have
     * nothing to do.
     *
     * This is also where we calculate the next ACK byte, which will
     * be sent back after this frame is fully rendered. The bits in this
     * byte are explained by protocol.h.
     *
     * We set i2c_a21_target from _SYS_VF_A21 here, since it's convenient
     * to do so, but we do _not_ call i2c_a21_wait() automatically. Not
     * all video modes use flash at all, so we only wait if necessary.
     * (It is still possible for A21 to be set based on our change to
     * i2c_a21_target, but we don't require it.)
     */

    // Reset watchdog ONLY in main loop!
    power_wdt_set();

    __asm

        orl     _next_ack, #FRAME_ACK_CONTINUOUS

        mov     dptr, #_SYS_VA_FLAGS
        movx    a, @dptr

        ; Store target A21 value, from _SYS_VF_A21

        mov     c, acc.4
        mov     _i2c_a21_target, c

        ; Frame triggering

        jb      acc.3, 1$               ; Check _SYS_VF_CONTINUOUS

        anl     _next_ack, #~FRAME_ACK_CONTINUOUS
        rr      a
        xrl     a, _next_ack            ; Compare _SYS_VF_TOGGLE with bit 0
        rrc     a
        jnc     _graphics_ack           ; Return to ACK if no toggle
1$:

        ; Increment frame counter field

        mov     a, _next_ack
        inc     a
        xrl     a, _next_ack
        anl     a, #FRAME_ACK_COUNT
        xrl     _next_ack, a

    __endasm ;

    // Set up colormap (Used by FB32, STAMP)
    DPH1 = _SYS_VA_COLORMAP >> 8;

    /*
     * Video mode jump table.
     *
     * This acts as a tail-call. The mode-specific function returns
     * on our behalf, after acknowledging the frame.
     */

    __asm
        mov     dptr, #_SYS_VA_MODE
        movx    a, @dptr
        anl     a, #_SYS_VM_MASK
        mov     dptr, #gd_table
    
        ; Allow other modules to reuse this instruction
gd_jmp: jmp     @a+dptr

        ; Computed jump table
        ; Static analysis NOTE dyn_branch gd_jmp gd_n

gd_table:
gd_n1:  ajmp    _vm_powerdown   ; 0x00
        .ds 2
gd_n2:  ljmp    _vm_bg0_rom     ; 0x04
        .ds 1
gd_n3:  ljmp    _vm_solid       ; 0x08
        .ds 1
gd_n4:  ljmp    _vm_fb32        ; 0x0c
        .ds 1
gd_n5:  ljmp    _vm_fb64        ; 0x10
        .ds 1
gd_n6:  ljmp    _vm_fb128       ; 0x14
        .ds 1
gd_n7:  ajmp    _vm_bg0         ; 0x18
        .ds 2
gd_n8:  ajmp    _vm_bg0_bg1     ; 0x1c
        .ds 2
gd_n9:  ajmp    _vm_bg0_spr_bg1 ; 0x20
        .ds 2
gd_n10: ljmp    _vm_bg2         ; 0x24
        .ds 1
gd_n11: ljmp    _vm_stamp       ; 0x28
        .ds 1
gd_n12: ajmp    _vm_powerdown   ; 0x2c (unused)
        .ds 2
gd_n13: ajmp    _vm_powerdown   ; 0x30 (unused)
        .ds 2
gd_n14: ajmp    _vm_powerdown   ; 0x34 (unused)
        .ds 2
gd_n15: ajmp    _vm_powerdown   ; 0x38 (unused)
        .ds 2
gd_n16: ajmp    _vm_sleep       ; 0x3c

    __endasm ;
}

void vm_powerdown() __naked
{
    lcd_sleep();
    GRAPHICS_ARET();
}

void vm_sleep() __naked
{
    lcd_pwm_fade();
    power_sleep();
}

void graphics_ack(void) __naked
{
    /*
     * If next_ack doesn't match RF_ACK_FRAME, send a packet.
     * This must happen only after the frame has fully rendered.
     */

    __asm

        mov     a, _next_ack
        xrl     a, (_ack_data + RF_ACK_FRAME)
        jz      1$
        xrl     (_ack_data + RF_ACK_FRAME), a
        orl     _ack_bits, #RF_ACK_BIT_FRAME
1$:
        ajmp    _graphics_render_ret

    __endasm ;
}
