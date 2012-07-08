/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * Thundercracker cube firmware
 *
 * M. Elizabeth Scott <beth@sifteo.com>
 * Copyright <c> 2011 Sifteo, Inc. All rights reserved.
 */

#include "power.h"
#include "sensors.h"
#include "radio.h"
#include "graphics.h"
#include "cube_hardware.h"
#include "flash.h"
#include "params.h"
#include "battery.h"
#include "disconnected.h"

__bit global_busy_flag;


void startup() __naked
{
    /*
     * To save space, we don't use SDCC's standard libraries. And when you
     * aren't using the standard startup code, SDCC's handling of main()
     * and the IVT are kind of broken. We sidestep all of this by building
     * the IVT manually, and interleaving it with our setup code.
     *
     * Our early startup code only needs to initialize the stack and zero IRAM.
     * We have no initialized data, and XRAM does not need to be zero'ed
     * this early. (We zero it in draw_clear later on)
     *
     * Global interrupt enable is set only AFTER every module has a chance to
     * run its init() code. Modules may set subsystem-specific interrupt
     * enables, but they may not set the global interrupt enable on their
     * own.
     *
     * And finally, after the global interrut enable, we turn on the radio.
     * If a radio packet comes in earlier, its interrupt will be delayed;
     * and not all of our initialization code will correctly preserve this
     * flag in IRCON.
     */

    __asm
        ; Define the stack, in internal RAM.
        .area   SSEG (DATA)
__start__stack:
        .ds     1

        ; Emit code at the beginning of code space, in the IVT area.
        ; The NOTE lines below are handled specially by the static analyzer.

        .area   HOME (CODE)

v_0000: ; ============= NOTE vector 0x0000 __start__stack - Reset

        mov     sp, #(__start__stack - 1)       ; Init stack
        clr     a                               ; IRAM clear loop
        mov     r0, a
1$:     mov     @r0, a
        djnz    r0, 1$
        sjmp    init_1
        .ds     1

v_000b: ; ============= NOTE vector 0x000b 2 - TF0
        ljmp    _tf0_isr

init_1:
        lcall   _power_init         ; Start subsystem init sequence
        lcall   _radio_init
        lcall   _flash_init
        sjmp    init_2
        .ds     2

v_001b: ; ============= NOTE vector 0x001b 2 - TF1
        ljmp    _tf1_isr

init_2:
        lcall   _sensors_init       ; Subsystem init, continued
        lcall   _params_init
        setb    _IEN_EN             ; Global interrupt enable (subsystem init done)
        setb    _RF_CE              ; Radio enable
        sjmp    init_3
        .ds     1

v_002b: ; ============= NOTE vector 0x002b 2 - TF2
        ljmp    _tf2_isr

        .ds 29                      ; Reserved for now

v_004b: ; ============= NOTE vector 0x004b 2 - Radio
        ljmp    _radio_isr

        .ds 5

v_0053: ; ============= NOTE vector 0x0053 2 - SPI/I2C
        ljmp    _spi_i2c_isr

init_3:

    __endasm ;

    /**********************************************************************
     * Main Loop
     **********************************************************************/

    // XXX: Will go into main loop later
    disconnected_screen();

    __asm
_graphics_render_ret::

        ; Reset watchdog ONLY in main loop!
        __endasm; power_wdt_set(); __asm
        
        ;---------------------------------
        ; Idle-only Tasks
        ;---------------------------------

        jb      _global_busy_flag, 1$

        lcall   _battery_poll

1$:     clr     _global_busy_flag

        ;---------------------------------
        ; Main Tasks
        ;---------------------------------

        lcall   _flash_handle_fifo

        #ifndef DISABLE_SLEEP
            __endasm; power_idle_poll(); __asm
        #endif

        ljmp    _graphics_render

    __endasm ;
}
