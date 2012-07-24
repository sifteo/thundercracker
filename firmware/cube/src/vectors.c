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

        sjmp    init_1                          ; Continue init below...

        .ds     1

        ;---------------------------------
        ; TF0 Vector
        ;---------------------------------

v_000b: ljmp    _tf0_isr

        ;---------------------------------

init_1:

1$:     mov     a, _CLKLFCTRL                   ; Wait for 16 MHz oscillator startup
        jnb     acc.3, 1$

        lcall   _radio_init                     ; Turn on radio, and program it with static settings
        lcall   _power_init                     ; Check wakeup reason, handle wake-on-RF, turn on power rails

        sjmp    init_2                          ; Continue init below...

        ;---------------------------------
        ; TF1 Vector
        ;---------------------------------

v_001b: ljmp    _tf1_isr

        ;---------------------------------

init_2:
        lcall   _params_init                    ; Initialize HWID in NVM
        lcall   _flash_init                     ; Init flash state machine
        lcall   _sensors_init                   ; Init sensor IRQs
        acall   v_004b                          ; Manually run RF interrupt, to drain RX FIFO

        sjmp    init_3                          ; Continue init below...

        ;---------------------------------
        ; TF2 Vector
        ;---------------------------------

v_002b: ljmp    _tf2_isr

        ;---------------------------------
        ; Graphics Return Vector
        ;---------------------------------

_graphics_render_ret::

        jb      _radio_connected, 1$            ; Skip disconnected tasks if connected
        ljmp    _disconnected_poll              ; One frame in disconnected-mode main loop
1$:
        mov     a, _sensor_tick_counter_high    ; Check connection timeout
        cjne    a, _radio_packet_deadline, 2$
        sjmp    disconnected_init_sjmp          ; Disconnect if we reach the deadline
2$:
        lcall   _flash_handle_fifo              ; Pump flash FIFO
        sjmp    _graphics_render                ; Poll for graphics rendering

        ;---------------------------------

        .db 0
        .ascii "@scanlime"
        .db 0

        ;---------------------------------
        ; Radio Vector
        ;---------------------------------

v_004b: ljmp    _radio_isr

        ;---------------------------------

init_3:
        setb    _IEN_EN                         ; Global interrupt enable (subsystem init done)

disconnected_init_sjmp:
        ljmp    _disconnected_init              ; Init disconnected mode and enter graphics loop

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
