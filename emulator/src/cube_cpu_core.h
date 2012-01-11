/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*- */

/* 8051 emulator core
 *
 * Copyright (c) 2006 Jari Komppa
 * Copyright <c> 2011 Sifteo, Inc. All rights reserved.
 *
 * License for this file only:
 *
 * Permission is hereby granted, free of charge, to any person obtaining 
 * a copy of this software and associated documentation files (the 
 * "Software"), to deal in the Software without restriction, including 
 * without limitation the rights to use, copy, modify, merge, publish, 
 * distribute, sublicense, and/or sell copies of the Software, and to 
 * permit persons to whom the Software is furnished to do so, subject 
 * to the following conditions: 
 *
 * The above copyright notice and this permission notice shall be included 
 * in all copies or substantial portions of the Software. 
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS 
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, 
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE 
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER 
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING 
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS 
 * IN THE SOFTWARE. 
 *
 * (i.e. the MIT License)
 */

#ifndef _CUBE_CPU_CORE_H
#define _CUBE_CPU_CORE_H

#include <stdio.h>
#include <stdint.h>
#include "cube_cpu_irq.h"
#include "vtime.h"


namespace Cube {
namespace CPU {

NEVER_INLINE void trace_execution(em8051 *mCPU);
NEVER_INLINE void profile_tick(em8051 *mCPU);
NEVER_INLINE void timer_tick_work(em8051 *aCPU, bool tick12);

static ALWAYS_INLINE void timer_tick(em8051 *aCPU)
{
    /*
     * Examine all of the possible counter/timer clock sources.
     *
     * If no clock sources are active, exit early.
     * The timer code is slow, and we'd really rather not run it every tick.
     */

    bool tick12 = false;

    if (UNLIKELY(!--aCPU->prescaler12)) { 
        tick12 = true;
        aCPU->prescaler12 = 12;
        timer_tick_work(aCPU, true);
        
    } else if (UNLIKELY(aCPU->needTimerEdgeCheck)) {
        timer_tick_work(aCPU, false);
    }
}


static ALWAYS_INLINE void em8051_tick(em8051 *aCPU, unsigned numTicks,
                                      bool sbt, bool isProfiling, bool isTracing, bool hasBreakpoint,
                                      bool *ticked)
{
    aCPU->mTickDelay -= numTicks;
    
    /*
     * Interrupts are sent if the following cases are not true:
     *   1. interrupt of equal or higher priority is in progress (tested inside function)
     *   2. current cycle is not the final cycle of instruction (tickdelay = 0)
     *   3. the instruction in progress is RETI or any write to the IE or IP regs (TODO)
     *
     * We only check for interrupt dispatch if the corresponding flag in aCPU has been
     * set, in order to save time checking the many interrupt sources and priorities.
     *
     * If we're in interpreter mode, we dispatch interrupts between any two CPU
     * instructions, like real hardware would.
     *
     * In binary translation mode, we have to cheat a bit. If we allowed dispatch only
     * between basic block boundaries, the resulting latency variability would be far
     * too much for some of our timing-sensitive code, like the neighbor sensors. So,
     * we'll instead do better than real hardware, and dispatch interrupts on every clock
     * cycle. Since basic blocks are indivisible in the binary translated firmware, the
     * side-effects from a single basic block always happen before the interrupt is
     * delivered. But we can "virtually preempt" the basic block by storing its remaining
     * mTickDelay, and coming back to it later after return.
     */

    if (UNLIKELY(aCPU->needInterruptDispatch) && (aCPU->sbt || aCPU->mTickDelay <= 0)) {
        aCPU->needInterruptDispatch = false;
        aCPU->needInterruptDispatch = false;
        handle_interrupts(aCPU);
    }
    
    if (!aCPU->mTickDelay) {

        /*
         * Run one instruction!
         *
         * Or, in SBT mode, run one translated basic block.
         */

        unsigned pc = aCPU->mPC;
        aCPU->mPreviousPC = pc;

        if (sbt) {
            aCPU->mTickDelay = sbt_rom_code[pc](aCPU);
        } else {
            uint8_t opcode = aCPU->mCodeMem[pc];
            uint8_t operand1 = aCPU->mCodeMem[(pc + 1) & PC_MASK];
            uint8_t operand2 = aCPU->mCodeMem[(pc + 2) & PC_MASK];
            aCPU->mTickDelay = aCPU->op[aCPU->mCodeMem[pc]](aCPU, pc, opcode, operand1, operand2);
            aCPU->mPC = pc & PC_MASK;
        }
        
        if (ticked)
            *ticked = true;

        /*
         * Update profiler stats for this byte
         */

        if (UNLIKELY(isProfiling))
            profile_tick(aCPU);

        /*
         * Update parity bit
         * (Currently disabled, we don't use it)
         */

#ifdef EM8051_SUPPORT_PARITY
        {
            int v = aCPU->mSFR[REG_ACC];
            v ^= v >> 4;
            v &= 0xf;
            v = (0x6996 >> v) & 1;
            aCPU->mSFR[REG_PSW] = (aCPU->mSFR[REG_PSW] & ~PSWMASK_P) | (v * PSWMASK_P);
        }
#endif

        /*
         * Write execution trace
         */

        if (UNLIKELY(isTracing))
            trace_execution(aCPU);

        /*
         * Fire breakpoints
         */
        
        if (UNLIKELY(hasBreakpoint && aCPU->mBreakpoint == aCPU->mPC))
            except(aCPU, EXCEPTION_BREAK);
    }

    timer_tick(aCPU);
}


};  // namespace CPU
};  // namespace Cube

#endif

