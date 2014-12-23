/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * Thundercracker cube firmware
 *
 * M. Elizabeth Scott <beth@sifteo.com>
 * Copyright <c> 2011 Sifteo, Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include <stdint.h>
#include <protocol.h>
#include <cube_hardware.h>

#include "radio.h"
#include "sensors.h"
#include "sensors_i2c.h"
#include "sensors_nb.h"

volatile uint8_t sensor_tick_counter;
volatile uint8_t sensor_tick_counter_high;

uint8_t nb_bits_remaining;
uint8_t nb_buffer[2];

__bit nb_tx_mode;
__bit nb_rx_mask_state0;
__bit nb_rx_mask_state1;
__bit nb_rx_mask_state2;
__bit nb_rx_mask_bit0;


static void i2c_tx_byte() __naked
{
    /*
     * Synchronous I2C write of one byte, from r0. Includes retry.
     * Error status is in W2CON_NACK_ABIT.
     */

    __asm
2$:     clr     _IR_SPI
        mov     _W2DAT, r0
1$:     jnb     _IR_SPI, 1$             ; Wait until done

        mov     a, _W2CON1              ; Read/clear status
        jnb     W2CON1_READY_ABIT, 2$   ; Retry if not ready
        ret
    __endasm ;
}

static void i2c_accel_tx(const __code uint8_t *buf)
{
    /*
     * Standalone I2C transmit, used during initialization.
     *
     * Transmit a sequence of register writes, each represented
     * by two bytes: register name and register value. The list
     * is zero-terminated, and must not be empty.
     *
     * Begins with an I2C_RESET.
     */

    buf = buf;
    __asm

        I2C_RESET()

3$:
        ; Start transaction, read address

        mov     r0, #ACCEL_ADDR_TX
        acall   _i2c_tx_byte
        jb      W2CON1_NACK_ABIT, 1$

        ; Send two bytes from list

        mov     r1, #2              ; Two bytes total
2$:
        clr     a                   ; Send next byte from *dptr
        movc    a, @a+dptr
        mov     r0, a
        inc     dptr

        acall   _i2c_tx_byte 
        jb      W2CON1_NACK_ABIT, 1$
        djnz    r1, 2$

        ; End of transaction

        orl     _W2CON0, #W2CON0_STOP
        
        clr     a                   ; Check for end of list
        movc    a, @a+dptr
        jnz     3$
        ret

        ; Error return
1$:
        orl     _W2CON0, #W2CON0_STOP

    __endasm ;
}

void sensors_init()
{
    /*
     * This is a maze of IRQs, all with very specific priority
     * assignments.  Here's a quick summary:
     *
     *  - Prio 0 (lowest)
     *
     *    Radio
     *
     *       This is a very long-running ISR. It really needs to be
     *       just above the main thread in priority, but no higher.
     *
     *  - Prio 1 
     *
     *    Accelerometer (I2C)
     *
     *       I2C is somewhat time-sensitive, since we do need to
     *       service it within a byte-period. But this is a fairly
     *       long-timescale operation compared to our other IRQs, and
     *       there's no requirement for predictable latency.
     *
     *  - Prio 2
     *
     *    Master clock (Timer 0)
     *
     *       We need to try our best to service Timer 0 with
     *       predictable latency, within several microseconds, so that
     *       we can reliably begin our neighbor transmit on-time. This
     *       is what keeps us inside the timeslot that we were
     *       assigned.
     *
     *       This isn't possible with Timer 0, since it shares a
     *       priority level with the radio! We could pick a different
     *       timebase for the master clock, but it's probably easier to
     *       thunk the RF IRQ onto a different IRQ that has a lower
     *       priority (in a different prio group). But let's wait to
     *       see if this is even a problem. So far in practice it doesn't
     *       seem to be.
     *
     *  - Prio 3 (highest)
     *
     *    Neighbor pulse counter (Timer 1)
     *
     *       Absolutely must have low and predictable latency. We use
     *       this timer to line up our bit clock with the received
     *       bit, an operation that needs to be accurate down to just
     *       a couple dozen clock cycles.
     *
     *    Neighbor bit TX/RX (Timer 2)
     *
     *       This is the timer we use to actually sample/generate bits.
     *       It's our baud clock. Timing is critical for the same reasons
     *       as above. (Luckily we should never have these two high-prio
     *       interrupts competing. They're mutually exclusive)
     *
     *    Waking from radio naps (RTC2)
     *
     *       This isn't really as critical as the other two here, but
     *       we're stuck sharing a priority level. This handler is extremely
     *       short, though, so it doesn't cost much.
     *
     * These interrupt priority levels MUST be kept in sync; always update
     * this comment, the register settings below, and the static analysis
     * annotations below if you change anything.
     */

    __asm
        ; Reset -- NOTE vector 0x0000 __start__stack main-thread
        ; TF0   -- NOTE vector 0x000b 2 irq-prio-0
        ; TF1   -- NOTE vector 0x001b 2 irq-prio-3
        ; TF2   -- NOTE vector 0x002b 2 irq-prio-3
        ; Radio -- NOTE vector 0x004b 2 irq-prio-0
        ; I2C   -- NOTE vector 0x0053 2 irq-prio-1
        ; TICK  -- NOTE vector 0x006b 2 irq-prio-3
    __endasm ;

    /*
     * A/D converter (MISC irq) Priority
     */

    IP1 |= 0x10;
    
    /*
     * I2C / 2Wire, for the accelerometer
     */

    INTEXP |= 0x04;             // Enable 2Wire IRQ -> iex3
    W2CON1 = 0;                 // Unmask interrupt, leave everything at default
    T2CON |= 0x40;              // iex3 rising edge

    {
        static const __code uint8_t init[] = {
            ACCEL_CTRL_REG1, ACCEL_REG1_NORMAL,
            ACCEL_CTRL_REG4, ACCEL_REG4_NORMAL,
            ACCEL_CTRL_REG6, ACCEL_IO_00,
            ACCEL_TEMP_CFG_REG, ACCEL_TEMP_CFG_NORMAL,
            0
        };
        i2c_accel_tx(init);
    }

    IRCON = 0;                  // Reset interrupt flag
    IP0 |= 0x04;                // Interrupt priority level 1.
    IEN_SPI_I2C = 1;            // Global enable for SPI interrupts

    /*
     * Timer 0: master sensor period
     *
     * This is set up as a 13-bit counter, fed by Cclk/12.
     * It overflows at 16000000 / 12 / (1<<13) = 162.76 Hz
     */

    TCON_TR0 = 1;               // Timer 0 running
    IEN_TF0 = 1;                // Enable Timer 0 interrupt

    /*
     * Timer 1: neighbor pulse counter
     *
     * We use Timer 1 as a pulse counter in the neighbor receiver.
     * When we're idle, waiting for a new neighbor packet, we set the
     * counter to 0xFFFF and get an interrupt when it overflows.
     * Subsequently, we check it on every bit-period to see if we
     * received any pulses during that bit's time window.
     *
     * By default, the timer will be stopped. We start it after
     * exiting transmit mode for the first time. (We only wait
     * for RX packets when we aren't transmitting)
     */     

    TMOD |= 0x50;               // Timer 1 is a 16-bit counter
    IP0 |= 0x08;                // Highest priority for TF1 interrupt
    IP1 |= 0x08;
    IEN_TF1 = 1;                // Enable TF1 interrupt

    /*
     * Timer 2: neighbor bit period timer
     *
     * This is a very fast timer that we use during active reception
     * or transmission of a neighbor packet. It accurately times out
     * bit periods that we use for generating pulses or for sampling
     * Timer 1.
     *
     * We need this to be on Timer 2, since at such a short period we
     * really get a lot of benefit from having automatic reload.
     *
     * This timer begins stopped. We start it on-demand when we need
     * to do a transmit or receive.
     */     

    CRCH = 0xff;                // Constant reload value
    CRCL = 0x100 - NB_BIT_TICKS;
    T2CON |= 0x10;              // Reload enabled
    TH2 = 0xff;                 // TL2 is set as needed, but TH2 is constant
    IP0 |= 0x20;                // Highest priority for TF2 interrupt
    IP1 |= 0x20;
    IEN_TF2_EXF2 = 1;           // Enable TF2 interrupt

    /*
     * RTC2: radio packet timestamping
     *
     * We want to initialize this along with the other timer IRQs rather
     * than in radio_init, since it isn't necessary during wake-on-RF.
     *
     * Set up RTC2 in 'external capture' mode, which timestamps all
     * incoming radio packets. Also turn on the TICK interrupt, which
     * will be dormant until we begin a Radio Nap but will then be used
     * to wake up the radio.
     */

    RTC2CON = 0x09;
    IEN_TICK = 1;
}
