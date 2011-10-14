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

#include "cube_debug.h"
#include "cube_cpu.h"

namespace Cube {
namespace CPU {


int em8051_decode(em8051 *aCPU, int aPosition, char *aBuffer)
{
    return aCPU->dec[aCPU->mCodeMem[aPosition & (CODE_SIZE - 1)]](aCPU, aPosition, aBuffer);
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
    aCPU->mTickDelay = 0;
    aCPU->mSFR[REG_SP] = 7;
    aCPU->mSFR[REG_P0] = 0xff;
    aCPU->mSFR[REG_P1] = 0xff;
    aCPU->mSFR[REG_P2] = 0xff;
    aCPU->mSFR[REG_P3] = 0xff;

    // build function pointer lists

    disasm_setptrs(aCPU);
    op_setptrs(aCPU);

    // Clean internal variables
    aCPU->irq_count = 0;
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
        "Invalid opcode: 0xA5 encountered",
        "Hardware bus contention occurred",
        "SPI FIFO overrun/underrun",
        "Radio FIFO overrun/underrun",
        "I2C error",
        "XDATA error",
        "Binary translator error",
    };

    if (aCode < (int)(sizeof exc_names / sizeof exc_names[0]))
        return exc_names[aCode];
    else
        return "Unknown exception";
}


};  // namespace CPU
};  // namespace Cube

