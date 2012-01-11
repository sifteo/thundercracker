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
 *
 * core.c
 * General emulation functions
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "tracer.h"
#include "cube_debug.h"
#include "cube_cpu.h"

namespace Cube {
namespace CPU {


NEVER_INLINE void trace_execution(em8051 *mCPU)
{
    char assembly[128];
    uint8_t bank = (mCPU->mSFR[REG_PSW] & (PSWMASK_RS0|PSWMASK_RS1)) >> PSW_RS0;

    em8051_decode(mCPU, mCPU->mPC, assembly);

    Tracer::logV(mCPU, "@%04X i%d a%02X r%d[%02X%02X%02X%02X-%02X%02X%02X%02X] "
                 "d%d[%04X%04X] p[%02X%02X%02X%02X-%02X%02X%02X%02X] "
                 "t[%02X%02X%02X%02X%02X%02X]  %s",
                 mCPU->mPC, mCPU->irq_count,
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
                 mCPU->mSFR[REG_P1],
                 mCPU->mSFR[REG_P2],
                 mCPU->mSFR[REG_P3],
                 mCPU->mSFR[REG_P0DIR],
                 mCPU->mSFR[REG_P1DIR],
                 mCPU->mSFR[REG_P2DIR],
                 mCPU->mSFR[REG_P3DIR],
                 mCPU->mSFR[REG_TH0],
                 mCPU->mSFR[REG_TL0],
                 mCPU->mSFR[REG_TH1],
                 mCPU->mSFR[REG_TL1],
                 mCPU->mSFR[REG_TH2],
                 mCPU->mSFR[REG_TL2],
                 assembly);
}

NEVER_INLINE void profile_tick(em8051 *aCPU)
{
    struct profile_data *pd = &aCPU->mProfileData[aCPU->mPreviousPC];
    pd->total_cycles += aCPU->mTickDelay;
    if (pd->loop_prev) {
        pd->loop_cycles += aCPU->vtime->clocks - pd->loop_prev;
        pd->loop_hits++;
    }
    pd->loop_prev = aCPU->vtime->clocks;
}

int em8051_decode(em8051 *aCPU, int aPosition, char *aBuffer)
{
    return aCPU->dec[aCPU->mCodeMem[aPosition & PC_MASK]](aCPU, aPosition, aBuffer);
}

void em8051_reset(em8051 *aCPU, int aWipe)
{
    // clear memory, set registers to bootup values, etc    
    if (aWipe)
    {
        memset(aCPU->mCodeMem, 0, sizeof aCPU->mCodeMem);
        memset(aCPU->mExtData, 0, sizeof aCPU->mExtData);
        memset(aCPU->mData, 0, sizeof aCPU->mData);
    }

    memset(aCPU->mSFR, 0, 128);

    aCPU->mPC = 0;
    aCPU->mTickDelay = 1;
    aCPU->prescaler12 = 12;

    aCPU->mSFR[REG_SP] = 7;
    aCPU->mSFR[REG_P0] = 0xff;
    aCPU->mSFR[REG_P1] = 0xff;
    aCPU->mSFR[REG_P2] = 0xff;
    aCPU->mSFR[REG_P3] = 0xff;
    
    aCPU->mSFR[REG_P0DIR] = 0xFF;
    aCPU->mSFR[REG_P1DIR] = 0xFF;
    aCPU->mSFR[REG_P2DIR] = 0xFF;
    aCPU->mSFR[REG_P3DIR] = 0xFF;
    
    aCPU->mSFR[REG_SPIRCON0] = 0x01;
    aCPU->mSFR[REG_SPIRCON1] = 0x0F;
    aCPU->mSFR[REG_SPIRSTAT] = 0x03;
    aCPU->mSFR[REG_SPIRDAT] = 0x00;
    aCPU->mSFR[REG_RFCON] = RFCON_RFCSN;
    
    // build function pointer lists

    disasm_setptrs(aCPU);
    op_setptrs(aCPU);

    // Clean internal variables
    aCPU->irq_count = 0;
    aCPU->needInterruptDispatch = false;
}

static int readbyte(FILE * f)
{
    char data[3];
    data[0] = fgetc(f);
    data[1] = fgetc(f);
    data[2] = 0;
    return strtol(data, NULL, 16);
}

void em8051_init_sbt(struct em8051 *aCPU)
{
    /*
     * No firmware? Use our built in statically binary-translated
     * firmware.  We have a ROM image containing only the data
     * portions of the firmware, and we have a replacement opcode
     * exec function which executes translated basic-blocks.
     */
    memcpy(aCPU->mCodeMem, sbt_rom_data, sizeof aCPU->mCodeMem);
    aCPU->sbt = true;
}

int em8051_load(em8051 *aCPU, const char *aFilename)
{
    FILE *f;    
    if (aFilename == 0 || aFilename[0] == 0)
        return -1;
    f = fopen(aFilename, "r");
    if (!f) return -1;
    if (fgetc(f) != ':')
        return -2; // unsupported file format
    while (!feof(f))
    {
        int recordlength;
        int address;
        int recordtype;
        int checksum;
        int i;
        recordlength = readbyte(f);
        address = readbyte(f);
        address <<= 8;
        address |= readbyte(f);
        recordtype = readbyte(f);
        if (recordtype == 1)
            return 0; // we're done
        if (recordtype != 0)
            return -3; // unsupported record type
        checksum = recordtype + recordlength + (address & 0xff) + (address >> 8); // final checksum = 1 + not(checksum)
        for (i = 0; i < recordlength; i++)
        {
            int data = readbyte(f);
            checksum += data;
            aCPU->mCodeMem[address + i] = data;
        }
        i = readbyte(f);
        checksum &= 0xff;
        checksum = 256 - checksum;
        if (i != (checksum & 0xff))
            return -4; // checksum failure
        while (fgetc(f) != ':' && !feof(f)) {} // skip newline        
    }
    return -5;
}

const char *em8051_exc_name(int aCode)
{
    static const char *exc_names[] = {
        "Breakpoint reached",
        "SP exception: stack address > 127",
        "Invalid operation: acc-to-a move",
        "PSW not preserved over interrupt call",
        "SP not preserved over interrupt call",
        "ACC not preserved over interrupt call",
        "DP* not preserved over interrupt call",
        "R0-R7 not preserved over interrupt call",
        "Invalid opcode: 0xA5 encountered",
        "Hardware bus contention occurred",
        "SPI FIFO overrun/underrun",
        "Radio FIFO overrun/underrun",
        "I2C error",
        "XDATA error",
        "Binary translator error",
        "MDU error",
        "RNG error",
        "Nonvolatile memory write error",
    };

    if (aCode < (int)(sizeof exc_names / sizeof exc_names[0]))
        return exc_names[aCode];
    else
        return "Unknown exception";
}


NEVER_INLINE void timer_tick_work(em8051 *aCPU, bool tick12)
{
    /*
     * Check T1/2/3 edges only when prompted via aCPU->needTimerEdgeCheck.
     */

    uint8_t nextT012 = aCPU->mSFR[PORT_T012] & (PIN_T0 | PIN_T1 | PIN_T2);
    uint8_t fallingEdges = aCPU->t012 & ~nextT012;
    aCPU->t012 = nextT012;
    aCPU->needTimerEdgeCheck = false;
    
    /*
     * Neighbor input pulses are simulated as instantaneous events
     * less than one clock cycle in duration. Immediately clear the
     * neighbor inputs after we've sampled a new t012 value.
     */
 
    Neighbors::clearNeighborInput(*aCPU);
    
    
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
            if (v > 0xff) {
                aCPU->mSFR[REG_TCON] |= TCONMASK_TF0;
                aCPU->needInterruptDispatch = true;
            }
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
            if (v > 0xff) {
                aCPU->mSFR[REG_TCON] |= TCONMASK_TF1;
                aCPU->needInterruptDispatch = true;
            }
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
                        aCPU->needInterruptDispatch = true;
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
                        aCPU->needInterruptDispatch = true;
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
                    aCPU->needInterruptDispatch = true;
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
                        if (!(aCPU->mSFR[REG_TMOD] & (TMODMASK_M0_0 | TMODMASK_M1_0))) {
                            aCPU->mSFR[REG_TCON] |= TCONMASK_TF1;
                            aCPU->needInterruptDispatch = true;
                        }
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
                        if (!(aCPU->mSFR[REG_TMOD] & (TMODMASK_M0_0 | TMODMASK_M1_0))) {
                            aCPU->mSFR[REG_TCON] |= TCONMASK_TF1;
                            aCPU->needInterruptDispatch = true;
                        }
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
                    if (!(aCPU->mSFR[REG_TMOD] & (TMODMASK_M0_0 | TMODMASK_M1_0))) {
                        aCPU->mSFR[REG_TCON] |= TCONMASK_TF1;
                        aCPU->needInterruptDispatch = true;
                    }
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
        case 3: increment = t2Clk && (aCPU->t012 & PIN_T2); break;
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
                    aCPU->needInterruptDispatch = true;
                }
            }
        }
    }
}


};  // namespace CPU
};  // namespace Cube

