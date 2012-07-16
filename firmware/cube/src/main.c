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

        ;---------------------------------
        ; Reset Vector
        ;---------------------------------

v_0000:
        mov     sp, #(__start__stack - 1)       ; Init stack
        clr     a                               ; IRAM clear loop
        mov     r0, a
1$:     mov     @r0, a
        djnz    r0, 1$
        sjmp    init_1

        .ds     1

        ;---------------------------------
        ; TF0 Vector
        ;---------------------------------

v_000b: ljmp    _tf0_isr

        ;---------------------------------

init_1:
        lcall   _power_init         ; Start subsystem init sequence
        lcall   _params_init
        lcall   _flash_init
        sjmp    init_2

        .ds     2

        ;---------------------------------
        ; TF1 Vector
        ;---------------------------------

v_001b: ljmp    _tf1_isr

        ;---------------------------------

init_2:
        lcall   _radio_init         ; Subsystem init, continued
        lcall   _sensors_init
        setb    _IEN_EN             ; Global interrupt enable (subsystem init done)
        setb    _RF_CE              ; Radio enable
        sjmp    init_3

        .ds     1

        ;---------------------------------
        ; TF2 Vector
        ;---------------------------------

v_002b: ljmp    _tf2_isr

init_3:
        lcall   _disconnected_init              ; Set up disconnected-mode idle screen
        ; Fall through...

        ;---------------------------------
        ; Graphics return (Main Loop)
        ;---------------------------------

_graphics_render_ret::

        jb      _radio_connected, 2$            ; Skip disconnected tasks if connected
        lcall   _disconnected_poll
2$:

        lcall   _flash_handle_fifo              ; Flash polling task
        sjmp    _graphics_render                ; Branch to graphics mode handler

        ;---------------------------------

        .ds 4
        .db 0
        .ascii "@scanlime"
        .db 0

        ;---------------------------------
        ; Radio Vector
        ;---------------------------------

v_004b: ljmp    _radio_isr

        .ds 5

        ;---------------------------------
        ; SPI/I2C Vector
        ;---------------------------------

v_0053: ljmp    _spi_i2c_isr

        ;---------------------------------

        .db 0
        .ascii "(C) 2012 Sifteo Inc"
        .db 0

        ;---------------------------------
        ; TICK (RTC2) Vector
        ;---------------------------------

v_006b: setb    _RF_CE              ; Turn the radio back on
        mov     _RTC2CON, #0x09     ; Disable IRQ until the next nap
        reti

    __endasm ;
}
