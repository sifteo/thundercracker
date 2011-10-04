/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * This file is part of the internal implementation of the Sifteo SDK.
 * Confidential, not for redistribution.
 *
 * Copyright <c> 2011 Sifteo, Inc. All rights reserved.
 */

#include "systime.h"
#include "hardware.h"
#include "gpio.h"

static const unsigned SYSTICK_HZ = (48000000 / 8);
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
    NVIC.SysTick_CS = 3;
}

IRQ_HANDLER ISR_SysTick()
{
    tickBase += SysTime::hzTicks(SYSTICK_IRQ_HZ);
}

SysTime::Ticks SysTime::ticks()
{
    Ticks t = tickBase + ((SYSTICK_RELOAD - NVIC.SysTick) * hzTicks(SYSTICK_IRQ_HZ)) / SYSTICK_RELOAD;

    if (t < lastTick) {
        /*
         * Not monotonic. The timer must have rolled over, but we
         * haven't processed the rollover IRQ yet.
         */
        t += hzTicks(SYSTICK_IRQ_HZ);
    }

    lastTick = t;
    return t;
}
