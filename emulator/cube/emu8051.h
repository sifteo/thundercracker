/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*- */

/* 8051 emulator core
 * Copyright 2006 Jari Komppa
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

#ifndef __EMU8051_H
#define __EMU8051_H

#include <stdio.h>
#include <stdint.h>

struct em8051;

// Operation: returns number of ticks the operation should take
typedef int (*em8051operation)(struct em8051 *aCPU); 

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

struct em8051
{
    unsigned char *mCodeMem;      // 1k - 64k, must be power of 2
    int mCodeMemSize; 
    unsigned char *mExtData; // 0 - 64k, must be power of 2
    int mExtDataSize;
    unsigned char *mLowerData; // 128 bytes
    unsigned char *mUpperData; // 0 or 128 bytes; leave to NULL if none
    unsigned char *mSFR; // 128 bytes; (special function registers)
    int mPC; // Program Counter; outside memory area
    int mTickDelay; // How many ticks should we delay before continuing
    int mTimerTickDelay;
    em8051operation op[256]; // function pointers to opcode handlers
    em8051decoder dec[256]; // opcode-to-string decoder handlers    
    em8051exception except; // callback: exceptional situation occurred
    em8051sfrread sfrread; // callback: SFR register being read
    em8051sfrwrite sfrwrite; // callback: SFR register written
    em8051xread xread; // callback: external memory being read
    em8051xwrite xwrite; // callback: external memory being written

    // Profiler state
    struct profile_data *mProfilerMem;
    uint64_t profilerTotal;
    FILE *traceFile;

    uint8_t irq_count;          // Number of currently active IRQ handlers

    struct {
        // Stored register values for sanity-checking ISRs
        uint8_t a, psw, sp;

        // Priority of *this* interrupt handler
        uint8_t priority;
    } irql[NUM_IRQ_LEVELS];
};

// set the emulator into reset state. Must be called before tick(), as
// it also initializes the function pointers. aWipe tells whether to reset
// all memory to zero.
void reset(struct em8051 *aCPU, int aWipe);

// run one emulator tick, or 12 hardware clock cycles.
// returns 1 if a new operation was executed.
int tick(struct em8051 *aCPU);

// decode the next operation as character string.
// buffer must be big enough (64 bytes is very safe). 
// Returns length of opcode.
int decode(struct em8051 *aCPU, int aPosition, char *aBuffer);

// Load an intel hex format object file. Returns negative for errors.
int load_obj(struct em8051 *aCPU, char *aFilename);

// Alternate way to execute an opcode (switch-structure instead of function pointers)
int do_op(struct em8051 *aCPU);

// Internal: Pushes a value into stack
void push_to_stack(struct em8051 *aCPU, int aValue);


enum EM8051_EXCEPTION
{
    EXCEPTION_STACK,  // stack address > 127 with no upper memory, or roll over
    EXCEPTION_ACC_TO_A, // acc-to-a move operation; illegal (acc-to-acc is ok, a-to-acc is ok..)
    EXCEPTION_IRET_PSW_MISMATCH, // psw not preserved over interrupt call (doesn't care about P, F0 or UNUSED)
    EXCEPTION_IRET_SP_MISMATCH,  // sp not preserved over interrupt call
    EXCEPTION_IRET_ACC_MISMATCH, // acc not preserved over interrupt call
    EXCEPTION_ILLEGAL_OPCODE,    // for the single 'reserved' opcode in the architecture
    EXCEPTION_BUS_CONTENTION,    // Hardware bus contention
    EXCEPTION_SPI_XRUN,          // SPI FIFO overrun/underrun
    EXCEPTION_RADIO_XRUN,        // Radio FIFO overrun/underrun
    EXCEPTION_I2C,              // I2C error
};

#endif
