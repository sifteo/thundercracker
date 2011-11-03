/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * Thundercracker cube firmware
 *
 * M. Elizabeth Scott <beth@sifteo.com>
 * Copyright <c> 2011 Sifteo, Inc. All rights reserved.
 */

#include "graphics.h"
#include "main.h"


void graphics_render(void) __naked
{
    /*
     * Check the toggle bit (rendering trigger), in bit 1 of
     * vram.flags. If it matches the LSB of frame_count, we have
     * nothing to do.
     */

    __asm
        mov     dptr, #_SYS_VA_FLAGS
        movx    a, @dptr
        jb      acc.3, 1$                       ; Handle _SYS_VF_CONTINUOUS
        rr      a
        xrl     a, (_ack_data + RF_ACK_FRAME)   ; Compare _SYS_VF_TOGGLE with frame_count LSB
        rrc     a
        jnc     3$                              ; Return if no toggle
1$:
    __endasm ;
    
    global_busy_flag = 1;

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
        mov     dptr, #10$
        jmp     @a+dptr

        ; These redundant labels are required by the binary translator!

10$:    ljmp    _lcd_sleep      ; 0x00
        nop
11$:    ljmp    _vm_bg0_rom     ; 0x04
        nop
12$:    ljmp    _vm_solid       ; 0x08
        nop
13$:    ljmp    _vm_fb32        ; 0x0c
        nop
14$:    ljmp    _vm_fb64        ; 0x10
        nop
15$:    ljmp    _vm_fb128       ; 0x14
        nop
16$:    ljmp    _vm_bg0         ; 0x18
        nop
17$:    ljmp    _vm_bg0_bg1     ; 0x1c
        nop
18$:    ljmp    _vm_bg0_spr_bg1 ; 0x20
        nop
19$:    ljmp    _vm_bg2         ; 0x24
        nop
20$:    ljmp    _lcd_sleep      ; 0x28 (unused)
        nop
21$:    ljmp    _lcd_sleep      ; 0x2c (unused)
        nop
22$:    ljmp    _lcd_sleep      ; 0x30 (unused)
        nop
23$:    ljmp    _lcd_sleep      ; 0x34 (unused)
        nop
24$:    ljmp    _lcd_sleep      ; 0x38 (unused)
        nop
25$:    ljmp    _lcd_sleep      ; 0x3c (unused)

3$:     ret

    __endasm ;
}
