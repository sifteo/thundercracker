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

// 16-bit accumulator, for integrating battery voltage
uint16_t i2c_battery_total;
uint8_t i2c_battery_count;

__bit i2c_a21_current;  // Set by main thread only. What do we want A21 to be?
__bit i2c_a21_target;   // Set by ISR only. What was A21 last updated to?
__bit i2c_a21_lock;     // Set by ISR only. Is an A21 update in progress?

__bit i2c_saved_dps;    // Store DPS in a bit variable, to save space


void read_factory_packet_header() __naked
{
    /*
     * Read the first byte of our factory test packet.
     *
     * Also, at this point, exit the disconnected state. A testjig
     * that writes commands to us counts as a connection, and resets
     * the packet deadline to the max possible (over 5 minutes)
     */
    __asm
        setb    _radio_connected

        mov     a, _sensor_tick_counter_high
        dec     a
        mov     _radio_packet_deadline, a

        mov     a, _W2DAT

        ajmp    read_factory_packet_header_ret
    __endasm ;
}


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

        mov     a, _DPS
        rrc     a
        mov     _i2c_saved_dps, c
        mov     _DPS, #0

        mov     a, _W2CON1              ; Check status of I2C engine.
        jnb     acc.0, #1$              ; Not a real I2C interrupt. Ignore it.
        jb      acc.1, #2$              ; Was not acknowledged!

        mov     dptr, #_i2c_state_fn    ; Call the state handler
        mov     a, _i2c_state
        acall   i2c_j
        mov     _i2c_state, a           ; Returns next state

        ; Return from IRQ. We get called back after the next byte finishes.

1$:

        mov     c, _i2c_saved_dps
        rlc     a
        mov     _DPS, a

        pop     dph
        pop     dpl
        pop     psw
        pop     acc
        reti

        ; NACK handler. Stop, and go to a no-op state until we are
        ; triggered again by TF2.

2$:
        orl     _W2CON0, #W2CON0_STOP  
        mov     _i2c_state, #(as_nop - _i2c_state_fn)
        sjmp    #1$

        ; Static analysis NOTE dyn_branch i2c_j [atfb]s_[0-9]
i2c_j:  jmp     @a+dptr

    __endasm ;
}


/*
 * I2C State Machine
 *
 * This works a little differently than some of the other state machines,
 * since here CPU speed is really not an issue (I2C transactions happen
 * at a relatively pokey 10 kBps) and instead we're really trying to
 * conserve code space.
 *
 * So, each state here is actually called rather than jumped to, since
 * we can return with a new state in 'a' using fewer bytes than if we
 * had to set i2c_state and sjmp.
 *
 * This state machine is VERY TIGHT. The last state label needs to be
 * within 255 bytes of the beginning of the function. There is just barely
 * enough room.
 */

#pragma sdcc_hash +
#define NEXT(_label)                                            __endasm; \
    __asm mov   a, #(_label - _i2c_state_fn)                    __endasm; \
    __asm ret                                                   __endasm; \
    __asm

void i2c_state_fn(void) __naked
{
    __asm

        ;--------------------------------------------------------------------
        ; Accelerometer States
        ;--------------------------------------------------------------------

        ; 1. TX address finished, Send register address next
as_1:
        mov     _W2DAT, #ACCEL_START_READ_X
        NEXT    (as_2)

        ; 2. Register address finished. Send repeated start, and RX address
as_2:
        orl     _W2CON0, #W2CON0_START
        mov     _W2DAT, #ACCEL_ADDR_RX
        NEXT    (as_3)

        ; 3. RX address finished. Subsequent bytes will be reads.
as_3:
        NEXT    (as_4)

        ; 4. Read (and discard) X axis low byte.
as_4:
        mov     a, _W2DAT
        NEXT    (as_5)

        ; 5. Read X axis high byte.
as_5:
        #if HWREV >= 3
                ; x axis is inverted on rev 3 hardware
                mov     a, _W2DAT
                cpl     a
                mov     _i2c_temp_1, a
        #else
                mov     _i2c_temp_1, _W2DAT
        #endif

        NEXT    (as_6)

        ; 6. Read (and discard) Y axis low byte.
as_6:
        mov     a, _W2DAT
        NEXT    (as_7)

        ; 7. Read Y axis high byte.
        ;    (Y axis is inverted on rev 2+ hardware)
as_7:
        mov     a, _W2DAT
        cpl     a
        mov     _i2c_temp_2, a
        NEXT    (as_8)

        ; 8. Read (and discard) Z axis low byte.
        ; This is the second-to-last byte, so we queue a Stop condition.
as_8:
        mov     a, _W2DAT
        orl     _W2CON0, #W2CON0_STOP
        NEXT    (as_9)

        ; 9. Read Z axis high byte, and store results
as_9:
        ljmp    _i2c_accel_store_results
i2c_accel_store_results_ret:

        ; Fall through...

        ;--------------------------------------------------------------------
        ; Battery A/D Converter States
        ;--------------------------------------------------------------------

        ; 1. Send TX address, start transaction
bs_1:
        mov     _W2DAT, #ACCEL_ADDR_TX
        NEXT    (bs_2)

        ; 2. TX address finished, Send register address next
bs_2:
        mov     _W2DAT, #ACCEL_START_READ_BAT
        NEXT    (bs_3)

        ; 3. Register address finished. Send repeated start, and RX address
bs_3:
        orl     _W2CON0, #W2CON0_START
        mov     _W2DAT, #ACCEL_ADDR_RX
        NEXT    (bs_4)

        ; 4. RX address finished. Subsequent bytes will be reads.
bs_4:
        NEXT    (bs_5)

        ; 5. Read and ignore low byte.
        ; This is the second-to-last byte, so we queue a Stop condition.
bs_5:
        mov     a, _W2DAT
        orl     _W2CON0, #W2CON0_STOP
        NEXT    (bs_6)

        ; 6. Read high byte, and store results
bs_6:
        ljmp    _i2c_battery_store_results
i2c_battery_store_results_ret:

        ; Fall through...

        ;--------------------------------------------------------------------
        ; A21 States
        ;--------------------------------------------------------------------
        
        ; Our flash memory is arranged as two banks, in order to address all
        ; of the 4MB part with only 3x7=21 general purpose address lines.
        ; This 22nd bit, A21, is wired up to what happened to be a convenient
        ; free GPIO- the INT2 pin on our accelerometer. We control the A21
        ; state by enabling or disabling an inverter on the LIS3DHs interrupt
        ; controller.
        ;
        ; This is a relatvely expensive operation that needs to be synchronized
        ; with the clients on our main thread who request it: the graphics
        ; engine and the flash HAL.
        ;
        ; We do this with two bits which indicate the _target_ state requested
        ; by the main thread, and the _current_ state that the A21 pin is in.
        ;
        ; This ISR polls both bits, and changes _current_ to match _target_
        ; if there is a mismatch. The main thread always sets _target_ to the
        ; desired state, and polls _current_ to wait for us.

        setb    _i2c_a21_lock       ; Must set lock prior to sampling current/target

        mov     c, _i2c_a21_target
        rlc     a                   ; acc.0 = target, other bits undefined
        mov     _i2c_temp_1, a      ; Latch the sampled target state in temp1
        mov     c, _i2c_a21_current
        addc    a, #0               ; Compare to current state using addc as an XOR
        jnb     acc.0, #ts_4        ; Fall through if there is no change

        mov     _W2DAT, #ACCEL_ADDR_TX
        NEXT    (ts_1)

        ; 1. TX address finished, Send register address next
ts_1:
        mov     _W2DAT, #ACCEL_CTRL_REG6
        NEXT    (ts_2)

        ; 2. Send new value for CTRL_REG6, and stop the transaction.
ts_2:
        mov     a, _i2c_temp_1      ; New target to acc.0
        rl      a
        anl     a, #ACCEL_IO_11     ; Move to bit 1, mask off unknown bits
        mov     _W2DAT, a
        orl     _W2CON0, #W2CON0_STOP
        NEXT    (ts_3)
        
        ; 3. Update i2c_a21_current after transaction is fully ended,
        ;    then fall through to factory test below.
ts_3:
        mov     a, _i2c_temp_1
        rrc     a
        mov     _i2c_a21_current, c

        ; 4. Release lock, fall through to factory test
ts_4:
        clr     _i2c_a21_lock

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
fs_2n:  NEXT    (fs_2)

        ; 2. Send an ACK byte.
        ;
        ; Note that we need to trash a register here in order to address
        ; the ACK buffer, but all of the non-test code manages to avoid
        ; touching anything other than the accumulator and DPTR. So, do a
        ; quick local save-and-restore of r0, taking care to also reset our
        ; register bank.
fs_2:
        mov     psw, #0             ; Back to register bank 0
        mov     DPL, r0             ; Save R0
        mov     r0, _i2c_temp_1     ; Send next ACK byte
        mov     a, @r0
        mov     _W2DAT, a
        mov     r0, DPL             ; Restore R0

        inc     _i2c_temp_1         ; Iterate over all ACK bytes
        djnz    _i2c_temp_2, fs_2n

        mov     _i2c_temp_1, #(PARAMS_HWID & 0xFF)
        mov     _i2c_temp_2, #HWID_LEN
fs_3n:  NEXT    (fs_3)

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
        djnz    _i2c_temp_2, fs_3n
        NEXT    (fs_4)

        ; 4. Send repeated start, and RX address
fs_4:
        orl     _W2CON0, #W2CON0_START
        mov     _W2DAT, #FACTORY_ADDR_RX
        NEXT    (fs_5)

        ; 5. RX address finished. Subsequent bytes will be reads.
fs_5:
fs_6n:
        NEXT    (fs_6)

        ; 6. Read first byte of factory test packet. Check for one-byte ops.
fs_6:
        ajmp    _read_factory_packet_header
read_factory_packet_header_ret:

        cjne    a, #0xfd, #fs_skip_fd   ; Check for flash data [fd] packet
        NEXT    (fs_9)
fs_skip_fd:

        cjne    a, #0xfe, #fs_skip_fe   ; Check for flash reset [fe] packet
        setb    _flash_reset_request
        sjmp    #fs_6n
fs_skip_fe:

        cjne    a, #0xff, #fs_skip_ff   ; Check for sentinel [ff] packet
        orl     _W2CON0, #W2CON0_STOP   ;   End of transaction.
as_nop: NEXT    (as_nop)                ;   Get stuck in a no-op state.
fs_skip_ff:

        anl     a, #3                   ; Enforce VRAM address limit,
        mov     _i2c_temp_1, a          ;    prepare for VRAM write
        NEXT    (fs_7)

        ; 7. Read second byte of factory test VRAM write packet.
fs_7:
        mov     _i2c_temp_2, _W2DAT
        NEXT    (fs_8)

        ; 8. Read third and final byte of factory test VRAM packet.
        ;
        ; This loops back to state 6 afterwards, to read as many
        ; three-byte packets as the test jig is willing to send us.
fs_8:
        mov     dpl, _i2c_temp_2
        mov     dph, _i2c_temp_1
        mov     a, _W2DAT               ; Poke byte into VRAM, read from i2c.
        movx    @dptr, a
        sjmp    fs_6n

        ; 9. Read flash loadstream byte from factory test packet.
fs_9:
        mov     psw, #0                 ; Back to register bank 0
        mov     DPL, r0                 ; Save R0
        mov     r0, _flash_fifo_head    ; Load the flash write pointer
        mov     a, _W2DAT               ; Store byte to the FIFO
        mov     @r0, a
        inc     r0                      ; Advance head pointer
        cjne    r0, #(_flash_fifo + FLS_FIFO_SIZE), 1$
        mov     r0, #_flash_fifo        ; Wrap
1$:     mov     _flash_fifo_head, r0
        mov     r0, DPL                 ; Restore R0

        sjmp    fs_6n

    __endasm ;
}


void i2c_a21_wait(void) __naked
{
    /*
     * Wait for A21 to match i2c_a21_target.
     *
     * We need to ensure 'current' and 'target' match, plus we need
     * to make sure i2c_a21_lock is 0 so we know the value of 'target'
     * has not already been latched by the ISR.
     *
     * We do this batch of tests with the IRQs disabled. The Nordic
     * docs imply it's not always safe to update W2CON1's IRQ mask
     * bit, so for now we'll just disabled all IRQs.
     */

    __asm
1$:
        clr     _IEN_EN             ; Entering critical section

        jb      _i2c_a21_lock, #2$  ; Spin if locked
        
        mov     c, _i2c_a21_target
        rlc     a                   ; acc.0 = target, other bits undefined
        mov     c, _i2c_a21_current ; Sample current state
        addc    a, #0               ; Compare, using addc as an XOR
        jb      acc.0, #2$          ; Spin if they do not match

        setb    _IEN_EN             ; Leave critical section
        ret                         ; Successfully exit

2$:     setb    _IEN_EN             ; Leave critical section
        sjmp    #1$                 ; Keep waiting

    __endasm ;
}

void i2c_battery_store_results() __naked
{
    /*
     * Store a battery voltage measurement.
     *
     * The high byte is in W2DAT. The low byte is ignored. The ADC is only 10-bit,
     * and even the high 8 bits are pretty noisy. For battery voltage, we'd much
     * prefer a slower but higher resolution result, so we integrate this noisy
     * data over time in order to get a stable 8-bit battery voltage reading.
     *
     * The voltage we sample from the accelerometer is formatted as a signed
     * value, where 0 is the middle of the range, and polarity is inverted.
     * To convert it back to an unsigned number with the proper polarity, we
     * subtract the sample from 0x7F.
     */

    __asm

        ; Convert to unsigned

        mov     a, #0x7F
        clr     c
        subb    a, _W2DAT

        ; Add to the 16-bit accumulator

        add     a, _i2c_battery_total+0
        mov     _i2c_battery_total+0, a

        mov     a, _i2c_battery_total+1
        addc    a, #0
        mov     _i2c_battery_total+1, a

        ; Roll over after 256 measurements

        djnz    _i2c_battery_count, 1$

        ; Skip hysteresis test if we have never updated the battery voltage

        mov     a, (_ack_data + RF_ACK_BATTERY_V)
        jz      4$

        ; Apply hysteresis. The reading must change by more than one LSB
        ; before we actually update it.

        mov     a, _i2c_battery_total+1
        clr     c
        subb    a, (_ack_data + RF_ACK_BATTERY_V)
        jz      3$                                      ; Skip if no delta
        dec     a
        jz      3$                                      ; Skip if delta +1
        add     a, #2
        jz      3$                                      ; Skip if delta -1
4$:

        ; Force the battery voltage to be nonzero. We reserve zero to
        ; mean we have not yet finished integrating.

        mov     a, _i2c_battery_total+1
        jnz     2$
        inc     a
2$:

        ; Update the ACK buffer

        mov     (_ack_data + RF_ACK_BATTERY_V), a
        orl     _ack_bits, #RF_ACK_BIT_BATTERY_V

3$:     ; Clear accumulator and exit

        clr     a
        mov     _i2c_battery_total+0, a
        mov     _i2c_battery_total+1, a

1$:
        ljmp    i2c_battery_store_results_ret

    __endasm ;
}

void i2c_accel_store_results() __naked
{
    /*
     * In rapid succession, store both axes and set the change flag
     * if necessary. This minimizes the chances of ever sending one old axis and
     * one new axis. In fact, since this interrupt is higher priority than the
     * RF interrupt, we are guaranteed to send synchronized updates of both axes.
     *
     * Expects the X axis to be in W2DAT, with X and Y in i2c_temp_1 and _2 respectively.
     * Returns by jumping to i2c_accel_store_results_ret.
     */

    __asm

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

        ljmp    i2c_accel_store_results_ret
    __endasm ;
}
