/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * Thundercracker cube firmware
 *
 * M. Elizabeth Scott <beth@sifteo.com>
 * Copyright <c> 2011 Sifteo, Inc. All rights reserved.
 */

#include <stdint.h>
#include <protocol.h>
#include <cube_hardware.h>

#include "radio.h"
#include "params.h"
#include "sensors.h"
#include "sensors_i2c.h"

uint8_t i2c_state;
uint8_t i2c_temp_1;
uint8_t i2c_temp_2;


/*
 * I2C ISR --
 *
 *    This is where we handle accelerometer reads. Since I2C is a
 *    pretty slow bus, and the hardware has no real buffering to speak
 *    of, we use this IRQ to implement a state machine that executes
 *    our I2C read byte-by-byte.
 *
 *    These reads get kicked off right after the neighbor ISR, so that we'll
 *    start taking a new accel reading any time the previous reading,
 *    if any, had a chance to be retrieved.
 *
 *    After every accelerometer read finishes, we do a quick poll for
 *    factory test commands.
 */

void spi_i2c_isr(void) __interrupt(VECTOR_SPI_I2C) __naked
{
    __asm
        push    acc
        push    psw
        push    dpl
        push    dph
        push    _DPS
        mov     _DPS, #0

        ;--------------------------------------------------------------------

        ; Check status of I2C engine.

        mov     a, _W2CON1
        jnb     acc.0, as_ret           ; Not a real I2C interrupt. Ignore it.
        jb      acc.1, as_nack          ; Was not acknowledged!

        mov     dptr, #as_1
        mov     a, _i2c_state
        jmp     @a+dptr

        ;--------------------------------------------------------------------

        ; NACK handler. Stop, and go to a no-op state until we are
        ; triggered again by TF2.

as_nack:
        orl     _W2CON0, #W2CON0_STOP  
        mov     _i2c_state, #(as_nop_state - as_1)

        ; Fall through to as_ret.

        ;--------------------------------------------------------------------

        ; Return from IRQ. We get called back after the next byte finishes.

as_ret:
        pop     _DPS
        pop     dph
        pop     dpl
        pop     psw
        pop     acc
        reti

        ;--------------------------------------------------------------------
        ; Accelerometer States
        ;--------------------------------------------------------------------

        ; 1. TX address finished, Send register address next

as_1:
        mov     _W2DAT, #ACCEL_START_READ_X
        mov     _i2c_state, #(as_2 - as_1)
as_nop_state:
        sjmp    as_ret

        ; 2. Register address finished. Send repeated start, and RX address
as_2:
        orl     _W2CON0, #W2CON0_START
        mov     _W2DAT, #ACCEL_ADDR_RX
        mov     _i2c_state, #(as_3 - as_1)
        sjmp    as_ret

        ; 3. RX address finished. Subsequent bytes will be reads.
as_3:
        mov     _i2c_state, #(as_4 - as_1)
        sjmp    as_ret

        ; 4. Read (and discard) X axis low byte.
as_4:
        mov     a, _W2DAT
        mov     _i2c_state, #(as_5 - as_1)
        sjmp    as_ret

        ; 5. Read X axis high byte.
as_5:
        mov     _i2c_temp_1, _W2DAT
        mov     _i2c_state, #(as_6 - as_1)
        sjmp    as_ret

        ; 6. Read (and discard) Y axis low byte.
as_6:
        mov     a, _W2DAT
        mov     _i2c_state, #(as_7 - as_1)
        sjmp    as_ret

        ; 7. Read Y axis high byte.
as_7:
    #if HWREV == 2
        ; y axis is inverted on rev 2 hardware
        mov     a, _W2DAT
        cpl     a
        mov     _i2c_temp_2, a
    #else
        mov     _i2c_temp_2, _W2DAT
    #endif
        mov     _i2c_state, #(as_8 - as_1)
        sjmp    as_ret

        ; 8. Read (and discard) Z axis low byte.
        ; This is the second-to-last byte, so we queue a Stop condition.
as_8:
        mov     a, _W2DAT
        orl     _W2CON0, #W2CON0_STOP ; stopping after last read below for now
        mov     _i2c_state, #(as_9 - as_1)
        sjmp    as_ret

        ; 9. Read Z axis high byte.
        ;
        ; In rapid succession, store both axes and set the change flag
        ; if necessary. This minimizes the chances of ever sending one old axis and
        ; one new axis. In fact, since this interrupt is higher priority than the
        ; RF interrupt, we are guaranteed to send synchronized updates of both axes.
as_9:

        mov     a, _W2DAT
        xrl     a, (_ack_data + RF_ACK_ACCEL + 2)
        jz      1$
        xrl     (_ack_data + RF_ACK_ACCEL + 2), a
        orl     _ack_bits, #RF_ACK_BIT_ACCEL
1$:

        mov     a, _i2c_temp_2
        xrl     a, (_ack_data + RF_ACK_ACCEL + 1)
        jz      2$
        xrl     (_ack_data + RF_ACK_ACCEL + 1), a
        orl     _ack_bits, #RF_ACK_BIT_ACCEL
2$:

        mov     a, _i2c_temp_1
        xrl     a, (_ack_data + RF_ACK_ACCEL + 0)
        jz      3$
        xrl     (_ack_data + RF_ACK_ACCEL + 0), a
        orl     _ack_bits, #RF_ACK_BIT_ACCEL
3$:

        ; Fall through to fs_1...

        ;--------------------------------------------------------------------
        ; Factory Test States
        ;--------------------------------------------------------------------

        ; See cube-factorytest-protocol.rst for a description of
        ; the overall sequence of operations here as well as the actual
        ; allowed packet formats.

        ; 1. Start writing.
        ;
        ; Unless we are connected to a factory test jig, this will just
        ; land us in as_nack next time through.
        ;
        ; Here we use i2c_temp_1 to hold the current address in our ACK,
        ; and i2c_temp_2 as a down-counter.

fs_1:
        mov     _W2DAT, #FACTORY_ADDR_TX
        mov     _i2c_temp_1, #_ack_data
        mov     _i2c_temp_2, #RF_MEM_ACK_LEN
        mov     _i2c_state, #(fs_2 - as_1)
fs_ret:
        ljmp    as_ret

        ; 2. Send an ACK byte.
        ;
        ; Note that we need to trash a register here in order to address
        ; the ACK buffer, but all of the non-test code manages to avoid
        ; touching anything other than the accumulator and DPTR. So, do a
        ; quick local save-and-restore of r0, taking care to also reset our
        ; register bank.

fs_2:
        mov     psw, #0             ; Back to register bank 0
        push    0                   ; Save R0
        mov     r0, _i2c_temp_1     ; Send next ACK byte
        mov     a, @r0
        mov     _W2DAT, a
        pop     0                   ; Restore R0

        inc     _i2c_temp_1         ; Iterate over all ACK bytes
        djnz    _i2c_temp_2, fs_ret

        mov     _i2c_temp_1, #(PARAMS_HWID & 0xFF)
        mov     _i2c_temp_2, #HWID_LEN
        mov     _i2c_state, #(fs_3 - as_1)
        sjmp    fs_ret

        ; 3. Send an HWID byte.
        ;
        ; These bytes come from internal OTP flash, accessed via DPTR.
        ; As above, we use temp_1 as a pointer and temp_2 as a counter.

fs_3:
        mov     dph, #(PARAMS_HWID >> 8)
        mov     dpl, _i2c_temp_1
        movx    a, @dptr
        mov     _W2DAT, a

        inc     _i2c_temp_1
        djnz    _i2c_temp_2, fs_ret
        mov     _i2c_state, #(fs_4 - as_1)
        sjmp    fs_ret

        ; 4. Send repeated start, and RX address

fs_4:
        orl     _W2CON0, #W2CON0_START
        mov     _W2DAT, #FACTORY_ADDR_RX
        mov     _i2c_state, #(fs_5 - as_1)
        sjmp    fs_ret

        ; 5. RX address finished. Subsequent bytes will be reads.

fs_5:
        mov     _i2c_state, #(fs_6 - as_1)
        sjmp    fs_ret

        ; 6. Read first byte of factory test packet.
        ;    Check for one-byte ops.

fs_6:
        mov     a, _W2DAT

        cjne    a, #0xfd, #1$           ; Check for flash data [fd] packet
        mov     _i2c_state, #(fs_9 - as_1)
        sjmp    fs_ret
1$:

        cjne    a, #0xfe, #2$           ; Check for flash reset [fe] packet
        mov     _flash_fifo_head, #FLS_FIFO_RESET
        sjmp    fs_ret
2$:

        cjne    a, #0xff, #3$           ; Check for sentinel [ff] packet
        ljmp    as_nack                 ;   End of transaction.
3$:

        anl     a, #3                   ; Enforce VRAM address limit,
        mov     _i2c_temp_1, a          ;    prepare for VRAM write
        mov     _i2c_state, #(fs_7 - as_1)
        sjmp    fs_ret

        ; 7. Read second byte of factory test VRAM write packet.

fs_7:
        mov     _i2c_temp_2, _W2DAT
        mov     _i2c_state, #(fs_8 - as_1)
        sjmp    fs_ret

        ; 8. Read third and final byte of factory test VRAM packet.
        ;
        ; This loops back to state 6 afterwards, to read as many
        ; three-byte packets as the test jig is willing to send us.

fs_8:
        mov     dpl, _i2c_temp_2
        mov     dph, _i2c_temp_1
        mov     a, _W2DAT               ; Poke byte into VRAM, read from i2c.
        movx    @dptr, a

fs_goto_6:
        mov     _i2c_state, #(fs_6 - as_1)
        sjmp    fs_ret

        ; 9. Read flash loadstream byte from factory test packet.

fs_9:

        mov     psw, #0                 ; Back to register bank 0
        push    0                       ; Save R0
        mov     a, _flash_fifo_head     ; Load the flash write pointer
        add     a, #_flash_fifo         ; Address relative to flash_fifo[]
        mov     r0, a
        mov     a, _W2DAT               ; Store byte to the FIFO
        mov     @r0, a
        pop     0                       ; Restore R0

        mov     a, _flash_fifo_head     ; Advance head pointer
        inc     a
        anl     a, #(FLS_FIFO_SIZE - 1)
        mov     _flash_fifo_head, a

        sjmp    fs_goto_6

    __endasm ;
}
