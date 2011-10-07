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

#include "cube_cpu.h"

namespace Cube {
namespace CPU {


#define BAD_VALUE 0x77
#define PSW aCPU->mSFR[REG_PSW]
#define ACC aCPU->mSFR[REG_ACC]
#define PC aCPU->mPC
#define OPCODE aCPU->mCodeMem[(PC + 0)&(CODE_SIZE-1)]
#define OPERAND1 aCPU->mCodeMem[(PC + 1)&(CODE_SIZE-1)]
#define OPERAND2 aCPU->mCodeMem[(PC + 2)&(CODE_SIZE-1)]
#define INDIR_RX_ADDRESS (aCPU->mData[(OPCODE & 1) + 8 * ((PSW & (PSWMASK_RS0|PSWMASK_RS1))>>PSW_RS0)])
#define RX_ADDRESS ((OPCODE & 7) + 8 * ((PSW & (PSWMASK_RS0|PSWMASK_RS1))>>PSW_RS0))
#define CARRY ((PSW & PSWMASK_C) >> PSW_C)


static int read_mem(em8051 *aCPU, int aAddress)
{
    if (aAddress > 0x7f)
    {
        return aCPU->sfrread(aCPU, aAddress);
    }
    else
    {
        return aCPU->mData[aAddress];
    }
}

void em8051_push(em8051 *aCPU, int aValue)
{
    aCPU->mSFR[REG_SP]++;
    aCPU->mData[aCPU->mSFR[REG_SP]] = aValue;
    if (aCPU->mSFR[REG_SP] == 0)
        aCPU->except(aCPU, EXCEPTION_STACK);
}

static int pop_from_stack(em8051 *aCPU)
{
    int value = BAD_VALUE;
    value = aCPU->mData[aCPU->mSFR[REG_SP]];
    aCPU->mSFR[REG_SP]--;

    if (aCPU->mSFR[REG_SP] == 0xff)
        aCPU->except(aCPU, EXCEPTION_STACK);
    return value;
}


static void add_solve_flags(em8051 * aCPU, int value1, int value2, int acc)
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

static void sub_solve_flags(em8051 * aCPU, int value1, int value2)
{
    int carry = (((value1 & 255) - (value2 & 255)) >> 8) & 1;
    int auxcarry = (((value1 & 7) - (value2 & 7)) >> 3) & 1;
    int overflow = ((((value1 & 127) - (value2 & 127)) >> 7) & 1)^carry;
    PSW = (PSW & ~(PSWMASK_C|PSWMASK_AC|PSWMASK_OV)) |
                          (carry << PSW_C) | (auxcarry << PSW_AC) | (overflow << PSW_OV);
}


static int ajmp_offset(em8051 *aCPU)
{
    int address = ((PC + 2) & 0xf800) |
                  OPERAND1 | 
                  ((OPCODE & 0xe0) << 3);

    PC = address;

    return 3;
}

static int ljmp_address(em8051 *aCPU)
{
    int address = (OPERAND1 << 8) | OPERAND2;
    PC = address;

    return 4;
}


static int rr_a(em8051 *aCPU)
{
    ACC = (ACC >> 1) | (ACC << 7);
    PC++;
    return 1;
}

static int inc_a(em8051 *aCPU)
{
    ACC++;
    PC++;
    return 1;
}

static int inc_mem(em8051 *aCPU)
{
    int address = OPERAND1;
    if (address > 0x7f)
    {
        aCPU->mSFR[address - 0x80]++;
        aCPU->sfrwrite(aCPU, address);
    }
    else
    {
        aCPU->mData[address]++;
    }
    PC += 2;
    return 3;
}

static int inc_indir_rx(em8051 *aCPU)
{    
    int address = INDIR_RX_ADDRESS;
    aCPU->mData[address]++;
    PC++;
    return 3;
}

static int jbc_bitaddr_offset(em8051 *aCPU)
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
        if (aCPU->mData[address] & bitmask)
        {
            aCPU->mData[address] &= ~bitmask;
            PC += (signed char)OPERAND2 + 3;
        }
        else
        {
            PC += 3;
        }
    }
    return 4;
}

static int acall_offset(em8051 *aCPU)
{
    int address = ((PC + 2) & 0xf800) | OPERAND1 | ((OPCODE & 0xe0) << 3);
    em8051_push(aCPU, (PC + 2) & 0xff);
    em8051_push(aCPU, (PC + 2) >> 8);
    PC = address;
    return 6;
}

static int lcall_address(em8051 *aCPU)
{
    em8051_push(aCPU, (PC + 3) & 0xff);
    em8051_push(aCPU, (PC + 3) >> 8);
    PC = (aCPU->mCodeMem[(PC + 1) & (CODE_SIZE-1)] << 8) | 
         (aCPU->mCodeMem[(PC + 2) & (CODE_SIZE-1)] << 0);
    return 6;
}

static int rrc_a(em8051 *aCPU)
{
    int c = (PSW & PSWMASK_C) >> PSW_C;
    int newc = ACC & 1;
    ACC = (ACC >> 1) | (c << 7);
    PSW = (PSW & ~PSWMASK_C) | (newc << PSW_C);
    PC++;
    return 1;
}

static int dec_a(em8051 *aCPU)
{
    ACC--;
    PC++;
    return 1;
}

static int dec_mem(em8051 *aCPU)
{
    int address = OPERAND1;
    if (address > 0x7f)
    {
        aCPU->mSFR[address - 0x80]--;
        aCPU->sfrwrite(aCPU, address);
    }
    else
    {
        aCPU->mData[address]--;
    }
    PC += 2;
    return 3;
}

static int dec_indir_rx(em8051 *aCPU)
{
    int address = INDIR_RX_ADDRESS;
    aCPU->mData[address]--;
    PC++;
    return 3;
}


static int jb_bitaddr_offset(em8051 *aCPU)
{
    int address = OPERAND1;
    if (address > 0x7f)
    {
        int bit = address & 7;
        int bitmask = (1 << bit);
        int value;
        address &= 0xf8;  
        value = aCPU->sfrread(aCPU, address);

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
        if (aCPU->mData[address] & bitmask)
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

static int ret(em8051 *aCPU)
{
    PC = pop_from_stack(aCPU) << 8;
    PC |= pop_from_stack(aCPU);
    return 4;
}

static int rl_a(em8051 *aCPU)
{
    ACC = (ACC << 1) | (ACC >> 7);
    PC++;
    return 1;
}

static int add_a_imm(em8051 *aCPU)
{
    add_solve_flags(aCPU, ACC, OPERAND1, 0);
    ACC += OPERAND1;
    PC += 2;
    return 2;
}

static int add_a_mem(em8051 *aCPU)
{
    int value = read_mem(aCPU, OPERAND1);
    add_solve_flags(aCPU, ACC, value, 0);
    ACC += value;
        PC += 2;
    return 2;
}

static int add_a_indir_rx(em8051 *aCPU)
{
    int address = INDIR_RX_ADDRESS;
    add_solve_flags(aCPU, ACC, aCPU->mData[address], 0);
    ACC += aCPU->mData[address];
    PC++;
    return 2;
}

static int jnb_bitaddr_offset(em8051 *aCPU)
{
    int address = OPERAND1;
    if (address > 0x7f)
    {
        int bit = address & 7;
        int bitmask = (1 << bit);
        int value;
        address &= 0xf8;        
        value = aCPU->sfrread(aCPU, address);
        
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
        if (!(aCPU->mData[address] & bitmask))
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

static int reti(em8051 *aCPU)
{
    if (aCPU->irq_count)
    {
        int i = --aCPU->irq_count;

        /*
         * State restore sanity-check
         */

        int psw_bits = PSWMASK_OV | PSWMASK_RS0 | PSWMASK_RS1 | PSWMASK_AC | PSWMASK_C;

        if (aCPU->irql[i].a != aCPU->mSFR[REG_ACC])
            aCPU->except(aCPU, EXCEPTION_IRET_ACC_MISMATCH);

        if (aCPU->irql[i].sp != aCPU->mSFR[REG_SP])
            aCPU->except(aCPU, EXCEPTION_IRET_SP_MISMATCH);    

        if ((aCPU->irql[i].psw & psw_bits) != (aCPU->mSFR[REG_PSW] & psw_bits))
            aCPU->except(aCPU, EXCEPTION_IRET_PSW_MISMATCH);
    }

    PC = pop_from_stack(aCPU) << 8;
    PC |= pop_from_stack(aCPU);
    return 4;
}

static int rlc_a(em8051 *aCPU)
{
    int c = CARRY;
    int newc = ACC >> 7;
    ACC = (ACC << 1) | c;
    PSW = (PSW & ~PSWMASK_C) | (newc << PSW_C);
    PC++;
    return 1;
}

static int addc_a_imm(em8051 *aCPU)
{
    int carry = CARRY;
    add_solve_flags(aCPU, ACC, OPERAND1, carry);
    ACC += OPERAND1 + carry;
    PC += 2;
    return 2;
}

static int addc_a_mem(em8051 *aCPU)
{
    int carry = CARRY;
    int value = read_mem(aCPU, OPERAND1);
    add_solve_flags(aCPU, ACC, value, carry);
    ACC += value + carry;
    PC += 2;
    return 2;
}

static int addc_a_indir_rx(em8051 *aCPU)
{
    int carry = CARRY;
    int address = INDIR_RX_ADDRESS;
    add_solve_flags(aCPU, ACC, aCPU->mData[address], carry);
    ACC += aCPU->mData[address] + carry;
    PC++;
    return 2;
}


static int jc_offset(em8051 *aCPU)
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

static int orl_mem_a(em8051 *aCPU)
{
    int address = OPERAND1;
    if (address > 0x7f)
    {
        aCPU->mSFR[address - 0x80] |= ACC;
        aCPU->sfrwrite(aCPU, address);
    }
    else
    {
        aCPU->mData[address] |= ACC;
    }
    PC += 2;
    return 3;
}

static int orl_mem_imm(em8051 *aCPU)
{
    int address = OPERAND1;
    if (address > 0x7f)
    {
        aCPU->mSFR[address - 0x80] |= OPERAND2;
        aCPU->sfrwrite(aCPU, address);
    }
    else
    {
        aCPU->mData[address] |= OPERAND2;
    }
    
    PC += 3;
    return 4;
}

static int orl_a_imm(em8051 *aCPU)
{
    ACC |= OPERAND1;
    PC += 2;
    return 2;
}

static int orl_a_mem(em8051 *aCPU)
{
    int value = read_mem(aCPU, OPERAND1);
    ACC |= value;
    PC += 2;
    return 2;
}

static int orl_a_indir_rx(em8051 *aCPU)
{
    int address = INDIR_RX_ADDRESS;
    ACC |= aCPU->mData[address];
    PC++;
    return 2;
}


static int jnc_offset(em8051 *aCPU)
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

static int anl_mem_a(em8051 *aCPU)
{
    int address = OPERAND1;
    if (address > 0x7f)
    {
        aCPU->mSFR[address - 0x80] &= ACC;
        aCPU->sfrwrite(aCPU, address);
    }
    else
    {
        aCPU->mData[address] &= ACC;
    }
    PC += 2;
    return 3;
}

static int anl_mem_imm(em8051 *aCPU)
{
    int address = OPERAND1;
    if (address > 0x7f)
    {
        aCPU->mSFR[address - 0x80] &= OPERAND2;
        aCPU->sfrwrite(aCPU, address);
    }
    else
    {
        aCPU->mData[address] &= OPERAND2;
    }
    PC += 3;
    return 4;
}

static int anl_a_imm(em8051 *aCPU)
{
    ACC &= OPERAND1;
    PC += 2;
    return 2;
}

static int anl_a_mem(em8051 *aCPU)
{
    int value = read_mem(aCPU, OPERAND1);
    ACC &= value;
    PC += 2;
    return 2;
}

static int anl_a_indir_rx(em8051 *aCPU)
{
    int address = INDIR_RX_ADDRESS;
    ACC &= aCPU->mData[address];
    PC++;
    return 2;
}


static int jz_offset(em8051 *aCPU)
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

static int xrl_mem_a(em8051 *aCPU)
{
    int address = OPERAND1;
    if (address > 0x7f)
    {
        aCPU->mSFR[address - 0x80] ^= ACC;
        aCPU->sfrwrite(aCPU, address);
    }
    else
    {
        aCPU->mData[address] ^= ACC;
    }
    PC += 2;
    return 3;
}

static int xrl_mem_imm(em8051 *aCPU)
{
    int address = OPERAND1;
    if (address > 0x7f)
    {
        aCPU->mSFR[address - 0x80] ^= OPERAND2;
        aCPU->sfrwrite(aCPU, address);
    }
    else
    {
        aCPU->mData[address] ^= OPERAND2;
    }
    PC += 3;
    return 4;
}

static int xrl_a_imm(em8051 *aCPU)
{
    ACC ^= OPERAND1;
    PC += 2;
    return 2;
}

static int xrl_a_mem(em8051 *aCPU)
{
    int value = read_mem(aCPU, OPERAND1);
    ACC ^= value;
    PC += 2;
    return 2;
}

static int xrl_a_indir_rx(em8051 *aCPU)
{
    int address = INDIR_RX_ADDRESS;
    ACC ^= aCPU->mData[address];
    PC++;
    return 2;
}


static int jnz_offset(em8051 *aCPU)
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

static int orl_c_bitaddr(em8051 *aCPU)
{
    int address = OPERAND1;
    int carry = CARRY;
    if (address > 0x7f)
    {
        int bit = address & 7;
        int bitmask = (1 << bit);
        int value;
        address &= 0xf8;        
        value = aCPU->sfrread(aCPU, address);
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
        value = (aCPU->mData[address] & bitmask) ? 1 : carry;
        PSW = (PSW & ~PSWMASK_C) | (PSWMASK_C * value);
    }
    PC += 2;
    return 2;
}

static int jmp_indir_a_dptr(em8051 *aCPU)
{
    PC = ((aCPU->mSFR[CUR_DPH] << 8) | (aCPU->mSFR[CUR_DPL])) + ACC;
    return 2;
}

static int mov_a_imm(em8051 *aCPU)
{
    ACC = OPERAND1;
    PC += 2;
    return 2;
}

static int mov_mem_imm(em8051 *aCPU)
{
    int address = OPERAND1;
    if (address > 0x7f)
    {
        aCPU->mSFR[address - 0x80] = OPERAND2;
        aCPU->sfrwrite(aCPU, address);
    }
    else
    {
        aCPU->mData[address] = OPERAND2;
    }

    PC += 3;
    return 3;
}

static int mov_indir_rx_imm(em8051 *aCPU)
{
    int address = INDIR_RX_ADDRESS;
    int value = OPERAND1;
    aCPU->mData[address] = value;
    PC += 2;
    return 3;
}


static int sjmp_offset(em8051 *aCPU)
{
    PC += (signed char)(OPERAND1) + 2;
    return 3;
}

static int anl_c_bitaddr(em8051 *aCPU)
{
    int address = OPERAND1;
    int carry = CARRY;
    if (address > 0x7f)
    {
        int bit = address & 7;
        int bitmask = (1 << bit);
        int value;
        address &= 0xf8;        
        value = aCPU->sfrread(aCPU, address);
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
        value = (aCPU->mData[address] & bitmask) ? carry : 0;
        PSW = (PSW & ~PSWMASK_C) | (PSWMASK_C * value);
    }
    PC += 2;
    return 2;
}

static int movc_a_indir_a_pc(em8051 *aCPU)
{
    int address = PC + 1 + ACC;
    address &= CODE_SIZE - 1;

#ifdef PROFILE_MOVC
    aCPU->mProfilerMem[address].total_cycles++;
#endif

    ACC = aCPU->mCodeMem[address];
    PC++;
    return 3;
}

static int div_ab(em8051 *aCPU)
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

static int mov_mem_mem(em8051 *aCPU)
{
    int address1 = OPERAND2;
    int value = read_mem(aCPU, OPERAND1);

    if (address1 > 0x7f)
    {
        aCPU->mSFR[address1 - 0x80] = value;
        aCPU->sfrwrite(aCPU, address1);
    }
    else
    {
        aCPU->mData[address1] = value;
    }

    PC += 3;
    return 4;
}

static int mov_mem_indir_rx(em8051 *aCPU)
{
    int address1 = OPERAND1;
    int address2 = INDIR_RX_ADDRESS;
    aCPU->mData[address1] = aCPU->mData[address2];
    PC += 2;
    return 4;
}


static int mov_dptr_imm(em8051 *aCPU)
{
    aCPU->mSFR[CUR_DPH] = OPERAND1;
    aCPU->mSFR[CUR_DPL] = OPERAND2;
    PC += 3;
    return 3;
}

static int mov_bitaddr_c(em8051 *aCPU) 
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
        aCPU->sfrwrite(aCPU, address);
    }
    else
    {
        int bit = address & 7;
        int bitmask = (1 << bit);
        address >>= 3;
        address += 0x20;
        aCPU->mData[address] = (aCPU->mData[address] & ~bitmask) | (carry << bit);
    }
    PC += 2;
    return 3;
}

static int movc_a_indir_a_dptr(em8051 *aCPU)
{
    int address = ((aCPU->mSFR[CUR_DPH] << 8) | (aCPU->mSFR[CUR_DPL] << 0)) + ACC;
    address &= CODE_SIZE - 1;

#ifdef PROFILE_MOVC
    aCPU->mProfilerMem[address].total_cycles++;
#endif

    ACC = aCPU->mCodeMem[address];
    PC++;
    return 3;
}

static int subb_a_imm(em8051 *aCPU)
{
    int carry = CARRY;
    sub_solve_flags(aCPU, ACC, OPERAND1 + carry);
    ACC -= OPERAND1 + carry;
    PC += 2;
    return 2;
}

static int subb_a_mem(em8051 *aCPU) 
{
    int carry = CARRY;
    int value = read_mem(aCPU, OPERAND1) + carry;
    sub_solve_flags(aCPU, ACC, value);
    ACC -= value;

    PC += 2;
    return 2;
}
static int subb_a_indir_rx(em8051 *aCPU)
{
    int carry = CARRY;
    int address = INDIR_RX_ADDRESS;
    sub_solve_flags(aCPU, ACC, aCPU->mData[address] + carry);
    ACC -= aCPU->mData[address] + carry;
    PC++;
    return 2;
}


static int orl_c_compl_bitaddr(em8051 *aCPU)
{
    int address = OPERAND1;
    int carry = CARRY;
    if (address > 0x7f)
    {
        int bit = address & 7;
        int bitmask = (1 << bit);
        int value;
        address &= 0xf8;        
        value = aCPU->sfrread(aCPU, address);
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
        value = (aCPU->mData[address] & bitmask) ? carry : 1;
        PSW = (PSW & ~PSWMASK_C) | (PSWMASK_C * value);
    }
    PC += 2;
    return 2;
}

static int mov_c_bitaddr(em8051 *aCPU) 
{
    int address = OPERAND1;
    if (address > 0x7f)
    {
        int bit = address & 7;
        int bitmask = (1 << bit);
        int value;
        address &= 0xf8;        
        value = aCPU->sfrread(aCPU, address);
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
        value = (aCPU->mData[address] & bitmask) ? 1 : 0;
        PSW = (PSW & ~PSWMASK_C) | (PSWMASK_C * value);
    }

    PC += 2;
    return 2;
}

static int inc_dptr(em8051 *aCPU)
{
    aCPU->mSFR[CUR_DPL]++;
    if (!aCPU->mSFR[CUR_DPL])
        aCPU->mSFR[CUR_DPH]++;
    PC++;
    return 1;
}

static int mul_ab(em8051 *aCPU)
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

static int mov_indir_rx_mem(em8051 *aCPU)
{
    int address1 = INDIR_RX_ADDRESS;
    int value = read_mem(aCPU, OPERAND1);
    aCPU->mData[address1] = value;
    PC += 2;
    return 5;
}


static int anl_c_compl_bitaddr(em8051 *aCPU)
{
    int address = OPERAND1;
    int carry = CARRY;
    if (address > 0x7f)
    {
        int bit = address & 7;
        int bitmask = (1 << bit);
        int value;
        address &= 0xf8;        
        value = aCPU->sfrread(aCPU, address);
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
        value = (aCPU->mData[address] & bitmask) ? 0 : carry;
        PSW = (PSW & ~PSWMASK_C) | (PSWMASK_C * value);
    }
    PC += 2;
    return 2;
}


static int cpl_bitaddr(em8051 *aCPU)
{
    int address = aCPU->mCodeMem[(PC + 1) & (CODE_SIZE - 1)];
    if (address > 0x7f)
    {
        // Data sheet does not explicitly say that the modification source
        // is read from output latch, but we'll assume that is what happens.
        int bit = address & 7;
        int bitmask = (1 << bit);
        address &= 0xf8;        
        aCPU->mSFR[address - 0x80] ^= bitmask;
        aCPU->sfrwrite(aCPU, address);
    }
    else
    {
        int bit = address & 7;
        int bitmask = (1 << bit);
        address >>= 3;
        address += 0x20;
        aCPU->mData[address] ^= bitmask;
    }
    PC += 2;
    return 3;
}

static int cpl_c(em8051 *aCPU)
{
    PSW ^= PSWMASK_C;
    PC++;
    return 1;
}

static int cjne_a_imm_offset(em8051 *aCPU)
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

static int cjne_a_mem_offset(em8051 *aCPU)
{
    int address = OPERAND1;
    int value;
    if (address > 0x7f)
    {
        value = aCPU->sfrread(aCPU, address);
    }
    else
    {
        value = aCPU->mData[address];
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
static int cjne_indir_rx_imm_offset(em8051 *aCPU)
{
    int address = INDIR_RX_ADDRESS;
    int value1 = BAD_VALUE;
    int value2 = OPERAND1;
    value1 = aCPU->mData[address];

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

static int push_mem(em8051 *aCPU)
{
    int value = read_mem(aCPU, OPERAND1);
    em8051_push(aCPU, value);   
    PC += 2;
    return 4;
}


static int clr_bitaddr(em8051 *aCPU)
{
    int address = aCPU->mCodeMem[(PC + 1) & (CODE_SIZE - 1)];
    if (address > 0x7f)
    {
        // Data sheet does not explicitly say that the modification source
        // is read from output latch, but we'll assume that is what happens.
        int bit = address & 7;
        int bitmask = (1 << bit);
        address &= 0xf8;        
        aCPU->mSFR[address - 0x80] &= ~bitmask;
        aCPU->sfrwrite(aCPU, address);
    }
    else
    {
        int bit = address & 7;
        int bitmask = (1 << bit);
        address >>= 3;
        address += 0x20;
        aCPU->mData[address] &= ~bitmask;
    }
    PC += 2;
    return 3;
}

static int clr_c(em8051 *aCPU)
{
    PSW &= ~PSWMASK_C;
    PC++;
    return 1;
}

static int swap_a(em8051 *aCPU)
{
    ACC = (ACC << 4) | (ACC >> 4);
    PC++;
    return 1;
}

static int xch_a_mem(em8051 *aCPU)
{
    int address = OPERAND1;
    int value = read_mem(aCPU, OPERAND1);
    if (address > 0x7f)
    {
        aCPU->mSFR[address - 0x80] = ACC;
        ACC = value;
        aCPU->sfrwrite(aCPU, address);
    }
    else
    {
        aCPU->mData[address] = ACC;
        ACC = value;
    }
    PC += 2;
    return 3;
}

static int xch_a_indir_rx(em8051 *aCPU)
{
    int address = INDIR_RX_ADDRESS;
    int value = aCPU->mData[address];
    aCPU->mData[address] = ACC;
    ACC = value;
    PC++;
    return 3;
}


static int pop_mem(em8051 *aCPU)
{
    int address = OPERAND1;
    if (address > 0x7f)
    {
        aCPU->mSFR[address - 0x80] = pop_from_stack(aCPU);
        aCPU->sfrwrite(aCPU, address);
    }
    else
    {
        aCPU->mData[address] = pop_from_stack(aCPU);
    }

    PC += 2;
    return 3;
}

static int setb_bitaddr(em8051 *aCPU)
{
    int address = aCPU->mCodeMem[(PC + 1) & (CODE_SIZE - 1)];
    if (address > 0x7f)
    {
        // Data sheet does not explicitly say that the modification source
        // is read from output latch, but we'll assume that is what happens.
        int bit = address & 7;
        int bitmask = (1 << bit);
        address &= 0xf8;        
        aCPU->mSFR[address - 0x80] |= bitmask;
        aCPU->sfrwrite(aCPU, address);
    }
    else
    {
        int bit = address & 7;
        int bitmask = (1 << bit);
        address >>= 3;
        address += 0x20;
        aCPU->mData[address] |= bitmask;
    }
    PC += 2;
    return 3;
}

static int setb_c(em8051 *aCPU)
{
    PSW |= PSWMASK_C;
    PC++;
    return 1;
}

static int da_a(em8051 *aCPU)
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

static int djnz_mem_offset(em8051 *aCPU)
{
    int address = OPERAND1;
    int value;
    if (address > 0x7f)
    {
        aCPU->mSFR[address - 0x80]--;
        value = aCPU->mSFR[address - 0x80];
        aCPU->sfrwrite(aCPU, address);
    }
    else
    {
        aCPU->mData[address]--;
        value = aCPU->mData[address];
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

static int xchd_a_indir_rx(em8051 *aCPU)
{
    int address = INDIR_RX_ADDRESS;
    int value = aCPU->mData[address];
    aCPU->mData[address] = (aCPU->mData[address] & 0x0f) | (ACC & 0x0f);
    ACC = (ACC & 0xf0) | (value & 0x0f);
    PC++;
    return 3;
}


static int movx_a_indir_dptr(em8051 *aCPU)
{
    int dptr = (aCPU->mSFR[CUR_DPH] << 8) | aCPU->mSFR[CUR_DPL];

    if (dptr >= XDATA_SIZE)
        aCPU->except(aCPU, EXCEPTION_XDATA_ERROR);
    else
        ACC = aCPU->mExtData[dptr];

    PC++;
    return 4;
}

static int movx_a_indir_rx(em8051 *aCPU)
{
    int address = INDIR_RX_ADDRESS;

    if (address >= XDATA_SIZE)
        aCPU->except(aCPU, EXCEPTION_XDATA_ERROR);
    else
        ACC = aCPU->mExtData[address];

    PC++;
    return 4;
}

static int clr_a(em8051 *aCPU)
{
    ACC = 0;
    PC++;
    return 1;
}

static int mov_a_mem(em8051 *aCPU)
{
    // mov a,acc is not a valid instruction
    int address = OPERAND1;
    int value = read_mem(aCPU, address);
    if (REG_ACC == address - 0x80)
        aCPU->except(aCPU, EXCEPTION_ACC_TO_A);
    ACC = value;

    PC += 2;
    return 2;
}

static int mov_a_indir_rx(em8051 *aCPU)
{
    int address = INDIR_RX_ADDRESS;
    ACC = aCPU->mData[address];
    PC++;
    return 2;
}


static int movx_indir_dptr_a(em8051 *aCPU)
{
    int dptr = (aCPU->mSFR[CUR_DPH] << 8) | aCPU->mSFR[CUR_DPL];

    if (dptr >= XDATA_SIZE)
        aCPU->except(aCPU, EXCEPTION_XDATA_ERROR);
    else
        aCPU->mExtData[dptr] = ACC;

    PC++;
    return 5;
}

static int movx_indir_rx_a(em8051 *aCPU)
{
    int address = INDIR_RX_ADDRESS;

    if (address >= XDATA_SIZE)
        aCPU->except(aCPU, EXCEPTION_XDATA_ERROR);
    else
        aCPU->mExtData[address] = ACC;

    PC++;
    return 5;
}

static int cpl_a(em8051 *aCPU)
{
    ACC = ~ACC;
    PC++;
    return 1;
}

static int mov_mem_a(em8051 *aCPU)
{
    int address = OPERAND1;
    if (address > 0x7f)
    {
        aCPU->mSFR[address - 0x80] = ACC;
        aCPU->sfrwrite(aCPU, address);
    }
    else
    {
        aCPU->mData[address] = ACC;
    }
    PC += 2;
    return 3;
}

static int mov_indir_rx_a(em8051 *aCPU)
{
    int address = INDIR_RX_ADDRESS;
    aCPU->mData[address] = ACC;
    PC++;
    return 3;
}

static int nop(em8051 *aCPU)
{
    if (aCPU->mCodeMem[PC & (CODE_SIZE - 1)] != 0)
        aCPU->except(aCPU, EXCEPTION_ILLEGAL_OPCODE);
    PC++;
    return 1;
}

static int inc_rx(em8051 *aCPU)
{
    int rx = RX_ADDRESS;
    aCPU->mData[rx]++;
    PC++;
    return 2;
}

static int dec_rx(em8051 *aCPU)
{
    int rx = RX_ADDRESS;
    aCPU->mData[rx]--;
    PC++;
    return 2;
}

static int add_a_rx(em8051 *aCPU)
{
    int rx = RX_ADDRESS;
    add_solve_flags(aCPU, aCPU->mData[rx], ACC, 0);
    ACC += aCPU->mData[rx];
    PC++;
    return 1;
}

static int addc_a_rx(em8051 *aCPU)
{
    int rx = RX_ADDRESS;
    int carry = CARRY;
    add_solve_flags(aCPU, aCPU->mData[rx], ACC, carry);
    ACC += aCPU->mData[rx] + carry;
    PC++;
    return 1;
}

static int orl_a_rx(em8051 *aCPU)
{
    int rx = RX_ADDRESS;
    ACC |= aCPU->mData[rx];
    PC++;
    return 1;
}

static int anl_a_rx(em8051 *aCPU)
{
    int rx = RX_ADDRESS;
    ACC &= aCPU->mData[rx];
    PC++;
    return 1;
}

static int xrl_a_rx(em8051 *aCPU)
{
    int rx = RX_ADDRESS;
    ACC ^= aCPU->mData[rx];    
    PC++;
    return 1;
}


static int mov_rx_imm(em8051 *aCPU)
{
    int rx = RX_ADDRESS;
    aCPU->mData[rx] = OPERAND1;
    PC += 2;
    return 2;
}

static int mov_mem_rx(em8051 *aCPU)
{
    int rx = RX_ADDRESS;
    int address = OPERAND1;
    if (address > 0x7f)
    {
        aCPU->mSFR[address - 0x80] = aCPU->mData[rx];
        aCPU->sfrwrite(aCPU, address);
    }
    else
    {
        aCPU->mData[address] = aCPU->mData[rx];
    }
    PC += 2;
    return 3;
}

static int subb_a_rx(em8051 *aCPU)
{
    int rx = RX_ADDRESS;
    int carry = CARRY;
    sub_solve_flags(aCPU, ACC, aCPU->mData[rx] + carry);
    ACC -= aCPU->mData[rx] + carry;
    PC++;
    return 1;
}

static int mov_rx_mem(em8051 *aCPU)
{
    int rx = RX_ADDRESS;
    int value = read_mem(aCPU, OPERAND1);
    aCPU->mData[rx] = value;

    PC += 2;
    return 4;
}

static int cjne_rx_imm_offset(em8051 *aCPU)
{
    int rx = RX_ADDRESS;
    int value = OPERAND1;
    
    if (aCPU->mData[rx] < value)
    {
        PSW |= PSWMASK_C;
    }
    else
    {
        PSW &= ~PSWMASK_C;
    }

    if (aCPU->mData[rx] != value)
    {
        PC += (signed char)OPERAND2 + 3;
    }
    else
    {
        PC += 3;
    }
    return 4;
}

static int xch_a_rx(em8051 *aCPU)
{
    int rx = RX_ADDRESS;
    int a = ACC;
    ACC = aCPU->mData[rx];
    aCPU->mData[rx] = a;
    PC++;
    return 2;
}

static int djnz_rx_offset(em8051 *aCPU)
{
    int rx = RX_ADDRESS;
    aCPU->mData[rx]--;
    if (aCPU->mData[rx])
    {
        PC += (signed char)OPERAND1 + 2;
    }
    else
    {
        PC += 2;
    }
    return 3;
}

static int mov_a_rx(em8051 *aCPU)
{
    int rx = RX_ADDRESS;
    ACC = aCPU->mData[rx];

    PC++;
    return 1;
}

static int mov_rx_a(em8051 *aCPU)
{
    int rx = RX_ADDRESS;
    aCPU->mData[rx] = ACC;
    PC++;
    return 2;
}

void op_setptrs(em8051 *aCPU)
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


};  // namespace CPU
};  // namespace Cube

