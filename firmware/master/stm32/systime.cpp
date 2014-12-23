/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * Thundercracker firmware
 *
 * Copyright <c> 2012 Sifteo, Inc.
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

#include "systime.h"
#include "hardware.h"
#include "gpio.h"
#include "tasks.h"

static const unsigned SYSTICK_HZ = (72000000 / 8);
static const unsigned SYSTICK_IRQ_HZ = 10;
static const uint32_t SYSTICK_RELOAD = (SYSTICK_HZ / SYSTICK_IRQ_HZ) - 1;

/*
 * tickBase --
 *    Coarse timer, incremented once per rollover. Written only by the ISR. Read by anyone.
 *
 * lastTick --
 *    Last value returned from ticks(). Not used from interrupt context.
 */
static SysTime::Ticks tickBase, lastTick;


void SysTime::init()
{
    /*
     * The Cortex-M3 has a 24-bit hardware cycle timer, with an
     * interrupt and programmable reload.
     *
     * To use this timer to create a global monotonic timebase, we set
     * it up to roll over at a convenient rate (10Hz).  Those
     * rollovers increment a global counter, which we then combine
     * with the current timer value in ticks().
     */

    tickBase = 0;
    lastTick = 0;

    NVIC.SysTick_CS = 0;
    NVIC.SysTick_RELOAD = SYSTICK_RELOAD;
    NVIC.SysTick = 0;

    // Enable timer, enable interrupt
    // NOTE: NVIC.SysTick gets loaded with NVIC.SysTick_RELOAD when 'enable' is applied
    NVIC.SysTick_CS = 3;
}

IRQ_HANDLER ISR_SysTick()
{
    tickBase += SysTime::hzTicks(SYSTICK_IRQ_HZ);

    STATIC_ASSERT(SYSTICK_IRQ_HZ == Tasks::HEARTBEAT_HZ);
#ifndef RFTEST
    Tasks::heartbeatISR();
#endif
}

SysTime::Ticks SysTime::ticks()
{
    /*
     * Fractional part of our timebase (between IRQs)
     */
    uint32_t fractional = SYSTICK_RELOAD - NVIC.SysTick;
    Ticks t = tickBase;

    /*
     * We need to multiply 'fractional' by (hzTicks(SYSTICK_IRQ_HZ)) /
     * SYSTICK_RELOAD), in order to convert it from SysTick units to
     * Ticks (nanoseconds). Any integer approximation of this number
     * would give unacceptable error, and even any 32-bit fixed point
     * approximation.  So, we'll do a 32x64 fixed-point multiply. This
     * avoids a costly division, and keeps us on operations that the
     * Cortex-M3 implements in hardware.
     */

    t += (fractional * (uint64_t)(hzTicks(SYSTICK_IRQ_HZ) * 0x100000000ULL / SYSTICK_RELOAD)) >> 32;

    /*
     * Not monotonic? The timer must have rolled over, but we haven't
     * processed the rollover IRQ yet.
     */

    if (t < lastTick) {
        t += hzTicks(SYSTICK_IRQ_HZ);
    }
    lastTick = t;

    return t;
}
