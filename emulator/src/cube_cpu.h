/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*- */

/* 8051 emulator core
 *
 * Copyright 2006 Jari Komppa
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
 * emu8051.h
 * Emulator core header file
 */

#ifndef _CUBE_CPU_H
#define _CUBE_CPU_H

#include <stdio.h>
#include <stdint.h>
#include "cube_cpu_reg.h"
#include "macros.h"

namespace Cube {
namespace CPU {

struct em8051;

// Operation: returns number of ticks the operation should take
typedef int (*em8051operation)(struct em8051 *aCPU, uint8_t opcode, uint8_t operand1, uint8_t operand2);

// Decodes opcode at position, and fills the buffer with the assembler code. 
// Returns how many bytes the opcode takes.
typedef int (*em8051decoder)(struct em8051 *aCPU, int aPosition, char *aBuffer);

// Callback: some exceptional situation occurred. See EM8051_EXCEPTION enum, below
typedef void (*em8051exception)(struct em8051 *aCPU, int aCode);

// Callback: an SFR register is about to be read (not called for 'a' ops nor psw changes)
// Default is to return the value in the SFR register. Ports may act differently.
typedef int (*em8051sfrread)(struct em8051 *aCPU, int aRegister);

// Callback: an SFR register has changed (not called for 'a' ops)
// Default is to do nothing
typedef void (*em8051sfrwrite)(struct em8051 *aCPU, int aRegister);

// Callback: writing to external memory
// Default is to update external memory
// (can be used to control some peripherals)
typedef void (*em8051xwrite)(struct em8051 *aCPU, int aAddress, int aValue);

// Callback: reading from external memory
// Default is to return the value in external memory 
// (can be used to control some peripherals)
typedef int (*em8051xread)(struct em8051 *aCPU, int aAddress);

struct profile_data
{
    uint64_t total_cycles;
    uint64_t loop_cycles;
    uint64_t loop_prev;
    uint64_t loop_hits;
    uint64_t flash_idle;
};

#define NUM_IRQ_LEVELS  4


/*
 * Warning! The order of members in this structure is actually quite
 * important for performance.  We want to keep things that are
 * accessed frequently near each other, and near the beginning of the
 * structure. This means that things like the opcode table, code
 * memory, and the profiler are banished to the outerlands, while we
 * keep the register file and SFRs very close at hand.
 */

struct em8051
{
    uint8_t mData[256];
    uint8_t mSFR[128];

    int mPC;
    int mPreviousPC;
    int mTickDelay;     // How many ticks should we delay before continuing
    int mTimerTickDelay;
    int mBreakpoint;
    bool sbt;           // In static binary translation mode

    uint64_t profilerTotal;

    uint8_t irq_count;          // Number of currently active IRQ handlers
    uint8_t ifp;                // Last IFP state
    uint8_t t012;               // Last T0/1/2 state
    uint8_t prescaler12;        // 1/12 prescaler
    uint8_t prescaler24;        // 1/24 prescaler

    em8051exception except;
    em8051sfrread sfrread;
    em8051sfrwrite sfrwrite;
    void *callbackData;

    em8051operation op[256]; // function pointers to opcode handlers
    em8051decoder dec[256];  // opcode-to-string decoder handlers    

    uint8_t mExtData[XDATA_SIZE];
    uint8_t mCodeMem[CODE_SIZE];

    struct {
        // Stored register values for sanity-checking ISRs
        uint8_t a, psw, sp;

        // Priority of *this* interrupt handler
        uint8_t priority;
    } irql[NUM_IRQ_LEVELS];

    // Profiler state
    FILE *traceFile;
    struct profile_data *mProfileData;
};

// set the emulator into reset state. Must be called before tick(), as
// it also initializes the function pointers. aWipe tells whether to reset
// all memory to zero.
void em8051_reset(struct em8051 *aCPU, int aWipe);

// decode the next operation as character string.
// buffer must be big enough (64 bytes is very safe). 
// Returns length of opcode.
int em8051_decode(struct em8051 *aCPU, int aPosition, char *aBuffer);

// Load an intel hex format object file. Returns negative for errors.
int em8051_load(struct em8051 *aCPU, const char *aFilename);

// Switch to static binary translation mode
void em8051_init_sbt(struct em8051 *aCPU);

// Internal: Pushes a value into stack
void em8051_push(struct em8051 *aCPU, int aValue);

// Get a human-readable name for an exception code
const char *em8051_exc_name(int aCode);

// Private functions
void disasm_setptrs(em8051 *aCPU);
void op_setptrs(em8051 *aCPU);

enum EM8051_EXCEPTION
{
    EXCEPTION_BREAK = 0,         // user-defined breakpoint (mBreakpoint) reached
    EXCEPTION_STACK,             // stack address > 127 with no upper memory, or roll over
    EXCEPTION_ACC_TO_A,          // acc-to-a move operation; illegal (acc-to-acc is ok, a-to-acc is ok..)
    EXCEPTION_IRET_PSW_MISMATCH, // psw not preserved over interrupt call (doesn't care about P, F0 or UNUSED)
    EXCEPTION_IRET_SP_MISMATCH,  // sp not preserved over interrupt call
    EXCEPTION_IRET_ACC_MISMATCH, // acc not preserved over interrupt call
    EXCEPTION_ILLEGAL_OPCODE,    // for the single 'reserved' opcode in the architecture
    EXCEPTION_BUS_CONTENTION,    // Hardware bus contention
    EXCEPTION_SPI_XRUN,          // SPI FIFO overrun/underrun
    EXCEPTION_RADIO_XRUN,        // Radio FIFO overrun/underrun
    EXCEPTION_I2C,               // I2C error
    EXCEPTION_XDATA_ERROR,       // Access to unmapped portion of xdata 
    EXCEPTION_SBT,               // Binary translator error (executing untranslated code)
};


};  // namespace CPU
};  // namespace Cube

#endif
