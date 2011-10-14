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


namespace Cube {
namespace CPU {


static void traceExecution(em8051 *mCPU)
{
    char assembly[128];
    uint8_t bank = (mCPU->mSFR[REG_PSW] & (PSWMASK_RS0|PSWMASK_RS1)) >> PSW_RS0;

    em8051_decode(mCPU, mCPU->mPC, assembly);

    fprintf(mCPU->traceFile,
            "%10u  PC=%04x   %-35s A=%02x R%d=[%02x %02x %02x %02x-%02x %02x %02x %02x] "
            "DP=[%d %04x %04x] P=[%02x.%02x %02x.%02x %02x.%02x %02x.%02x] "
            "TMR=[%02x%02x %02x%02x %02x%02x] DBG=%02x\n",

            (unsigned) mCPU->profilerTotal,
            mCPU->mPC, assembly,
            mCPU->mSFR[REG_ACC],
            bank,
            mCPU->mData[bank*8 + 0],
            mCPU->mData[bank*8 + 1],
            mCPU->mData[bank*8 + 2],
            mCPU->mData[bank*8 + 3],
            mCPU->mData[bank*8 + 4],
            mCPU->mData[bank*8 + 5],
            mCPU->mData[bank*8 + 6],
            mCPU->mData[bank*8 + 7],
            mCPU->mSFR[REG_DPS] & 1,
            (mCPU->mSFR[REG_DPH] << 8) | mCPU->mSFR[REG_DPL],
            (mCPU->mSFR[REG_DPH1] << 8) | mCPU->mSFR[REG_DPL1],
            mCPU->mSFR[REG_P0],
            mCPU->mSFR[REG_P0DIR],
            mCPU->mSFR[REG_P1],
            mCPU->mSFR[REG_P1DIR],
            mCPU->mSFR[REG_P2],
            mCPU->mSFR[REG_P2DIR],
            mCPU->mSFR[REG_P3],
            mCPU->mSFR[REG_P3DIR],
            mCPU->mSFR[REG_TH0],
            mCPU->mSFR[REG_TL0],
            mCPU->mSFR[REG_TH1],
            mCPU->mSFR[REG_TL1],
            mCPU->mSFR[REG_TH2],
            mCPU->mSFR[REG_TL2],
            mCPU->mSFR[REG_DEBUG]);
}


static ALWAYS_INLINE void timer_tick(em8051 *aCPU)
{
    /*
     * Examine all of the possible counter/timer clock sources
     */

    bool tick12 = false;
    if (UNLIKELY(++aCPU->prescaler12 == 12)) { 
        tick12 = true;
        aCPU->prescaler12 = 0;
    }

    uint8_t nextT012 = aCPU->mSFR[PORT_T012] & (PIN_T0 | PIN_T1 | PIN_T2);
    uint8_t fallingEdges = aCPU->t012 & ~nextT012;
    aCPU->t012 = nextT012;

    /*
     * If no clock sources are active, exit early.
     * The timer code is slow, and we'd really rather not run it every tick.
     */

    if (LIKELY(tick12 == false && fallingEdges == 0))
        return;

    /*
     * Timer 0 / Timer 1
     */

    if ((aCPU->mSFR[REG_TMOD] & (TMODMASK_M0_0 | TMODMASK_M1_0)) == (TMODMASK_M0_0 | TMODMASK_M1_0))
    {
        // timer/counter 0 in mode 3

        bool increment = 0;
        
        // Check if we're run enabled
        // TODO: also run if GATE is one and INT is one (external interrupt)
        if (!(aCPU->mSFR[REG_TMOD] & TMODMASK_GATE_0) && 
            (aCPU->mSFR[REG_TCON] & TCONMASK_TR0))
        {
            // check timer / counter mode
            if (aCPU->mSFR[REG_TMOD] & TMODMASK_CT_0)
                increment = fallingEdges & PIN_T0;
            else
                increment = tick12;
        }
        if (increment)
        {
            int v = aCPU->mSFR[REG_TL0];
            v++;
            aCPU->mSFR[REG_TL0] = v & 0xff;

            // TL0 overflowed
            if (v > 0xff)
                aCPU->mSFR[REG_TCON] |= TCONMASK_TF0;
        }

        increment = false;
        
        // Check if we're run enabled
        // TODO: also run if GATE is one and INT is one (external interrupt)
        if (!(aCPU->mSFR[REG_TMOD] & TMODMASK_GATE_1) && 
            (aCPU->mSFR[REG_TCON] & TCONMASK_TR1))
        {
            // check timer / counter mode
            if (aCPU->mSFR[REG_TMOD] & TMODMASK_CT_1)
                increment = fallingEdges & PIN_T1;
            else
                increment = tick12;
        }

        if (increment)
        {
            int v = aCPU->mSFR[REG_TH0];
            v++;
            aCPU->mSFR[REG_TH0] = v & 0xff;

            // TH0 overflowed
            if (v > 0xff)
                aCPU->mSFR[REG_TCON] |= TCONMASK_TF1;
        }

    }

    /*
     * Timer 0
     */

    {        
        bool increment = 0;
        
        // Check if we're run enabled
        // TODO: also run if GATE is one and INT is one (external interrupt)
        if (!(aCPU->mSFR[REG_TMOD] & TMODMASK_GATE_0) && 
            (aCPU->mSFR[REG_TCON] & TCONMASK_TR0))
        {
            // check timer / counter mode
            if (aCPU->mSFR[REG_TMOD] & TMODMASK_CT_0)
                increment = fallingEdges & PIN_T0;
            else
                increment = tick12;
        }
        
        if (increment)
        {
            int v;

            switch (aCPU->mSFR[REG_TMOD] & (TMODMASK_M0_0 | TMODMASK_M1_0))
            {
            case 0: // 13-bit timer
                v = aCPU->mSFR[REG_TL0] & 0x1f; // lower 5 bits of TL0
                v++;
                aCPU->mSFR[REG_TL0] = (aCPU->mSFR[REG_TL0] & ~0x1f) | (v & 0x1f);
                if (v > 0x1f)
                {
                    // TL0 overflowed
                    v = aCPU->mSFR[REG_TH0];
                    v++;
                    aCPU->mSFR[REG_TH0] = v & 0xff;
                    if (v > 0xff)
                    {
                        // TH0 overflowed; set bit
                        aCPU->mSFR[REG_TCON] |= TCONMASK_TF0;
                    }
                }
                break;
            case TMODMASK_M0_0: // 16-bit timer/counter
                v = aCPU->mSFR[REG_TL0];
                v++;
                aCPU->mSFR[REG_TL0] = v & 0xff;
                if (v > 0xff)
                {
                    // TL0 overflowed
                    v = aCPU->mSFR[REG_TH0];
                    v++;
                    aCPU->mSFR[REG_TH0] = v & 0xff;
                    if (v > 0xff)
                    {
                        // TH0 overflowed; set bit
                        aCPU->mSFR[REG_TCON] |= TCONMASK_TF0;
                    }
                }
                break;
            case TMODMASK_M1_0: // 8-bit auto-reload timer
                v = aCPU->mSFR[REG_TL0];
                v++;
                aCPU->mSFR[REG_TL0] = v & 0xff;
                if (v > 0xff)
                {
                    // TL0 overflowed; reload
                    aCPU->mSFR[REG_TL0] = aCPU->mSFR[REG_TH0];
                    aCPU->mSFR[REG_TCON] |= TCONMASK_TF0;
                }
                break;
            default: // two 8-bit timers
                // TODO
                break;
            }
        }
    }

    /*
     * Timer 1
     */

    {        
        bool increment = 0;

        if (!(aCPU->mSFR[REG_TMOD] & TMODMASK_GATE_1) && 
            (aCPU->mSFR[REG_TCON] & TCONMASK_TR1))
        {
            if (aCPU->mSFR[REG_TMOD] & TMODMASK_CT_1)
                increment = fallingEdges & PIN_T1;
            else
                increment = tick12;
        }

        if (increment)
        {
            int v;

            switch (aCPU->mSFR[REG_TMOD] & (TMODMASK_M0_1 | TMODMASK_M1_1))
            {
            case 0: // 13-bit timer
                v = aCPU->mSFR[REG_TL1] & 0x1f; // lower 5 bits of TL0
                v++;
                aCPU->mSFR[REG_TL1] = (aCPU->mSFR[REG_TL1] & ~0x1f) | (v & 0x1f);
                if (v > 0x1f)
                {
                    // TL1 overflowed
                    v = aCPU->mSFR[REG_TH1];
                    v++;
                    aCPU->mSFR[REG_TH1] = v & 0xff;
                    if (v > 0xff)
                    {
                        // TH1 overflowed; set bit
                        // Only update TF1 if timer 0 is not in "mode 3"
                        if (!(aCPU->mSFR[REG_TMOD] & (TMODMASK_M0_0 | TMODMASK_M1_0)))
                            aCPU->mSFR[REG_TCON] |= TCONMASK_TF1;
                    }
                }
                break;
            case TMODMASK_M0_1: // 16-bit timer/counter
                v = aCPU->mSFR[REG_TL1];
                v++;
                aCPU->mSFR[REG_TL1] = v & 0xff;
                if (v > 0xff)
                {
                    // TL1 overflowed
                    v = aCPU->mSFR[REG_TH1];
                    v++;
                    aCPU->mSFR[REG_TH1] = v & 0xff;
                    if (v > 0xff)
                    {
                        // TH1 overflowed; set bit
                        // Only update TF1 if timer 0 is not in "mode 3"
                        if (!(aCPU->mSFR[REG_TMOD] & (TMODMASK_M0_0 | TMODMASK_M1_0)))
                            aCPU->mSFR[REG_TCON] |= TCONMASK_TF1;
                    }
                }
                break;
            case TMODMASK_M1_1: // 8-bit auto-reload timer
                v = aCPU->mSFR[REG_TL1];
                v++;
                aCPU->mSFR[REG_TL1] = v & 0xff;
                if (v > 0xff)
                {
                    // TL0 overflowed; reload
                    aCPU->mSFR[REG_TL1] = aCPU->mSFR[REG_TH1];
                    // Only update TF1 if timer 0 is not in "mode 3"
                    if (!(aCPU->mSFR[REG_TMOD] & (TMODMASK_M0_0 | TMODMASK_M1_0)))
                        aCPU->mSFR[REG_TCON] |= TCONMASK_TF1;
                }
                break;
            default: // disabled
                break;
            }
        }
    }

    /*
     * Timer 2
     *
     * XXX: Capture not implemented
     * XXX: Reload from t2ex not implemented
     */

    {        
        bool tick24 = false;

        if (tick12 && ++aCPU->prescaler24 == 2) {
            tick24 = true;
            aCPU->prescaler24 = 0;
        }

        uint8_t t2con = aCPU->mSFR[REG_T2CON];
        bool t2Clk = (t2con & 0x80) ? tick24 : tick12;

        // Timer mode

        bool increment = false;
        switch (t2con & 0x03) {
        case 0: increment = 0; break;
        case 1: increment = t2Clk; break;
        case 2: increment = fallingEdges & PIN_T2; break;
        case 3: increment = t2Clk && (nextT012 & PIN_T2); break;
        }
         
        if (increment) {
            int v = aCPU->mSFR[REG_TL2];
            v++;
            aCPU->mSFR[REG_TL2] = v & 0xff;

            if (v > 0xff) {
                // TL2 overflowed
                v = aCPU->mSFR[REG_TH2];
                v++;
                aCPU->mSFR[REG_TH2] = v & 0xff;

                if (v > 0xff) {
                    // TH2 overflowed, reload and set interrupt
                    
                    switch (t2con & 0x18) {

                    case 0x10:  
                        // Reload Mode 0
                        aCPU->mSFR[REG_TL2] = aCPU->mSFR[REG_CRCL];
                        aCPU->mSFR[REG_TH2] = aCPU->mSFR[REG_CRCH];
                        break;                 

                    case 0x18:
                        // Reload Mode 1
                        // XXX: Not implemented
                        break;
                    }
                    
                    aCPU->mSFR[REG_IRCON] |= IRCON_TF2;
                }
            }
        }
    }
}


static ALWAYS_INLINE int em8051_tick(em8051 *aCPU)
{
    int v;
    int ticked = 0;

    if (LIKELY(--aCPU->mTickDelay <= 0)) {

        /*
         * Interrupts are sent if the following cases are not true:
         *   1. interrupt of equal or higher priority is in progress (tested inside function)
         *   2. current cycle is not the final cycle of instruction (tickdelay = 0)
         *   3. the instruction in progress is RETI or any write to the IE or IP regs (TODO)
         */

        handle_interrupts(aCPU);

        /*
         * Run one instruction!
         */

        unsigned pc = aCPU->mPC & (CODE_SIZE - 1);
        aCPU->mPreviousPC = aCPU->mPC;
        aCPU->mTickDelay = aCPU->op[aCPU->mCodeMem[pc]](aCPU);
        ticked = 1;

        /*
         * XXX: The CPU can't access VirtualTime, but the profiler and tracer
         *      need a cycle count. This is what we currently use...
         */
        aCPU->profilerTotal += aCPU->mTickDelay;

        /*
         * Update profiler stats for this byte
         */

        if (aCPU->mProfileData) {
            struct profile_data *pd = &aCPU->mProfileData[pc];
            pd->total_cycles += aCPU->mTickDelay;
            if (pd->loop_prev) {
                pd->loop_cycles += aCPU->profilerTotal - pd->loop_prev;
                pd->loop_hits++;
            }
            pd->loop_prev = aCPU->profilerTotal;
        }

        /*
         * Update parity bit
         */

        v = aCPU->mSFR[REG_ACC];
        v ^= v >> 4;
        v &= 0xf;
        v = (0x6996 >> v) & 1;
        aCPU->mSFR[REG_PSW] = (aCPU->mSFR[REG_PSW] & ~PSWMASK_P) | (v * PSWMASK_P);

        /*
         * Write execution trace
         */

        if (aCPU->traceFile)
            traceExecution(aCPU);

        /*
         * Fire breakpoints
         */
        
        if (aCPU->mBreakpoint && aCPU->mBreakpoint == aCPU->mPC)
            aCPU->except(aCPU, EXCEPTION_BREAK);
    }

    timer_tick(aCPU);

    return ticked;
}


};  // namespace CPU
};  // namespace Cube

#endif

