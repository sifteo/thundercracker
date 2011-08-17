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
 * opcodes.c
 * 8051 opcode simulation functions
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "emu8051.h"

#define BAD_VALUE 0x77
#define PSW aCPU->mSFR[REG_PSW]
#define ACC aCPU->mSFR[REG_ACC]
#define PC aCPU->mPC
#define OPCODE aCPU->mCodeMem[(PC + 0)&(aCPU->mCodeMemSize-1)]
#define OPERAND1 aCPU->mCodeMem[(PC + 1)&(aCPU->mCodeMemSize-1)]
#define OPERAND2 aCPU->mCodeMem[(PC + 2)&(aCPU->mCodeMemSize-1)]
#define INDIR_RX_ADDRESS (aCPU->mLowerData[(OPCODE & 1) + 8 * ((PSW & (PSWMASK_RS0|PSWMASK_RS1))>>PSW_RS0)])
#define RX_ADDRESS ((OPCODE & 7) + 8 * ((PSW & (PSWMASK_RS0|PSWMASK_RS1))>>PSW_RS0))
#define CARRY ((PSW & PSWMASK_C) >> PSW_C)

static int read_mem(struct em8051 *aCPU, int aAddress)
{
    if (aAddress > 0x7f)
    {
        if (aCPU->sfrread)
            return aCPU->sfrread(aCPU, aAddress);
        else
            return aCPU->mSFR[aAddress - 0x80];
    }
    else
    {
        return aCPU->mLowerData[aAddress];
    }
}

void push_to_stack(struct em8051 *aCPU, int aValue)
{
    aCPU->mSFR[REG_SP]++;
    if (aCPU->mSFR[REG_SP] > 0x7f)
    {
        if (aCPU->mUpperData)
        {
            aCPU->mUpperData[aCPU->mSFR[REG_SP] - 0x80] = aValue;
        }
        else
        {
            if (aCPU->except)
                aCPU->except(aCPU, EXCEPTION_STACK);
        }
    }
    else
    {
        aCPU->mLowerData[aCPU->mSFR[REG_SP]] = aValue;
    }
    if (aCPU->mSFR[REG_SP] == 0)
        if (aCPU->except)
            aCPU->except(aCPU, EXCEPTION_STACK);
}

static int pop_from_stack(struct em8051 *aCPU)
{
    int value = BAD_VALUE;
    if (aCPU->mSFR[REG_SP] > 0x7f)
    {
        if (aCPU->mUpperData)
        {
            value = aCPU->mUpperData[aCPU->mSFR[REG_SP] - 0x80];
        }
        else
        {
            if (aCPU->except)
                aCPU->except(aCPU, EXCEPTION_STACK);
        }
    }
    else
    {
        value = aCPU->mLowerData[aCPU->mSFR[REG_SP]];
    }
    aCPU->mSFR[REG_SP]--;

    if (aCPU->mSFR[REG_SP] == 0xff)
        if (aCPU->except)
            aCPU->except(aCPU, EXCEPTION_STACK);
    return value;
}


static void add_solve_flags(struct em8051 * aCPU, int value1, int value2, int acc)
{
    /* Carry: overflow from 7th bit to 8th bit */
    int carry = ((value1 & 255) + (value2 & 255) + acc) >> 8;
    
    /* Auxiliary carry: overflow from 3th bit to 4th bit */
    int auxcarry = ((value1 & 7) + (value2 & 7) + acc) >> 3;
    
    /* Overflow: overflow from 6th or 7th bit, but not both */
    int overflow = (((value1 & 127) + (value2 & 127) + acc) >> 7)^carry;
    
    PSW = (PSW & ~(PSWMASK_C | PSWMASK_AC | PSWMASK_OV)) |
          (carry << PSW_C) | (auxcarry << PSW_AC) | (overflow << PSW_OV);
}

static void sub_solve_flags(struct em8051 * aCPU, int value1, int value2)
{
    int carry = (((value1 & 255) - (value2 & 255)) >> 8) & 1;
    int auxcarry = (((value1 & 7) - (value2 & 7)) >> 3) & 1;
    int overflow = ((((value1 & 127) - (value2 & 127)) >> 7) & 1)^carry;
    PSW = (PSW & ~(PSWMASK_C|PSWMASK_AC|PSWMASK_OV)) |
                          (carry << PSW_C) | (auxcarry << PSW_AC) | (overflow << PSW_OV);
}


static int ajmp_offset(struct em8051 *aCPU)
{
    int address = (PC + 2) & 0xf800 |
                  OPERAND1 | 
                  ((OPCODE & 0xe0) << 3);

    PC = address;

    return 3;
}

static int ljmp_address(struct em8051 *aCPU)
{
    int address = (OPERAND1 << 8) | OPERAND2;
    PC = address;

    return 4;
}


static int rr_a(struct em8051 *aCPU)
{
    ACC = (ACC >> 1) | (ACC << 7);
    PC++;
    return 1;
}

static int inc_a(struct em8051 *aCPU)
{
    ACC++;
    PC++;
    return 1;
}

static int inc_mem(struct em8051 *aCPU)
{
    int address = OPERAND1;
    if (address > 0x7f)
    {
        aCPU->mSFR[address - 0x80]++;
        if (aCPU->sfrwrite)
            aCPU->sfrwrite(aCPU, address);
    }
    else
    {
        aCPU->mLowerData[address]++;
    }
    PC += 2;
    return 3;
}

static int inc_indir_rx(struct em8051 *aCPU)
{    
    int address = INDIR_RX_ADDRESS;
    if (address > 0x7f)
    {
        if (aCPU->mUpperData)
        {
            aCPU->mUpperData[address - 0x80]++;
        }
    }
    else
    {
        aCPU->mLowerData[address]++;
    }
    PC++;
    return 3;
}

static int jbc_bitaddr_offset(struct em8051 *aCPU)
{
    // "Note: when this instruction is used to test an output pin, the value used 
    // as the original data will be read from the output data latch, not the input pin"
    int address = OPERAND1;
    if (address > 0x7f)
    {
        int bit = address & 7;
        int bitmask = (1 << bit);
        int value;
        address &= 0xf8;        
        value = aCPU->mSFR[address - 0x80];
        
        if (value & bitmask)
        {
            aCPU->mSFR[address - 0x80] &= ~bitmask;
            PC += (signed char)OPERAND2 + 3;
            if (aCPU->sfrwrite)
                aCPU->sfrwrite(aCPU, address);
        }
        else
        {
            PC += 3;
        }
    }
    else
    {
        int bit = address & 7;
        int bitmask = (1 << bit);
        address >>= 3;
        address += 0x20;
        if (aCPU->mLowerData[address] & bitmask)
        {
            aCPU->mLowerData[address] &= ~bitmask;
            PC += (signed char)OPERAND2 + 3;
        }
        else
        {
            PC += 3;
        }
    }
    return 4;
}

static int acall_offset(struct em8051 *aCPU)
{
    int address = (PC + 2) & 0xf800 | OPERAND1 | ((OPCODE & 0xe0) << 3);
    push_to_stack(aCPU, (PC + 2) & 0xff);
    push_to_stack(aCPU, (PC + 2) >> 8);
    PC = address;
    return 6;
}

static int lcall_address(struct em8051 *aCPU)
{
    push_to_stack(aCPU, (PC + 3) & 0xff);
    push_to_stack(aCPU, (PC + 3) >> 8);
    PC = (aCPU->mCodeMem[(PC + 1) & (aCPU->mCodeMemSize-1)] << 8) | 
         (aCPU->mCodeMem[(PC + 2) & (aCPU->mCodeMemSize-1)] << 0);
    return 6;
}

static int rrc_a(struct em8051 *aCPU)
{
    int c = (PSW & PSWMASK_C) >> PSW_C;
    int newc = ACC & 1;
    ACC = (ACC >> 1) | (c << 7);
    PSW = (PSW & ~PSWMASK_C) | (newc << PSW_C);
    PC++;
    return 1;
}

static int dec_a(struct em8051 *aCPU)
{
    ACC--;
    PC++;
    return 1;
}

static int dec_mem(struct em8051 *aCPU)
{
    int address = OPERAND1;
    if (address > 0x7f)
    {
        aCPU->mSFR[address - 0x80]--;
        if (aCPU->sfrwrite)
            aCPU->sfrwrite(aCPU, address);
    }
    else
    {
        aCPU->mLowerData[address]--;
    }
    PC += 2;
    return 3;
}

static int dec_indir_rx(struct em8051 *aCPU)
{
    int address = INDIR_RX_ADDRESS;
    if (address > 0x7f)
    {
        if (aCPU->mUpperData)
        {
            aCPU->mUpperData[address - 0x80]--;
        }
    }
    else
    {
        aCPU->mLowerData[address]--;
    }
    PC++;
    return 3;
}


static int jb_bitaddr_offset(struct em8051 *aCPU)
{
    int address = OPERAND1;
    if (address > 0x7f)
    {
        int bit = address & 7;
        int bitmask = (1 << bit);
        int value;
        address &= 0xf8;        
        if (aCPU->sfrread)
            value = aCPU->sfrread(aCPU, address);
        else
            value = aCPU->mSFR[address - 0x80];
        
        if (value & bitmask)
        {
            PC += (signed char)OPERAND2 + 3;
        }
        else
        {
            PC += 3;
        }
    }
    else
    {
        int bit = address & 7;
        int bitmask = (1 << bit);
        address >>= 3;
        address += 0x20;
        if (aCPU->mLowerData[address] & bitmask)
        {
            PC += (signed char)OPERAND2 + 3;
        }
        else
        {
            PC += 3;
        }
    }
    return 4;
}

static int ret(struct em8051 *aCPU)
{
    PC = pop_from_stack(aCPU) << 8;
    PC |= pop_from_stack(aCPU);
    return 4;
}

static int rl_a(struct em8051 *aCPU)
{
    ACC = (ACC << 1) | (ACC >> 7);
    PC++;
    return 1;
}

static int add_a_imm(struct em8051 *aCPU)
{
    add_solve_flags(aCPU, ACC, OPERAND1, 0);
    ACC += OPERAND1;
    PC += 2;
    return 2;
}

static int add_a_mem(struct em8051 *aCPU)
{
    int value = read_mem(aCPU, OPERAND1);
    add_solve_flags(aCPU, ACC, value, 0);
    ACC += value;
	PC += 2;
    return 2;
}

static int add_a_indir_rx(struct em8051 *aCPU)
{
    int address = INDIR_RX_ADDRESS;
    if (address > 0x7f)
    {
        int value;
        if (aCPU->mUpperData)
        {
            value = aCPU->mUpperData[address - 0x80];
        }

        add_solve_flags(aCPU, ACC, value, 0);
        ACC += value;
    }
    else
    {
        add_solve_flags(aCPU, ACC, aCPU->mLowerData[address], 0);
        ACC += aCPU->mLowerData[address];
    }

    PC++;
    return 2;
}

static int jnb_bitaddr_offset(struct em8051 *aCPU)
{
    int address = OPERAND1;
    if (address > 0x7f)
    {
        int bit = address & 7;
        int bitmask = (1 << bit);
        int value;
        address &= 0xf8;        
        if (aCPU->sfrread)
            value = aCPU->sfrread(aCPU, address);
        else
            value = aCPU->mSFR[address - 0x80];
        
        if (!(value & bitmask))
        {
            PC += (signed char)OPERAND2 + 3;
        }
        else
        {
            PC += 3;
        }
    }
    else
    {
        int bit = address & 7;
        int bitmask = (1 << bit);
        address >>= 3;
        address += 0x20;
        if (!(aCPU->mLowerData[address] & bitmask))
        {
            PC += (signed char)OPERAND2 + 3;
        }
        else
        {
            PC += 3;
        }
    }
    return 4;
}

static int reti(struct em8051 *aCPU)
{
    if (aCPU->mInterruptActive)
    {
        if (aCPU->except)
        {
            int hi = 0;
            if (aCPU->mInterruptActive > 1)
                hi = 1;
            if (aCPU->int_a[hi] != aCPU->mSFR[REG_ACC])
                aCPU->except(aCPU, EXCEPTION_IRET_ACC_MISMATCH);
            if (aCPU->int_sp[hi] != aCPU->mSFR[REG_SP])
                aCPU->except(aCPU, EXCEPTION_IRET_SP_MISMATCH);    
            if ((aCPU->int_psw[hi] & (PSWMASK_OV | PSWMASK_RS0 | PSWMASK_RS1 | PSWMASK_AC | PSWMASK_C)) !=                 
                (aCPU->mSFR[REG_PSW] & (PSWMASK_OV | PSWMASK_RS0 | PSWMASK_RS1 | PSWMASK_AC | PSWMASK_C)))
                aCPU->except(aCPU, EXCEPTION_IRET_PSW_MISMATCH);
        }

        if (aCPU->mInterruptActive & 2)
            aCPU->mInterruptActive &= ~2;
        else
            aCPU->mInterruptActive = 0;
    }

    PC = pop_from_stack(aCPU) << 8;
    PC |= pop_from_stack(aCPU);
    return 4;
}

static int rlc_a(struct em8051 *aCPU)
{
    int c = CARRY;
    int newc = ACC >> 7;
    ACC = (ACC << 1) | c;
    PSW = (PSW & ~PSWMASK_C) | (newc << PSW_C);
    PC++;
    return 1;
}

static int addc_a_imm(struct em8051 *aCPU)
{
    int carry = CARRY;
    add_solve_flags(aCPU, ACC, OPERAND1, carry);
    ACC += OPERAND1 + carry;
    PC += 2;
    return 2;
}

static int addc_a_mem(struct em8051 *aCPU)
{
    int carry = CARRY;
    int value = read_mem(aCPU, OPERAND1);
    add_solve_flags(aCPU, ACC, value, carry);
    ACC += value + carry;
    PC += 2;
    return 2;
}

static int addc_a_indir_rx(struct em8051 *aCPU)
{
    int carry = CARRY;
    int address = INDIR_RX_ADDRESS;
    if (address > 0x7f)
    {
        int value = BAD_VALUE;
        if (aCPU->mUpperData)
        {
            value = aCPU->mUpperData[address - 0x80];
        }

        add_solve_flags(aCPU, ACC, value, carry);
        ACC += value + carry;
    }
    else
    {
        add_solve_flags(aCPU, ACC, aCPU->mLowerData[address], carry);
        ACC += aCPU->mLowerData[address] + carry;
    }
    PC++;
    return 2;
}


static int jc_offset(struct em8051 *aCPU)
{
    if (PSW & PSWMASK_C)
    {
        PC += (signed char)OPERAND1 + 2;
    }
    else
    {
        PC += 2;
    }
    return 3;
}

static int orl_mem_a(struct em8051 *aCPU)
{
    int address = OPERAND1;
    if (address > 0x7f)
    {
        aCPU->mSFR[address - 0x80] |= ACC;
        if (aCPU->sfrwrite)
            aCPU->sfrwrite(aCPU, address);
    }
    else
    {
        aCPU->mLowerData[address] |= ACC;
    }
    PC += 2;
    return 3;
}

static int orl_mem_imm(struct em8051 *aCPU)
{
    int address = OPERAND1;
    if (address > 0x7f)
    {
        aCPU->mSFR[address - 0x80] |= OPERAND2;
        if (aCPU->sfrwrite)
            aCPU->sfrwrite(aCPU, address);
    }
    else
    {
        aCPU->mLowerData[address] |= OPERAND2;
    }
    
    PC += 3;
    return 4;
}

static int orl_a_imm(struct em8051 *aCPU)
{
    ACC |= OPERAND1;
    PC += 2;
    return 2;
}

static int orl_a_mem(struct em8051 *aCPU)
{
    int value = read_mem(aCPU, OPERAND1);
    ACC |= value;
    PC += 2;
    return 2;
}

static int orl_a_indir_rx(struct em8051 *aCPU)
{
    int address = INDIR_RX_ADDRESS;
    if (address > 0x7f)
    {
        int value = BAD_VALUE;
        if (aCPU->mUpperData)
        {
            value = aCPU->mUpperData[address - 0x80];
        }

        ACC |= value;
    }
    else
    {
        ACC |= aCPU->mLowerData[address];
    }

    PC++;
    return 2;
}


static int jnc_offset(struct em8051 *aCPU)
{
    if (PSW & PSWMASK_C)
    {
        PC += 2;
    }
    else
    {
        PC += (signed char)OPERAND1 + 2;
    }
    return 3;
}

static int anl_mem_a(struct em8051 *aCPU)
{
    int address = OPERAND1;
    if (address > 0x7f)
    {
        aCPU->mSFR[address - 0x80] &= ACC;
        if (aCPU->sfrwrite)
            aCPU->sfrwrite(aCPU, address);
    }
    else
    {
        aCPU->mLowerData[address] &= ACC;
    }
    PC += 2;
    return 1;
}

static int anl_mem_imm(struct em8051 *aCPU)
{
    int address = OPERAND1;
    if (address > 0x7f)
    {
        aCPU->mSFR[address - 0x80] &= OPERAND2;
        if (aCPU->sfrwrite)
            aCPU->sfrwrite(aCPU, address);
    }
    else
    {
        aCPU->mLowerData[address] &= OPERAND2;
    }
    PC += 3;
    return 4;
}

static int anl_a_imm(struct em8051 *aCPU)
{
    ACC &= OPERAND1;
    PC += 2;
    return 2;
}

static int anl_a_mem(struct em8051 *aCPU)
{
    int value = read_mem(aCPU, OPERAND1);
    ACC &= value;
    PC += 2;
    return 2;
}

static int anl_a_indir_rx(struct em8051 *aCPU)
{
    int address = INDIR_RX_ADDRESS;
    if (address > 0x7f)
    {
        int value = BAD_VALUE;
        if (aCPU->mUpperData)
        {
            value = aCPU->mUpperData[address - 0x80];
        }

        ACC &= value;
    }
    else
    {
        ACC &= aCPU->mLowerData[address];
    }
    PC++;
    return 2;
}


static int jz_offset(struct em8051 *aCPU)
{
    if (!ACC)
    {
        PC += (signed char)OPERAND1 + 2;
    }
    else
    {
        PC += 2;
    }
    return 3;
}

static int xrl_mem_a(struct em8051 *aCPU)
{
    int address = OPERAND1;
    if (address > 0x7f)
    {
        aCPU->mSFR[address - 0x80] ^= ACC;
        if (aCPU->sfrwrite)
            aCPU->sfrwrite(aCPU, address);
    }
    else
    {
        aCPU->mLowerData[address] ^= ACC;
    }
    PC += 2;
    return 3;
}

static int xrl_mem_imm(struct em8051 *aCPU)
{
    int address = OPERAND1;
    if (address > 0x7f)
    {
        aCPU->mSFR[address - 0x80] ^= OPERAND2;
        if (aCPU->sfrwrite)
            aCPU->sfrwrite(aCPU, address);
    }
    else
    {
        aCPU->mLowerData[address] ^= OPERAND2;
    }
    PC += 3;
    return 4;
}

static int xrl_a_imm(struct em8051 *aCPU)
{
    ACC ^= OPERAND1;
    PC += 2;
    return 2;
}

static int xrl_a_mem(struct em8051 *aCPU)
{
    int value = read_mem(aCPU, OPERAND1);
    ACC ^= value;
    PC += 2;
    return 2;
}

static int xrl_a_indir_rx(struct em8051 *aCPU)
{
    int address = INDIR_RX_ADDRESS;
    if (address > 0x7f)
    {
        int value = BAD_VALUE;
        if (aCPU->mUpperData)
        {
            value = aCPU->mUpperData[address - 0x80];
        }

        ACC ^= value;
    }
    else
    {
        ACC ^= aCPU->mLowerData[address];
    }
    PC++;
    return 2;
}


static int jnz_offset(struct em8051 *aCPU)
{
    if (ACC)
    {
        PC += (signed char)OPERAND1 + 2;
    }
    else
    {
        PC += 2;
    }
    return 3;
}

static int orl_c_bitaddr(struct em8051 *aCPU)
{
    int address = OPERAND1;
    int carry = CARRY;
    if (address > 0x7f)
    {
        int bit = address & 7;
        int bitmask = (1 << bit);
        int value;
        address &= 0xf8;        
        if (aCPU->sfrread)
            value = aCPU->sfrread(aCPU, address);
        else
            value = aCPU->mSFR[address - 0x80];

        value = (value & bitmask) ? 1 : carry;

        PSW = (PSW & ~PSWMASK_C) | (PSWMASK_C * value);
    }
    else
    {
        int bit = address & 7;
        int bitmask = (1 << bit);
        int value;
        address >>= 3;
        address += 0x20;
        value = (aCPU->mLowerData[address] & bitmask) ? 1 : carry;
        PSW = (PSW & ~PSWMASK_C) | (PSWMASK_C * value);
    }
    PC += 2;
    return 2;
}

static int jmp_indir_a_dptr(struct em8051 *aCPU)
{
    PC = ((aCPU->mSFR[REG_DPH] << 8) | (aCPU->mSFR[REG_DPL])) + ACC;
    return 2;
}

static int mov_a_imm(struct em8051 *aCPU)
{
    ACC = OPERAND1;
    PC += 2;
    return 2;
}

static int mov_mem_imm(struct em8051 *aCPU)
{
    int address = OPERAND1;
    if (address > 0x7f)
    {
        aCPU->mSFR[address - 0x80] = OPERAND2;
        if (aCPU->sfrwrite)
            aCPU->sfrwrite(aCPU, address);
    }
    else
    {
        aCPU->mLowerData[address] = OPERAND2;
    }

    PC += 3;
    return 3;
}

static int mov_indir_rx_imm(struct em8051 *aCPU)
{
    int address = INDIR_RX_ADDRESS;
    int value = OPERAND1;
    if (address > 0x7f)
    {
        if (aCPU->mUpperData)
        {
            aCPU->mUpperData[address - 0x80] = value;
        }
    }
    else
    {
        aCPU->mLowerData[address] = value;
    }

    PC += 2;
    return 3;
}


static int sjmp_offset(struct em8051 *aCPU)
{
    PC += (signed char)(OPERAND1) + 2;
    return 3;
}

static int anl_c_bitaddr(struct em8051 *aCPU)
{
    int address = OPERAND1;
    int carry = CARRY;
    if (address > 0x7f)
    {
        int bit = address & 7;
        int bitmask = (1 << bit);
        int value;
        address &= 0xf8;        
        if (aCPU->sfrread)
            value = aCPU->sfrread(aCPU, address);
        else
            value = aCPU->mSFR[address - 0x80];

        value = (value & bitmask) ? carry : 0;

        PSW = (PSW & ~PSWMASK_C) | (PSWMASK_C * value);
    }
    else
    {
        int bit = address & 7;
        int bitmask = (1 << bit);
        int value;
        address >>= 3;
        address += 0x20;
        value = (aCPU->mLowerData[address] & bitmask) ? carry : 0;
        PSW = (PSW & ~PSWMASK_C) | (PSWMASK_C * value);
    }
    PC += 2;
    return 2;
}

static int movc_a_indir_a_pc(struct em8051 *aCPU)
{
    int address = PC + 1 + ACC;
    ACC = aCPU->mCodeMem[address & (aCPU->mCodeMemSize - 1)];
    PC++;
    return 3;
}

static int div_ab(struct em8051 *aCPU)
{
    int a = ACC;
    int b = aCPU->mSFR[REG_B];
    int res;
    PSW &= ~(PSWMASK_C|PSWMASK_OV);
    if (b)
    {
        res = a/b;
        b = a % b;
        a = res;
    }
    else
    {
        PSW |= PSWMASK_OV;
    }
    ACC = a;
    aCPU->mSFR[REG_B] = b;
    PC++;
    return 5;
}

static int mov_mem_mem(struct em8051 *aCPU)
{
    int address1 = OPERAND2;
    int value = read_mem(aCPU, OPERAND1);

    if (address1 > 0x7f)
    {
        aCPU->mSFR[address1 - 0x80] = value;
        if (aCPU->sfrwrite)
            aCPU->sfrwrite(aCPU, address1);
    }
    else
    {
        aCPU->mLowerData[address1] = value;
    }

    PC += 3;
    return 4;
}

static int mov_mem_indir_rx(struct em8051 *aCPU)
{
    int address1 = OPERAND1;
    int address2 = INDIR_RX_ADDRESS;
    if (address1 > 0x7f)
    {
        if (address2 > 0x7f)
        {
            int value = BAD_VALUE;
            if (aCPU->mUpperData)
            {
                value = aCPU->mUpperData[address2 - 0x80];
            }
            aCPU->mSFR[address1 - 0x80] = value;
            if (aCPU->sfrwrite)
                aCPU->sfrwrite(aCPU, address1);
        }
        else
        {
            aCPU->mSFR[address1 - 0x80] = aCPU->mLowerData[address2];
            if (aCPU->sfrwrite)
                aCPU->sfrwrite(aCPU, address1);
        }
    }
    else
    {
        if (address2 > 0x7f)
        {
            int value = BAD_VALUE;
            if (aCPU->mUpperData)
            {
                value = aCPU->mUpperData[address2 - 0x80];
            }
            aCPU->mLowerData[address1] = value;
        }
        else
        {
            aCPU->mLowerData[address1] = aCPU->mLowerData[address2];
        }
    }

    PC += 2;
    return 4;
}


static int mov_dptr_imm(struct em8051 *aCPU)
{
    aCPU->mSFR[REG_DPH] = OPERAND1;
    aCPU->mSFR[REG_DPL] = OPERAND2;
    PC += 3;
    return 3;
}

static int mov_bitaddr_c(struct em8051 *aCPU) 
{
    int address = OPERAND1;
    int carry = CARRY;
    if (address > 0x7f)
    {
        // Data sheet does not explicitly say that the modification source
        // is read from output latch, but we'll assume that is what happens.
        int bit = address & 7;
        int bitmask = (1 << bit);
        address &= 0xf8;        
        aCPU->mSFR[address - 0x80] = (aCPU->mSFR[address - 0x80] & ~bitmask) | (carry << bit);
        if (aCPU->sfrwrite)
            aCPU->sfrwrite(aCPU, address);
    }
    else
    {
        int bit = address & 7;
        int bitmask = (1 << bit);
        address >>= 3;
        address += 0x20;
        aCPU->mLowerData[address] = (aCPU->mLowerData[address] & ~bitmask) | (carry << bit);
    }
    PC += 2;
    return 3;
}

static int movc_a_indir_a_dptr(struct em8051 *aCPU)
{
    int address = ((aCPU->mSFR[REG_DPH] << 8) | (aCPU->mSFR[REG_DPL] << 0)) + ACC;
    ACC = aCPU->mCodeMem[address & (aCPU->mCodeMemSize - 1)];
    PC++;
    return 3;
}

static int subb_a_imm(struct em8051 *aCPU)
{
    int carry = CARRY;
    sub_solve_flags(aCPU, ACC, OPERAND1 + carry);
    ACC -= OPERAND1 + carry;
    PC += 2;
    return 1;
}

static int subb_a_mem(struct em8051 *aCPU) 
{
    int carry = CARRY;
    int value = read_mem(aCPU, OPERAND1) + carry;
    sub_solve_flags(aCPU, ACC, value);
    ACC -= value;

    PC += 2;
    return 2;
}
static int subb_a_indir_rx(struct em8051 *aCPU)
{
    int carry = CARRY;
    int address = INDIR_RX_ADDRESS;
    if (address > 0x7f)
    {
        int value = BAD_VALUE;
        if (aCPU->mUpperData)
        {
            value = aCPU->mUpperData[address - 0x80];
        }

        sub_solve_flags(aCPU, ACC, value);
        ACC -= value;
    }
    else
    {
        sub_solve_flags(aCPU, ACC, aCPU->mLowerData[address] + carry);
        ACC -= aCPU->mLowerData[address] + carry;
    }
    PC++;
    return 2;
}


static int orl_c_compl_bitaddr(struct em8051 *aCPU)
{
    int address = OPERAND1;
    int carry = CARRY;
    if (address > 0x7f)
    {
        int bit = address & 7;
        int bitmask = (1 << bit);
        int value;
        address &= 0xf8;        
        if (aCPU->sfrread)
            value = aCPU->sfrread(aCPU, address);
        else
            value = aCPU->mSFR[address - 0x80];

        value = (value & bitmask) ? carry : 1;

        PSW = (PSW & ~PSWMASK_C) | (PSWMASK_C * value);
    }
    else
    {
        int bit = address & 7;
        int bitmask = (1 << bit);
        int value;
        address >>= 3;
        address += 0x20;
        value = (aCPU->mLowerData[address] & bitmask) ? carry : 1;
        PSW = (PSW & ~PSWMASK_C) | (PSWMASK_C * value);
    }
    PC += 2;
    return 2;
}

static int mov_c_bitaddr(struct em8051 *aCPU) 
{
    int address = OPERAND1;
    int carry = CARRY;
    if (address > 0x7f)
    {
        int bit = address & 7;
        int bitmask = (1 << bit);
        int value;
        address &= 0xf8;        
        if (aCPU->sfrread)
            value = aCPU->sfrread(aCPU, address);
        else
            value = aCPU->mSFR[address - 0x80];

        value = (value & bitmask) ? 1 : 0;

        PSW = (PSW & ~PSWMASK_C) | (PSWMASK_C * value);
    }
    else
    {
        int bit = address & 7;
        int bitmask = (1 << bit);
        int value;
        address >>= 3;
        address += 0x20;
        value = (aCPU->mLowerData[address] & bitmask) ? 1 : 0;
        PSW = (PSW & ~PSWMASK_C) | (PSWMASK_C * value);
    }

    PC += 2;
    return 2;
}

static int inc_dptr(struct em8051 *aCPU)
{
    aCPU->mSFR[REG_DPL]++;
    if (!aCPU->mSFR[REG_DPL])
        aCPU->mSFR[REG_DPH]++;
    PC++;
    return 1;
}

static int mul_ab(struct em8051 *aCPU)
{
    int a = ACC;
    int b = aCPU->mSFR[REG_B];
    int res = a*b;
    ACC = res & 0xff;
    aCPU->mSFR[REG_B] = res >> 8;
    PSW &= ~(PSWMASK_C|PSWMASK_OV);
    if (aCPU->mSFR[REG_B])
        PSW |= PSWMASK_OV;
    PC++;
    return 5;
}

static int mov_indir_rx_mem(struct em8051 *aCPU)
{
    int address1 = INDIR_RX_ADDRESS;
    int value = read_mem(aCPU, OPERAND1);
    if (address1 > 0x7f)
    {
        if (aCPU->mUpperData)
        {
            aCPU->mUpperData[address1 - 0x80] = value;
        }
    }
    else
    {
        aCPU->mLowerData[address1] = value;
    }
    PC += 2;
    return 5;
}


static int anl_c_compl_bitaddr(struct em8051 *aCPU)
{
    int address = OPERAND1;
    int carry = CARRY;
    if (address > 0x7f)
    {
        int bit = address & 7;
        int bitmask = (1 << bit);
        int value;
        address &= 0xf8;        
        if (aCPU->sfrread)
            value = aCPU->sfrread(aCPU, address);
        else
            value = aCPU->mSFR[address - 0x80];

        value = (value & bitmask) ? 0 : carry;

        PSW = (PSW & ~PSWMASK_C) | (PSWMASK_C * value);
    }
    else
    {
        int bit = address & 7;
        int bitmask = (1 << bit);
        int value;
        address >>= 3;
        address += 0x20;
        value = (aCPU->mLowerData[address] & bitmask) ? 0 : carry;
        PSW = (PSW & ~PSWMASK_C) | (PSWMASK_C * value);
    }
    PC += 2;
    return 2;
}


static int cpl_bitaddr(struct em8051 *aCPU)
{
    int address = aCPU->mCodeMem[(PC + 1) & (aCPU->mCodeMemSize - 1)];
    if (address > 0x7f)
    {
        // Data sheet does not explicitly say that the modification source
        // is read from output latch, but we'll assume that is what happens.
        int bit = address & 7;
        int bitmask = (1 << bit);
        address &= 0xf8;        
        aCPU->mSFR[address - 0x80] ^= bitmask;
        if (aCPU->sfrwrite)
            aCPU->sfrwrite(aCPU, address);
    }
    else
    {
        int bit = address & 7;
        int bitmask = (1 << bit);
        address >>= 3;
        address += 0x20;
        aCPU->mLowerData[address] ^= bitmask;
    }
    PC += 2;
    return 3;
}

static int cpl_c(struct em8051 *aCPU)
{
    PSW ^= PSWMASK_C;
    PC++;
    return 1;
}

static int cjne_a_imm_offset(struct em8051 *aCPU)
{
    int value = OPERAND1;

    if (ACC < value)
    {
        PSW |= PSWMASK_C;
    }
    else
    {
        PSW &= ~PSWMASK_C;
    }

    if (ACC != value)
    {
        PC += (signed char)OPERAND2 + 3;
    }
    else
    {
        PC += 3;
    }
    return 4;
}

static int cjne_a_mem_offset(struct em8051 *aCPU)
{
    int address = OPERAND1;
    int value;
    if (address > 0x7f)
    {
        if (aCPU->sfrread)
            value = aCPU->sfrread(aCPU, address);
        else
            value = aCPU->mSFR[address - 0x80];
    }
    else
    {
        value = aCPU->mLowerData[address];
    }  

    if (ACC < value)
    {
        PSW |= PSWMASK_C;
    }
    else
    {
        PSW &= ~PSWMASK_C;
    }

    if (ACC != value)
    {
        PC += (signed char)OPERAND2 + 3;
    }
    else
    {
        PC += 3;
    }
    return 4;
}
static int cjne_indir_rx_imm_offset(struct em8051 *aCPU)
{
    int address = INDIR_RX_ADDRESS;
    int value1 = BAD_VALUE;
    int value2 = OPERAND1;
    if (address > 0x7f)
    {
        if (aCPU->mUpperData)
        {
            value1 = aCPU->mUpperData[address - 0x80];
        }
    }
    else
    {
        value1 = aCPU->mLowerData[address];
    }  

    if (value1 < value2)
    {
        PSW |= PSWMASK_C;
    }
    else
    {
        PSW &= ~PSWMASK_C;
    }

    if (value1 != value2)
    {
        PC += (signed char)OPERAND2 + 3;
    }
    else
    {
        PC += 3;
    }
    return 4;
}

static int push_mem(struct em8051 *aCPU)
{
    int value = read_mem(aCPU, OPERAND1);
    push_to_stack(aCPU, value);   
    PC += 2;
    return 4;
}


static int clr_bitaddr(struct em8051 *aCPU)
{
    int address = aCPU->mCodeMem[(PC + 1) & (aCPU->mCodeMemSize - 1)];
    if (address > 0x7f)
    {
        // Data sheet does not explicitly say that the modification source
        // is read from output latch, but we'll assume that is what happens.
        int bit = address & 7;
        int bitmask = (1 << bit);
        address &= 0xf8;        
        aCPU->mSFR[address - 0x80] &= ~bitmask;
        if (aCPU->sfrwrite)
            aCPU->sfrwrite(aCPU, address);
    }
    else
    {
        int bit = address & 7;
        int bitmask = (1 << bit);
        address >>= 3;
        address += 0x20;
        aCPU->mLowerData[address] &= ~bitmask;
    }
    PC += 2;
    return 3;
}

static int clr_c(struct em8051 *aCPU)
{
    PSW &= ~PSWMASK_C;
    PC++;
    return 1;
}

static int swap_a(struct em8051 *aCPU)
{
    ACC = (ACC << 4) | (ACC >> 4);
    PC++;
    return 1;
}

static int xch_a_mem(struct em8051 *aCPU)
{
    int address = OPERAND1;
    int value = read_mem(aCPU, OPERAND1);
    if (address > 0x7f)
    {
        aCPU->mSFR[address - 0x80] = ACC;
        ACC = value;
        if (aCPU->sfrwrite)
            aCPU->sfrwrite(aCPU, address);
    }
    else
    {
        aCPU->mLowerData[address] = ACC;
        ACC = value;
    }
    PC += 2;
    return 3;
}

static int xch_a_indir_rx(struct em8051 *aCPU)
{
    int address = INDIR_RX_ADDRESS;
    if (address > 0x7f)
    {
        int value;
        if (aCPU->mUpperData)
        {
            value = aCPU->mUpperData[address - 0x80];
            aCPU->mUpperData[address - 0x80] = ACC;
            ACC = value;
        }
    }
    else
    {
        int value = aCPU->mLowerData[address];
        aCPU->mLowerData[address] = ACC;
        ACC = value;
    }
    PC++;
    return 3;
}


static int pop_mem(struct em8051 *aCPU)
{
    int address = OPERAND1;
    if (address > 0x7f)
    {
        aCPU->mSFR[address - 0x80] = pop_from_stack(aCPU);
        if (aCPU->sfrwrite)
            aCPU->sfrwrite(aCPU, address);
    }
    else
    {
        aCPU->mLowerData[address] = pop_from_stack(aCPU);
    }

    PC += 2;
    return 3;
}

static int setb_bitaddr(struct em8051 *aCPU)
{
    int address = aCPU->mCodeMem[(PC + 1) & (aCPU->mCodeMemSize - 1)];
    if (address > 0x7f)
    {
        // Data sheet does not explicitly say that the modification source
        // is read from output latch, but we'll assume that is what happens.
        int bit = address & 7;
        int bitmask = (1 << bit);
        address &= 0xf8;        
        aCPU->mSFR[address - 0x80] |= bitmask;
        if (aCPU->sfrwrite)
            aCPU->sfrwrite(aCPU, address);
    }
    else
    {
        int bit = address & 7;
        int bitmask = (1 << bit);
        address >>= 3;
        address += 0x20;
        aCPU->mLowerData[address] |= bitmask;
    }
    PC += 2;
    return 3;
}

static int setb_c(struct em8051 *aCPU)
{
    PSW |= PSWMASK_C;
    PC++;
    return 1;
}

static int da_a(struct em8051 *aCPU)
{
    // data sheets for this operation are a bit unclear..
    // - should AC (or C) ever be cleared?
    // - should this be done in two steps?

    int result = ACC;
    if ((result & 0xf) > 9 || (PSW & PSWMASK_AC))
        result += 0x6;
    if ((result & 0xff0) > 0x90 || (PSW & PSWMASK_C))
        result += 0x60;
    if (result > 0x99)
        PSW |= PSWMASK_C;
    ACC = result;

 /*
    // this is basically what intel datasheet says the op should do..
    int adder = 0;
    if (ACC & 0xf > 9 || PSW & PSWMASK_AC)
        adder = 6;
    if (ACC & 0xf0 > 0x90 || PSW & PSWMASK_C)
        adder |= 0x60;
    adder += aCPU[REG_ACC];
    if (adder > 0x99)
        PSW |= PSWMASK_C;
    aCPU[REG_ACC] = adder;
*/
    PC++;
    return 1;
}

static int djnz_mem_offset(struct em8051 *aCPU)
{
    int address = OPERAND1;
    int value;
    if (address > 0x7f)
    {
        aCPU->mSFR[address - 0x80]--;
        value = aCPU->mSFR[address - 0x80];
        if (aCPU->sfrwrite)
            aCPU->sfrwrite(aCPU, address);
    }
    else
    {
        aCPU->mLowerData[address]--;
        value = aCPU->mLowerData[address];
    }
    if (value)
    {
        PC += (signed char)OPERAND2 + 3;
    }
    else
    {
        PC += 3;
    }
    return 4;
}

static int xchd_a_indir_rx(struct em8051 *aCPU)
{
    int address = INDIR_RX_ADDRESS;
    if (address > 0x7f)
    {
        int value;
        if (aCPU->mUpperData)
        {
            value = aCPU->mUpperData[address - 0x80];
            aCPU->mUpperData[address - 0x80] = (aCPU->mUpperData[address - 0x80] & 0xf0) | (ACC & 0x0f);
            ACC = (ACC & 0xf0) | (value & 0x0f);
        }
    }
    else
    {
        int value = aCPU->mLowerData[address];
        aCPU->mLowerData[address] = (aCPU->mLowerData[address] & 0x0f) | (ACC & 0x0f);
        ACC = (ACC & 0xf0) | (value & 0x0f);
    }
    PC++;
    return 3;
}


static int movx_a_indir_dptr(struct em8051 *aCPU)
{
    int dptr = (aCPU->mSFR[REG_DPH] << 8) | aCPU->mSFR[REG_DPL];
    if (aCPU->xread)
    {
        ACC = aCPU->xread(aCPU, dptr);
    }
    else
    {
        if (aCPU->mExtData)
            ACC = aCPU->mExtData[dptr & (aCPU->mExtDataSize - 1)];
    }
    PC++;
    return 2;
}

static int movx_a_indir_rx(struct em8051 *aCPU)
{
    int address = INDIR_RX_ADDRESS;
    if (aCPU->xread)
    {
        ACC = aCPU->xread(aCPU, address);
    }
    else
    {
        if (aCPU->mExtData)
            ACC = aCPU->mExtData[address & (aCPU->mExtDataSize - 1)];
    }

    PC++;
    return 4;
}

static int clr_a(struct em8051 *aCPU)
{
    ACC = 0;
    PC++;
    return 1;
}

static int mov_a_mem(struct em8051 *aCPU)
{
    // mov a,acc is not a valid instruction
    int address = OPERAND1;
    int value = read_mem(aCPU, address);
    if (REG_ACC == address - 0x80)
        if (aCPU->except)
            aCPU->except(aCPU, EXCEPTION_ACC_TO_A);
    ACC = value;

    PC += 2;
    return 2;
}

static int mov_a_indir_rx(struct em8051 *aCPU)
{
    int address = INDIR_RX_ADDRESS;
    if (address > 0x7f)
    {
        int value = BAD_VALUE;
        if (aCPU->mUpperData)
        {
            value = aCPU->mUpperData[address - 0x80];
        }

        ACC = value;
    }
    else
    {
        ACC = aCPU->mLowerData[address];
    }

    PC++;
    return 2;
}


static int movx_indir_dptr_a(struct em8051 *aCPU)
{
    int dptr = (aCPU->mSFR[REG_DPH] << 8) | aCPU->mSFR[REG_DPL];
    if (aCPU->xwrite)
    {
        aCPU->xwrite(aCPU, dptr, ACC);
    }
    else
    {
        if (aCPU->mExtData)
            aCPU->mExtData[dptr & (aCPU->mExtDataSize - 1)] = ACC;
    }

    PC++;
    return 5;
}

static int movx_indir_rx_a(struct em8051 *aCPU)
{
    int address = INDIR_RX_ADDRESS;

    if (aCPU->xwrite)
    {
        aCPU->xwrite(aCPU, address, ACC);
    }
    else
    {
        if (aCPU->mExtData)
            aCPU->mExtData[address & (aCPU->mExtDataSize - 1)] = ACC;
    }

    PC++;
    return 5;
}

static int cpl_a(struct em8051 *aCPU)
{
    ACC = ~ACC;
    PC++;
    return 1;
}

static int mov_mem_a(struct em8051 *aCPU)
{
    int address = OPERAND1;
    if (address > 0x7f)
    {
        aCPU->mSFR[address - 0x80] = ACC;
        if (aCPU->sfrwrite)
            aCPU->sfrwrite(aCPU, address);
    }
    else
    {
        aCPU->mLowerData[address] = ACC;
    }
    PC += 2;
    return 3;
}

static int mov_indir_rx_a(struct em8051 *aCPU)
{
    int address = INDIR_RX_ADDRESS;
    if (address > 0x7f)
    {
        if (aCPU->mUpperData)
            aCPU->mUpperData[address - 0x80] = ACC;
    }
    else
    {
        aCPU->mLowerData[address] = ACC;
    }

    PC++;
    return 2;
}

static int nop(struct em8051 *aCPU)
{
    if (aCPU->mCodeMem[PC & (aCPU->mCodeMemSize - 1)] != 0)
        if (aCPU->except)
            aCPU->except(aCPU, EXCEPTION_ILLEGAL_OPCODE);
    PC++;
    return 1;
}

static int inc_rx(struct em8051 *aCPU)
{
    int rx = RX_ADDRESS;
    aCPU->mLowerData[rx]++;
    PC++;
    return 2;
}

static int dec_rx(struct em8051 *aCPU)
{
    int rx = RX_ADDRESS;
    aCPU->mLowerData[rx]--;
    PC++;
    return 2;
}

static int add_a_rx(struct em8051 *aCPU)
{
    int rx = RX_ADDRESS;
    add_solve_flags(aCPU, aCPU->mLowerData[rx], ACC, 0);
    ACC += aCPU->mLowerData[rx];
    PC++;
    return 1;
}

static int addc_a_rx(struct em8051 *aCPU)
{
    int rx = RX_ADDRESS;
    int carry = CARRY;
    add_solve_flags(aCPU, aCPU->mLowerData[rx], ACC, carry);
    ACC += aCPU->mLowerData[rx] + carry;
    PC++;
    return 1;
}

static int orl_a_rx(struct em8051 *aCPU)
{
    int rx = RX_ADDRESS;
    ACC |= aCPU->mLowerData[rx];
    PC++;
    return 1;
}

static int anl_a_rx(struct em8051 *aCPU)
{
    int rx = RX_ADDRESS;
    ACC &= aCPU->mLowerData[rx];
    PC++;
    return 1;
}

static int xrl_a_rx(struct em8051 *aCPU)
{
    int rx = RX_ADDRESS;
    ACC ^= aCPU->mLowerData[rx];    
    PC++;
    return 1;
}


static int mov_rx_imm(struct em8051 *aCPU)
{
    int rx = RX_ADDRESS;
    aCPU->mLowerData[rx] = OPERAND1;
    PC += 2;
    return 2;
}

static int mov_mem_rx(struct em8051 *aCPU)
{
    int rx = RX_ADDRESS;
    int address = OPERAND1;
    if (address > 0x7f)
    {
        aCPU->mSFR[address - 0x80] = aCPU->mLowerData[rx];
        if (aCPU->sfrwrite)
            aCPU->sfrwrite(aCPU, address);
    }
    else
    {
        aCPU->mLowerData[address] = aCPU->mLowerData[rx];
    }
    PC += 2;
    return 3;
}

static int subb_a_rx(struct em8051 *aCPU)
{
    int rx = RX_ADDRESS;
    int carry = CARRY;
    sub_solve_flags(aCPU, ACC, aCPU->mLowerData[rx] + carry);
    ACC -= aCPU->mLowerData[rx] + carry;
    PC++;
    return 1;
}

static int mov_rx_mem(struct em8051 *aCPU)
{
    int rx = RX_ADDRESS;
    int value = read_mem(aCPU, OPERAND1);
    aCPU->mLowerData[rx] = value;

    PC += 2;
    return 4;
}

static int cjne_rx_imm_offset(struct em8051 *aCPU)
{
    int rx = RX_ADDRESS;
    int value = OPERAND1;
    
    if (aCPU->mLowerData[rx] < value)
    {
        PSW |= PSWMASK_C;
    }
    else
    {
        PSW &= ~PSWMASK_C;
    }

    if (aCPU->mLowerData[rx] != value)
    {
        PC += (signed char)OPERAND2 + 3;
    }
    else
    {
        PC += 3;
    }
    return 4;
}

static int xch_a_rx(struct em8051 *aCPU)
{
    int rx = RX_ADDRESS;
    int a = ACC;
    ACC = aCPU->mLowerData[rx];
    aCPU->mLowerData[rx] = a;
    PC++;
    return 2;
}

static int djnz_rx_offset(struct em8051 *aCPU)
{
    int rx = RX_ADDRESS;
    aCPU->mLowerData[rx]--;
    if (aCPU->mLowerData[rx])
    {
        PC += (signed char)OPERAND1 + 2;
    }
    else
    {
        PC += 2;
    }
    return 3;
}

static int mov_a_rx(struct em8051 *aCPU)
{
    int rx = RX_ADDRESS;
    ACC = aCPU->mLowerData[rx];

    PC++;
    return 1;
}

static int mov_rx_a(struct em8051 *aCPU)
{
    int rx = RX_ADDRESS;
    aCPU->mLowerData[rx] = ACC;
    PC++;
    return 2;
}

void op_setptrs(struct em8051 *aCPU)
{
    int i;
    for (i = 0; i < 8; i++)
    {
        aCPU->op[0x08 + i] = &inc_rx;
        aCPU->op[0x18 + i] = &dec_rx;
        aCPU->op[0x28 + i] = &add_a_rx;
        aCPU->op[0x38 + i] = &addc_a_rx;
        aCPU->op[0x48 + i] = &orl_a_rx;
        aCPU->op[0x58 + i] = &anl_a_rx;
        aCPU->op[0x68 + i] = &xrl_a_rx;
        aCPU->op[0x78 + i] = &mov_rx_imm;
        aCPU->op[0x88 + i] = &mov_mem_rx;
        aCPU->op[0x98 + i] = &subb_a_rx;
        aCPU->op[0xa8 + i] = &mov_rx_mem;
        aCPU->op[0xb8 + i] = &cjne_rx_imm_offset;
        aCPU->op[0xc8 + i] = &xch_a_rx;
        aCPU->op[0xd8 + i] = &djnz_rx_offset;
        aCPU->op[0xe8 + i] = &mov_a_rx;
        aCPU->op[0xf8 + i] = &mov_rx_a;
    }
    aCPU->op[0x00] = &nop;
    aCPU->op[0x01] = &ajmp_offset;
    aCPU->op[0x02] = &ljmp_address;
    aCPU->op[0x03] = &rr_a;
    aCPU->op[0x04] = &inc_a;
    aCPU->op[0x05] = &inc_mem;
    aCPU->op[0x06] = &inc_indir_rx;
    aCPU->op[0x07] = &inc_indir_rx;

    aCPU->op[0x10] = &jbc_bitaddr_offset;
    aCPU->op[0x11] = &acall_offset;
    aCPU->op[0x12] = &lcall_address;
    aCPU->op[0x13] = &rrc_a;
    aCPU->op[0x14] = &dec_a;
    aCPU->op[0x15] = &dec_mem;
    aCPU->op[0x16] = &dec_indir_rx;
    aCPU->op[0x17] = &dec_indir_rx;

    aCPU->op[0x20] = &jb_bitaddr_offset;
    aCPU->op[0x21] = &ajmp_offset;
    aCPU->op[0x22] = &ret;
    aCPU->op[0x23] = &rl_a;
    aCPU->op[0x24] = &add_a_imm;
    aCPU->op[0x25] = &add_a_mem;
    aCPU->op[0x26] = &add_a_indir_rx;
    aCPU->op[0x27] = &add_a_indir_rx;

    aCPU->op[0x30] = &jnb_bitaddr_offset;
    aCPU->op[0x31] = &acall_offset;
    aCPU->op[0x32] = &reti;
    aCPU->op[0x33] = &rlc_a;
    aCPU->op[0x34] = &addc_a_imm;
    aCPU->op[0x35] = &addc_a_mem;
    aCPU->op[0x36] = &addc_a_indir_rx;
    aCPU->op[0x37] = &addc_a_indir_rx;

    aCPU->op[0x40] = &jc_offset;
    aCPU->op[0x41] = &ajmp_offset;
    aCPU->op[0x42] = &orl_mem_a;
    aCPU->op[0x43] = &orl_mem_imm;
    aCPU->op[0x44] = &orl_a_imm;
    aCPU->op[0x45] = &orl_a_mem;
    aCPU->op[0x46] = &orl_a_indir_rx;
    aCPU->op[0x47] = &orl_a_indir_rx;

    aCPU->op[0x50] = &jnc_offset;
    aCPU->op[0x51] = &acall_offset;
    aCPU->op[0x52] = &anl_mem_a;
    aCPU->op[0x53] = &anl_mem_imm;
    aCPU->op[0x54] = &anl_a_imm;
    aCPU->op[0x55] = &anl_a_mem;
    aCPU->op[0x56] = &anl_a_indir_rx;
    aCPU->op[0x57] = &anl_a_indir_rx;

    aCPU->op[0x60] = &jz_offset;
    aCPU->op[0x61] = &ajmp_offset;
    aCPU->op[0x62] = &xrl_mem_a;
    aCPU->op[0x63] = &xrl_mem_imm;
    aCPU->op[0x64] = &xrl_a_imm;
    aCPU->op[0x65] = &xrl_a_mem;
    aCPU->op[0x66] = &xrl_a_indir_rx;
    aCPU->op[0x67] = &xrl_a_indir_rx;

    aCPU->op[0x70] = &jnz_offset;
    aCPU->op[0x71] = &acall_offset;
    aCPU->op[0x72] = &orl_c_bitaddr;
    aCPU->op[0x73] = &jmp_indir_a_dptr;
    aCPU->op[0x74] = &mov_a_imm;
    aCPU->op[0x75] = &mov_mem_imm;
    aCPU->op[0x76] = &mov_indir_rx_imm;
    aCPU->op[0x77] = &mov_indir_rx_imm;

    aCPU->op[0x80] = &sjmp_offset;
    aCPU->op[0x81] = &ajmp_offset;
    aCPU->op[0x82] = &anl_c_bitaddr;
    aCPU->op[0x83] = &movc_a_indir_a_pc;
    aCPU->op[0x84] = &div_ab;
    aCPU->op[0x85] = &mov_mem_mem;
    aCPU->op[0x86] = &mov_mem_indir_rx;
    aCPU->op[0x87] = &mov_mem_indir_rx;

    aCPU->op[0x90] = &mov_dptr_imm;
    aCPU->op[0x91] = &acall_offset;
    aCPU->op[0x92] = &mov_bitaddr_c;
    aCPU->op[0x93] = &movc_a_indir_a_dptr;
    aCPU->op[0x94] = &subb_a_imm;
    aCPU->op[0x95] = &subb_a_mem;
    aCPU->op[0x96] = &subb_a_indir_rx;
    aCPU->op[0x97] = &subb_a_indir_rx;

    aCPU->op[0xa0] = &orl_c_compl_bitaddr;
    aCPU->op[0xa1] = &ajmp_offset;
    aCPU->op[0xa2] = &mov_c_bitaddr;
    aCPU->op[0xa3] = &inc_dptr;
    aCPU->op[0xa4] = &mul_ab;
    aCPU->op[0xa5] = &nop; // unused
    aCPU->op[0xa6] = &mov_indir_rx_mem;
    aCPU->op[0xa7] = &mov_indir_rx_mem;

    aCPU->op[0xb0] = &anl_c_compl_bitaddr;
    aCPU->op[0xb1] = &acall_offset;
    aCPU->op[0xb2] = &cpl_bitaddr;
    aCPU->op[0xb3] = &cpl_c;
    aCPU->op[0xb4] = &cjne_a_imm_offset;
    aCPU->op[0xb5] = &cjne_a_mem_offset;
    aCPU->op[0xb6] = &cjne_indir_rx_imm_offset;
    aCPU->op[0xb7] = &cjne_indir_rx_imm_offset;

    aCPU->op[0xc0] = &push_mem;
    aCPU->op[0xc1] = &ajmp_offset;
    aCPU->op[0xc2] = &clr_bitaddr;
    aCPU->op[0xc3] = &clr_c;
    aCPU->op[0xc4] = &swap_a;
    aCPU->op[0xc5] = &xch_a_mem;
    aCPU->op[0xc6] = &xch_a_indir_rx;
    aCPU->op[0xc7] = &xch_a_indir_rx;

    aCPU->op[0xd0] = &pop_mem;
    aCPU->op[0xd1] = &acall_offset;
    aCPU->op[0xd2] = &setb_bitaddr;
    aCPU->op[0xd3] = &setb_c;
    aCPU->op[0xd4] = &da_a;
    aCPU->op[0xd5] = &djnz_mem_offset;
    aCPU->op[0xd6] = &xchd_a_indir_rx;
    aCPU->op[0xd7] = &xchd_a_indir_rx;

    aCPU->op[0xe0] = &movx_a_indir_dptr;
    aCPU->op[0xe1] = &ajmp_offset;
    aCPU->op[0xe2] = &movx_a_indir_rx;
    aCPU->op[0xe3] = &movx_a_indir_rx;
    aCPU->op[0xe4] = &clr_a;
    aCPU->op[0xe5] = &mov_a_mem;
    aCPU->op[0xe6] = &mov_a_indir_rx;
    aCPU->op[0xe7] = &mov_a_indir_rx;

    aCPU->op[0xf0] = &movx_indir_dptr_a;
    aCPU->op[0xf1] = &acall_offset;
    aCPU->op[0xf2] = &movx_indir_rx_a;
    aCPU->op[0xf3] = &movx_indir_rx_a;
    aCPU->op[0xf4] = &cpl_a;
    aCPU->op[0xf5] = &mov_mem_a;
    aCPU->op[0xf6] = &mov_indir_rx_a;
    aCPU->op[0xf7] = &mov_indir_rx_a;
}

int do_op(struct em8051 *aCPU)
{
    switch (OPCODE)
    {
    case 0x00: return nop(aCPU);
    case 0x01: return ajmp_offset(aCPU);
    case 0x02: return ljmp_address(aCPU);
    case 0x03: return rr_a(aCPU);
    case 0x04: return inc_a(aCPU);
    case 0x05: return inc_mem(aCPU);
    case 0x06: return inc_indir_rx(aCPU);
    case 0x07: return inc_indir_rx(aCPU);

    case 0x08:
    case 0x09:
    case 0x0a:
    case 0x0b:
    case 0x0c:
    case 0x0d:
    case 0x0e:
    case 0x0f: return inc_rx(aCPU);

    case 0x10: return jbc_bitaddr_offset(aCPU);
    case 0x11: return acall_offset(aCPU);
    case 0x12: return lcall_address(aCPU);
    case 0x13: return rrc_a(aCPU);
    case 0x14: return dec_a(aCPU);
    case 0x15: return dec_mem(aCPU);
    case 0x16: return dec_indir_rx(aCPU);
    case 0x17: return dec_indir_rx(aCPU);

    case 0x18:
    case 0x19:
    case 0x1a:
    case 0x1b:
    case 0x1c:
    case 0x1d:
    case 0x1e:
    case 0x1f: return dec_rx(aCPU);

    case 0x20: return jb_bitaddr_offset(aCPU);
    case 0x21: return ajmp_offset(aCPU);
    case 0x22: return ret(aCPU);
    case 0x23: return rl_a(aCPU);
    case 0x24: return add_a_imm(aCPU);
    case 0x25: return add_a_mem(aCPU);
    case 0x26: return add_a_indir_rx(aCPU);
    case 0x27: return add_a_indir_rx(aCPU);

    case 0x28:
    case 0x29:
    case 0x2a:
    case 0x2b:
    case 0x2c:
    case 0x2d:
    case 0x2e:
    case 0x2f: return add_a_rx(aCPU);

    case 0x30: return jnb_bitaddr_offset(aCPU);
    case 0x31: return acall_offset(aCPU);
    case 0x32: return reti(aCPU);
    case 0x33: return rlc_a(aCPU);
    case 0x34: return addc_a_imm(aCPU);
    case 0x35: return addc_a_mem(aCPU);
    case 0x36: return addc_a_indir_rx(aCPU);
    case 0x37: return addc_a_indir_rx(aCPU);

    case 0x38:
    case 0x39:
    case 0x3a:
    case 0x3b:
    case 0x3c:
    case 0x3d:
    case 0x3e:
    case 0x3f: return addc_a_rx(aCPU);

    case 0x40: return jc_offset(aCPU);
    case 0x41: return ajmp_offset(aCPU);
    case 0x42: return orl_mem_a(aCPU);
    case 0x43: return orl_mem_imm(aCPU);
    case 0x44: return orl_a_imm(aCPU);
    case 0x45: return orl_a_mem(aCPU);
    case 0x46: return orl_a_indir_rx(aCPU);
    case 0x47: return orl_a_indir_rx(aCPU);

    case 0x48:
    case 0x49:
    case 0x4a:
    case 0x4b:
    case 0x4c:
    case 0x4d:
    case 0x4e:
    case 0x4f: return orl_a_rx(aCPU);

    case 0x50: return jnc_offset(aCPU);
    case 0x51: return acall_offset(aCPU);
    case 0x52: return anl_mem_a(aCPU);
    case 0x53: return anl_mem_imm(aCPU);
    case 0x54: return anl_a_imm(aCPU);
    case 0x55: return anl_a_mem(aCPU);
    case 0x56: return anl_a_indir_rx(aCPU);
    case 0x57: return anl_a_indir_rx(aCPU);

    case 0x58:
    case 0x59:
    case 0x5a:
    case 0x5b:
    case 0x5c:
    case 0x5d:
    case 0x5e:
    case 0x5f: return anl_a_rx(aCPU);

    case 0x60: return jz_offset(aCPU);
    case 0x61: return ajmp_offset(aCPU);
    case 0x62: return xrl_mem_a(aCPU);
    case 0x63: return xrl_mem_imm(aCPU);
    case 0x64: return xrl_a_imm(aCPU);
    case 0x65: return xrl_a_mem(aCPU);
    case 0x66: return xrl_a_indir_rx(aCPU);
    case 0x67: return xrl_a_indir_rx(aCPU);

    case 0x68:
    case 0x69:
    case 0x6a:
    case 0x6b:
    case 0x6c:
    case 0x6d:
    case 0x6e:
    case 0x6f: return xrl_a_rx(aCPU);

    case 0x70: return jnz_offset(aCPU);
    case 0x71: return acall_offset(aCPU);
    case 0x72: return orl_c_bitaddr(aCPU);
    case 0x73: return jmp_indir_a_dptr(aCPU);
    case 0x74: return mov_a_imm(aCPU);
    case 0x75: return mov_mem_imm(aCPU);
    case 0x76: return mov_indir_rx_imm(aCPU);
    case 0x77: return mov_indir_rx_imm(aCPU);

    case 0x78:
    case 0x79:
    case 0x7a:
    case 0x7b:
    case 0x7c:
    case 0x7d:
    case 0x7e:
    case 0x7f: return mov_rx_imm(aCPU);

    case 0x80: return sjmp_offset(aCPU);
    case 0x81: return ajmp_offset(aCPU);
    case 0x82: return anl_c_bitaddr(aCPU);
    case 0x83: return movc_a_indir_a_pc(aCPU);
    case 0x84: return div_ab(aCPU);
    case 0x85: return mov_mem_mem(aCPU);
    case 0x86: return mov_mem_indir_rx(aCPU);
    case 0x87: return mov_mem_indir_rx(aCPU);

    case 0x88:
    case 0x89:
    case 0x8a:
    case 0x8b:
    case 0x8c:
    case 0x8d:
    case 0x8e:
    case 0x8f: return mov_mem_rx(aCPU);

    case 0x90: return mov_dptr_imm(aCPU);
    case 0x91: return acall_offset(aCPU);
    case 0x92: return mov_bitaddr_c(aCPU);
    case 0x93: return movc_a_indir_a_dptr(aCPU);
    case 0x94: return subb_a_imm(aCPU);
    case 0x95: return subb_a_mem(aCPU);
    case 0x96: return subb_a_indir_rx(aCPU);
    case 0x97: return subb_a_indir_rx(aCPU);

    case 0x98:
    case 0x99:
    case 0x9a:
    case 0x9b:
    case 0x9c:
    case 0x9d:
    case 0x9e:
    case 0x9f: return subb_a_rx(aCPU);

    case 0xa0: return orl_c_compl_bitaddr(aCPU);
    case 0xa1: return ajmp_offset(aCPU);
    case 0xa2: return mov_c_bitaddr(aCPU);
    case 0xa3: return inc_dptr(aCPU);
    case 0xa4: return mul_ab(aCPU);
    case 0xa5: return nop(aCPU); // unused
    case 0xa6: return mov_indir_rx_mem(aCPU);
    case 0xa7: return mov_indir_rx_mem(aCPU);

    case 0xa8:
    case 0xa9:
    case 0xaa:
    case 0xab:
    case 0xac:
    case 0xad:
    case 0xae:
    case 0xaf: return mov_rx_mem(aCPU);

    case 0xb0: return anl_c_compl_bitaddr(aCPU);
    case 0xb1: return acall_offset(aCPU);
    case 0xb2: return cpl_bitaddr(aCPU);
    case 0xb3: return cpl_c(aCPU);
    case 0xb4: return cjne_a_imm_offset(aCPU);
    case 0xb5: return cjne_a_mem_offset(aCPU);
    case 0xb6: return cjne_indir_rx_imm_offset(aCPU);
    case 0xb7: return cjne_indir_rx_imm_offset(aCPU);

    case 0xb8:
    case 0xb9:
    case 0xba:
    case 0xbb:
    case 0xbc:
    case 0xbd:
    case 0xbe:
    case 0xbf: return cjne_rx_imm_offset(aCPU);

    case 0xc0: return push_mem(aCPU);
    case 0xc1: return ajmp_offset(aCPU);
    case 0xc2: return clr_bitaddr(aCPU);
    case 0xc3: return clr_c(aCPU);
    case 0xc4: return swap_a(aCPU);
    case 0xc5: return xch_a_mem(aCPU);
    case 0xc6: return xch_a_indir_rx(aCPU);
    case 0xc7: return xch_a_indir_rx(aCPU);

    case 0xc8:
    case 0xc9:
    case 0xca:
    case 0xcb:
    case 0xcc:
    case 0xcd:
    case 0xce:
    case 0xcf: return xch_a_rx(aCPU);

    case 0xd0: return pop_mem(aCPU);
    case 0xd1: return acall_offset(aCPU);
    case 0xd2: return setb_bitaddr(aCPU);
    case 0xd3: return setb_c(aCPU);
    case 0xd4: return da_a(aCPU);
    case 0xd5: return djnz_mem_offset(aCPU);
    case 0xd6: return xchd_a_indir_rx(aCPU);
    case 0xd7: return xchd_a_indir_rx(aCPU);

    case 0xd8:
    case 0xd9:
    case 0xda:
    case 0xdb:
    case 0xdc:
    case 0xdd:
    case 0xde:
    case 0xdf: return djnz_rx_offset(aCPU);

    case 0xe0: return movx_a_indir_dptr(aCPU);
    case 0xe1: return ajmp_offset(aCPU);
    case 0xe2: return movx_a_indir_rx(aCPU);
    case 0xe3: return movx_a_indir_rx(aCPU);
    case 0xe4: return clr_a(aCPU);
    case 0xe5: return mov_a_mem(aCPU);
    case 0xe6: return mov_a_indir_rx(aCPU);
    case 0xe7: return mov_a_indir_rx(aCPU);

    case 0xe8:
    case 0xe9:
    case 0xea:
    case 0xeb:
    case 0xec:
    case 0xed:
    case 0xee:
    case 0xef: return mov_a_rx(aCPU);

    case 0xf0: return movx_indir_dptr_a(aCPU);
    case 0xf1: return acall_offset(aCPU);
    case 0xf2: return movx_indir_rx_a(aCPU);
    case 0xf3: return movx_indir_rx_a(aCPU);
    case 0xf4: return cpl_a(aCPU);
    case 0xf5: return mov_mem_a(aCPU);
    case 0xf6: return mov_indir_rx_a(aCPU);
    case 0xf7: return mov_indir_rx_a(aCPU);

    case 0xf8:
    case 0xf9:
    case 0xfa:
    case 0xfb:
    case 0xfc:
    case 0xfd:
    case 0xfe:
    case 0xff: return mov_rx_a(aCPU);
   }
    return 0;
}

