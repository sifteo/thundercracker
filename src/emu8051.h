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


#define NUM_IRQ_LEVELS  4

struct em8051
{
    unsigned char *mCodeMem; // 1k - 64k, must be power of 2
    int mCodeMemSize; 
    unsigned char *mExtData; // 0 - 64k, must be power of 2
    int mExtDataSize;
    unsigned char *mLowerData; // 128 bytes
    unsigned char *mUpperData; // 0 or 128 bytes; leave to NULL if none
    unsigned char *mSFR; // 128 bytes; (special function registers)
    int mPC; // Program Counter; outside memory area
    int mTickDelay; // How many ticks should we delay before continuing
    em8051operation op[256]; // function pointers to opcode handlers
    em8051decoder dec[256]; // opcode-to-string decoder handlers    
    em8051exception except; // callback: exceptional situation occurred
    em8051sfrread sfrread; // callback: SFR register being read
    em8051sfrwrite sfrwrite; // callback: SFR register written
    em8051xread xread; // callback: external memory being read
    em8051xwrite xwrite; // callback: external memory being written

    uint8_t irq_count;		// Number of currently active IRQ handlers

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
int decode(struct em8051 *aCPU, int aPosition, unsigned char *aBuffer);

// Load an intel hex format object file. Returns negative for errors.
int load_obj(struct em8051 *aCPU, char *aFilename);

// Alternate way to execute an opcode (switch-structure instead of function pointers)
int do_op(struct em8051 *aCPU);

// Internal: Pushes a value into stack
void push_to_stack(struct em8051 *aCPU, int aValue);


// SFR register locations
enum SFR_REGS
{
    REG_P0	= 0x80 - 0x80,
    REG_SP	= 0x81 - 0x80,
    REG_DPL	= 0x82 - 0x80,
    REG_DPH	= 0x83 - 0x80,
    REG_DPL1	= 0x82 - 0x80,
    REG_DPH1	= 0x83 - 0x80,


    REG_PCON	= 0x87 - 0x80,
    REG_TCON	= 0x88 - 0x80,
    REG_TMOD	= 0x89 - 0x80,
    REG_TL0	= 0x8A - 0x80,
    REG_TL1	= 0x8B - 0x80,
    REG_TH1	= 0x8D - 0x80,
    REG_TH0	= 0x8C - 0x80,
    REG_P1	= 0x90 - 0x80,
    REG_P0DIR	= 0x93 - 0x80,	// Pin directions
    REG_P1DIR	= 0x94 - 0x80,
    REG_P2DIR	= 0x95 - 0x80,
    REG_P3DIR	= 0x96 - 0x80,
    REG_S0CON	= 0x98 - 0x80,
    REG_P2	= 0xA0 - 0x80,
    REG_IEN0	= 0xA8 - 0x80,
    REG_IP0	= 0xA9 - 0x80,
    REG_P3	= 0xB0 - 0x80,
    REG_IEN1	= 0xB8 - 0x80,
    REG_IP1	= 0xB9 - 0x80,
    REG_IRCON	= 0xC0 - 0x80,
    REG_PSW	= 0xD0 - 0x80,
    REG_ACC	= 0xE0 - 0x80,
    REG_SRCON0	= 0xE4 - 0x80,	// Radio SPI Port
    REG_SRCON1	= 0xE5 - 0x80,
    REG_SRSTAT	= 0xE6 - 0x80,
    REG_SRDAT	= 0xE7 - 0x80,
    REG_RFCON	= 0xE8 - 0x80,	// Radio control
    REG_B	= 0xF0 - 0x80,
};

enum PSW_BITS
{
    PSW_P = 0,
    PSW_UNUSED = 1,
    PSW_OV = 2,
    PSW_RS0 = 3,
    PSW_RS1 = 4,
    PSW_F0 = 5,
    PSW_AC = 6,
    PSW_C = 7
};

enum PSW_MASKS
{
    PSWMASK_P = 0x01,
    PSWMASK_UNUSED = 0x02,
    PSWMASK_OV = 0x04,
    PSWMASK_RS0 = 0x08,
    PSWMASK_RS1 = 0x10,
    PSWMASK_F0 = 0x20,
    PSWMASK_AC = 0x40,
    PSWMASK_C = 0x80
};

enum IRQ_MASKS
{
    IRQM0_IFP	= (1 << 0),
    IRQM0_TF0	= (1 << 1),
    IRQM0_PFAIL	= (1 << 2),
    IRQM0_TF1	= (1 << 3),
    IRQM0_SER	= (1 << 4),
    IRQM0_TF2	= (1 << 5),
    IRQM0_EN	= (1 << 7),

    IRQM1_RFSPI	= (1 << 0),
    IRQM1_RF	= (1 << 1),
    IRQM1_SPI	= (1 << 2),
    IRQM1_WUOP	= (1 << 3),
    IRQM1_MISC	= (1 << 4),
    IRQM1_TICK	= (1 << 5),
    IRQM1_TMR2EX = (1 << 7),
};

enum IRCON_MASKS
{
    IRCON_RFSPI	= (1 << 0),
    IRCON_RF	= (1 << 1),
    IRCON_SPI	= (1 << 2),
    IRCON_WUOP	= (1 << 3),
    IRCON_MISC	= (1 << 4),
    IRCON_TICK	= (1 << 5),
    IRCON_TF2	= (1 << 6),
    IRCON_EXF2	= (1 << 7),
};

enum PT_MASKS
{
    PTMASK_PX0 = 0x01,
    PTMASK_PT0 = 0x02,
    PTMASK_PX1 = 0x04,
    PTMASK_PT1 = 0x08,
    PTMASK_PS  = 0x10,
    PTMASK_PT2 = 0x20,
    PTMASK_UNUSED1 = 0x40,
    PTMASK_UNUSED2 = 0x80
};

enum TCON_MASKS
{
    TCONMASK_IT0 = 0x01,
    TCONMASK_IE0 = 0x02,
    TCONMASK_IT1 = 0x04,
    TCONMASK_IE1 = 0x08,
    TCONMASK_TR0 = 0x10,
    TCONMASK_TF0 = 0x20,
    TCONMASK_TR1 = 0x40,
    TCONMASK_TF1 = 0x80
};

enum TMOD_MASKS
{
    TMODMASK_M0_0 = 0x01,
    TMODMASK_M1_0 = 0x02,
    TMODMASK_CT_0 = 0x04,
    TMODMASK_GATE_0 = 0x08,
    TMODMASK_M0_1 = 0x10,
    TMODMASK_M1_1 = 0x20,
    TMODMASK_CT_1 = 0x40,
    TMODMASK_GATE_1 = 0x80
};

enum IP_MASKS
{
    IPMASK_PX0 = 0x01,
    IPMASK_PT0 = 0x02,
    IPMASK_PX1 = 0x04,
    IPMASK_PT1 = 0x08,
    IPMASK_PS  = 0x10,
    IPMASK_PT2 = 0x20
};

enum EM8051_EXCEPTION
{
    EXCEPTION_STACK,  // stack address > 127 with no upper memory, or roll over
    EXCEPTION_ACC_TO_A, // acc-to-a move operation; illegal (acc-to-acc is ok, a-to-acc is ok..)
    EXCEPTION_IRET_PSW_MISMATCH, // psw not preserved over interrupt call (doesn't care about P, F0 or UNUSED)
    EXCEPTION_IRET_SP_MISMATCH,  // sp not preserved over interrupt call
    EXCEPTION_IRET_ACC_MISMATCH, // acc not preserved over interrupt call
    EXCEPTION_ILLEGAL_OPCODE,    // for the single 'reserved' opcode in the architecture
    EXCEPTION_BUS_CONTENTION,    // Hardware bus contention
};

#endif
