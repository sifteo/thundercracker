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
 * opcodes.c
 * 8051 opcode simulation functions
 */

#ifndef _CUBE_CPU_OPCODES_H
#define _CUBE_CPU_OPCODES_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cube_cpu.h"
#include "cube_cpu_callbacks.h"

namespace Cube {
namespace CPU {

struct Opcodes {

#define PSW aCPU->mSFR[REG_PSW]
#define ACC aCPU->mSFR[REG_ACC]
#define INDIR_RX_ADDRESS    (aCPU->mData[(opcode & 1) + 8 * ((PSW & (PSWMASK_RS0|PSWMASK_RS1))>>PSW_RS0)])
#define RX_ADDRESS          ((opcode & 7) + 8 * ((PSW & (PSWMASK_RS0|PSWMASK_RS1))>>PSW_RS0))
#define CARRY               ((PSW & PSWMASK_C) >> PSW_C)
#define INDIR_RX_ADDRESS_X  (INDIR_RX_ADDRESS | (aCPU->mSFR[REG_MPAGE] << 8))


static ALWAYS_INLINE FASTCALL int read_mem(em8051 *aCPU, int aAddress)
{
    if (aAddress > 0x7f)
        return SFR::read(aCPU, aAddress);
    else
        return aCPU->mData[aAddress];
}

static ALWAYS_INLINE void push_word_to_stack(em8051 *aCPU, uint16_t aValue)
{
    unsigned sp = aCPU->mSFR[REG_SP];

    if (sp + 2 >= 0x100) {
        except(aCPU, EXCEPTION_STACK);
        return;
    }

    aCPU->mData[sp+1] = aValue;
    aCPU->mData[sp+2] = aValue >> 8;

    aCPU->mSFR[REG_SP] = sp + 2;
}

static ALWAYS_INLINE void push_to_stack(em8051 *aCPU, int aValue)
{
    unsigned sp = aCPU->mSFR[REG_SP];

    if (sp + 1 >= 0x100) {
        except(aCPU, EXCEPTION_STACK);
        return;
    }

    aCPU->mData[sp+1] = aValue;

    aCPU->mSFR[REG_SP] = sp + 1;
}

static ALWAYS_INLINE FASTCALL int pop_word_from_stack(em8051 *aCPU)
{
    unsigned sp = aCPU->mSFR[REG_SP];
    
    if (sp < 2) {
        except(aCPU, EXCEPTION_STACK);
        return 0;
    }

    uint16_t value = aCPU->mData[sp];
    value = (value << 8) | aCPU->mData[sp-1];

    aCPU->mSFR[REG_SP] = sp - 2;

    return value;
}

static ALWAYS_INLINE FASTCALL int pop_from_stack(em8051 *aCPU)
{
    unsigned sp = aCPU->mSFR[REG_SP];
    
    if (sp < 1) {
        except(aCPU, EXCEPTION_STACK);
        return 0;
    }

    uint8_t value = aCPU->mData[sp];

    aCPU->mSFR[REG_SP] = sp - 1;

    return value;
}


static ALWAYS_INLINE void add_solve_flags(em8051 * aCPU, int value1, int value2, int acc)
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

static ALWAYS_INLINE void sub_solve_flags(em8051 * aCPU, int value1, int value2)
{
    int carry = (((value1 & 255) - (value2 & 255)) >> 8) & 1;
    int auxcarry = (((value1 & 7) - (value2 & 7)) >> 3) & 1;
    int overflow = ((((value1 & 127) - (value2 & 127)) >> 7) & 1)^carry;
    PSW = (PSW & ~(PSWMASK_C|PSWMASK_AC|PSWMASK_OV)) |
                          (carry << PSW_C) | (auxcarry << PSW_AC) | (overflow << PSW_OV);
}

static ALWAYS_INLINE uint8_t xdata_read(em8051 *aCPU, uint16_t dptr)
{    
    /*
     * Read from XDATA (RAM or NVM), returns an 8-bit data byte
     */

    if (LIKELY(dptr < XDATA_SIZE))
        return aCPU->mExtData[dptr];
        
    uint16_t nvm_addr = dptr - NVM_BASE;
    if (LIKELY(nvm_addr < NVM_SIZE))
        return NVM::read(aCPU, nvm_addr);
        
    except(aCPU, EXCEPTION_XDATA_ERROR);
    return 0xFF;
}

static ALWAYS_INLINE int xdata_write(em8051 *aCPU, uint16_t dptr, uint8_t value)
{    
    /*
     * Write a byte to XDATA (RAM or NVM). Returns the number of extra delay cycles.
     */

    if (LIKELY(dptr < XDATA_SIZE)) {
        aCPU->mExtData[dptr] = value;
        return 0;
    }
        
    uint16_t nvm_addr = dptr - NVM_BASE;
    if (LIKELY(nvm_addr < NVM_SIZE))
        return NVM::write(aCPU, nvm_addr, value);
        
    except(aCPU, EXCEPTION_XDATA_ERROR);
    return 0;
}

static ALWAYS_INLINE FASTCALL int ajmp_offset(em8051 *aCPU, unsigned &PC, uint8_t opcode, uint8_t operand1, uint8_t operand2)
{
    int address = ((PC + 2) & 0xf800) |
                  operand1 | 
                  ((opcode & 0xe0) << 3);

    PC = address;

    return 3;
}

static ALWAYS_INLINE FASTCALL int ljmp_address(em8051 *aCPU, unsigned &PC, uint8_t opcode, uint8_t operand1, uint8_t operand2)
{
    int address = (operand1 << 8) | operand2;
    PC = address;

    return 4;
}


static ALWAYS_INLINE FASTCALL int rr_a(em8051 *aCPU, unsigned &PC, uint8_t opcode, uint8_t operand1, uint8_t operand2)
{
    ACC = (ACC >> 1) | (ACC << 7);
    PC++;
    return 1;
}

static ALWAYS_INLINE FASTCALL int inc_a(em8051 *aCPU, unsigned &PC, uint8_t opcode, uint8_t operand1, uint8_t operand2)
{
    ACC++;
    PC++;
    return 1;
}

static ALWAYS_INLINE FASTCALL int inc_mem(em8051 *aCPU, unsigned &PC, uint8_t opcode, uint8_t operand1, uint8_t operand2)
{
    int address = operand1;
    if (address > 0x7f)
    {
        aCPU->mSFR[address - 0x80]++;
        SFR::write(aCPU, address);
    }
    else
    {
        aCPU->mData[address]++;
    }
    PC += 2;
    return 3;
}

static ALWAYS_INLINE FASTCALL int inc_indir_rx(em8051 *aCPU, unsigned &PC, uint8_t opcode, uint8_t operand1, uint8_t operand2)
{    
    int address = INDIR_RX_ADDRESS;
    aCPU->mData[address]++;
    PC++;
    return 3;
}

static ALWAYS_INLINE FASTCALL int jbc_bitaddr_offset(em8051 *aCPU, unsigned &PC, uint8_t opcode, uint8_t operand1, uint8_t operand2)
{
    // "Note: when this instruction is used to test an output pin, the value used 
    // as the original data will be read from the output data latch, not the input pin"
    int address = operand1;
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
            PC += (signed char)operand2 + 3;
            SFR::write(aCPU, address);
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
            PC += (signed char)operand2 + 3;
        }
        else
        {
            PC += 3;
        }
    }
    return 4;
}

static ALWAYS_INLINE FASTCALL int acall_offset(em8051 *aCPU, unsigned &PC, uint8_t opcode, uint8_t operand1, uint8_t operand2)
{
    int address = ((PC + 2) & 0xf800) | operand1 | ((opcode & 0xe0) << 3);
    push_word_to_stack(aCPU, PC + 2);
    PC = address;
    return 6;
}

static ALWAYS_INLINE FASTCALL int lcall_address(em8051 *aCPU, unsigned &PC, uint8_t opcode, uint8_t operand1, uint8_t operand2)
{
    push_word_to_stack(aCPU, PC + 3);
    PC = (operand1 << 8) | operand2;
    return 6;
}

static ALWAYS_INLINE FASTCALL int rrc_a(em8051 *aCPU, unsigned &PC, uint8_t opcode, uint8_t operand1, uint8_t operand2)
{
    int c = (PSW & PSWMASK_C) >> PSW_C;
    int newc = ACC & 1;
    ACC = (ACC >> 1) | (c << 7);
    PSW = (PSW & ~PSWMASK_C) | (newc << PSW_C);
    PC++;
    return 1;
}

static ALWAYS_INLINE FASTCALL int dec_a(em8051 *aCPU, unsigned &PC, uint8_t opcode, uint8_t operand1, uint8_t operand2)
{
    ACC--;
    PC++;
    return 1;
}

static ALWAYS_INLINE FASTCALL int dec_mem(em8051 *aCPU, unsigned &PC, uint8_t opcode, uint8_t operand1, uint8_t operand2)
{
    int address = operand1;
    if (address > 0x7f)
    {
        aCPU->mSFR[address - 0x80]--;
        SFR::write(aCPU, address);
    }
    else
    {
        aCPU->mData[address]--;
    }
    PC += 2;
    return 3;
}

static ALWAYS_INLINE FASTCALL int dec_indir_rx(em8051 *aCPU, unsigned &PC, uint8_t opcode, uint8_t operand1, uint8_t operand2)
{
    int address = INDIR_RX_ADDRESS;
    aCPU->mData[address]--;
    PC++;
    return 3;
}


static ALWAYS_INLINE FASTCALL int jb_bitaddr_offset(em8051 *aCPU, unsigned &PC, uint8_t opcode, uint8_t operand1, uint8_t operand2)
{
    int address = operand1;
    if (address > 0x7f)
    {
        int bit = address & 7;
        int bitmask = (1 << bit);
        int value;
        address &= 0xf8;  
        value = SFR::read(aCPU, address);

        if (value & bitmask)
        {
            PC += (signed char)operand2 + 3;
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
            PC += (signed char)operand2 + 3;
        }
        else
        {
            PC += 3;
        }
    }
    return 4;
}

static ALWAYS_INLINE FASTCALL int ret(em8051 *aCPU, unsigned &PC, uint8_t opcode, uint8_t operand1, uint8_t operand2)
{
    PC = pop_word_from_stack(aCPU);
    return 4;
}

static ALWAYS_INLINE FASTCALL int rl_a(em8051 *aCPU, unsigned &PC, uint8_t opcode, uint8_t operand1, uint8_t operand2)
{
    ACC = (ACC << 1) | (ACC >> 7);
    PC++;
    return 1;
}

static ALWAYS_INLINE FASTCALL int add_a_imm(em8051 *aCPU, unsigned &PC, uint8_t opcode, uint8_t operand1, uint8_t operand2)
{
    add_solve_flags(aCPU, ACC, operand1, 0);
    ACC += operand1;
    PC += 2;
    return 2;
}

static ALWAYS_INLINE FASTCALL int add_a_mem(em8051 *aCPU, unsigned &PC, uint8_t opcode, uint8_t operand1, uint8_t operand2)
{
    int value = read_mem(aCPU, operand1);
    add_solve_flags(aCPU, ACC, value, 0);
    ACC += value;
        PC += 2;
    return 2;
}

static ALWAYS_INLINE FASTCALL int add_a_indir_rx(em8051 *aCPU, unsigned &PC, uint8_t opcode, uint8_t operand1, uint8_t operand2)
{
    int address = INDIR_RX_ADDRESS;
    add_solve_flags(aCPU, ACC, aCPU->mData[address], 0);
    ACC += aCPU->mData[address];
    PC++;
    return 2;
}

static ALWAYS_INLINE FASTCALL int jnb_bitaddr_offset(em8051 *aCPU, unsigned &PC, uint8_t opcode, uint8_t operand1, uint8_t operand2)
{
    int address = operand1;
    if (address > 0x7f)
    {
        int bit = address & 7;
        int bitmask = (1 << bit);
        int value;
        address &= 0xf8;        
        value = SFR::read(aCPU, address);
        
        if (!(value & bitmask))
        {
            PC += (signed char)operand2 + 3;
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
            PC += (signed char)operand2 + 3;
        }
        else
        {
            PC += 3;
        }
    }
    return 4;
}

static ALWAYS_INLINE FASTCALL int reti(em8051 *aCPU, unsigned &PC, uint8_t opcode, uint8_t operand1, uint8_t operand2)
{
    // Time to execute the RETI instruction itself
    int cycles = 4;

    if (aCPU->irq_count)
    {
        int i = --aCPU->irq_count;

        // Sanity check
        irq_check(aCPU, i);
            
        // Resume the basic block we preempted
        cycles += aCPU->irql[i].tickDelay;
    }
    
    // We may need to fire a higher-priority pending interrupt that was delayed
    aCPU->needInterruptDispatch = true;

    PC = pop_word_from_stack(aCPU);
    
    return cycles;
}

static ALWAYS_INLINE FASTCALL int rlc_a(em8051 *aCPU, unsigned &PC, uint8_t opcode, uint8_t operand1, uint8_t operand2)
{
    int c = CARRY;
    int newc = ACC >> 7;
    ACC = (ACC << 1) | c;
    PSW = (PSW & ~PSWMASK_C) | (newc << PSW_C);
    PC++;
    return 1;
}

static ALWAYS_INLINE FASTCALL int addc_a_imm(em8051 *aCPU, unsigned &PC, uint8_t opcode, uint8_t operand1, uint8_t operand2)
{
    int carry = CARRY;
    add_solve_flags(aCPU, ACC, operand1, carry);
    ACC += operand1 + carry;
    PC += 2;
    return 2;
}

static ALWAYS_INLINE FASTCALL int addc_a_mem(em8051 *aCPU, unsigned &PC, uint8_t opcode, uint8_t operand1, uint8_t operand2)
{
    int carry = CARRY;
    int value = read_mem(aCPU, operand1);
    add_solve_flags(aCPU, ACC, value, carry);
    ACC += value + carry;
    PC += 2;
    return 2;
}

static ALWAYS_INLINE FASTCALL int addc_a_indir_rx(em8051 *aCPU, unsigned &PC, uint8_t opcode, uint8_t operand1, uint8_t operand2)
{
    int carry = CARRY;
    int address = INDIR_RX_ADDRESS;
    add_solve_flags(aCPU, ACC, aCPU->mData[address], carry);
    ACC += aCPU->mData[address] + carry;
    PC++;
    return 2;
}


static ALWAYS_INLINE FASTCALL int jc_offset(em8051 *aCPU, unsigned &PC, uint8_t opcode, uint8_t operand1, uint8_t operand2)
{
    if (PSW & PSWMASK_C)
    {
        PC += (signed char)operand1 + 2;
    }
    else
    {
        PC += 2;
    }
    return 3;
}

static ALWAYS_INLINE FASTCALL int orl_mem_a(em8051 *aCPU, unsigned &PC, uint8_t opcode, uint8_t operand1, uint8_t operand2)
{
    int address = operand1;
    if (address > 0x7f)
    {
        aCPU->mSFR[address - 0x80] |= ACC;
        SFR::write(aCPU, address);
    }
    else
    {
        aCPU->mData[address] |= ACC;
    }
    PC += 2;
    return 3;
}

static ALWAYS_INLINE FASTCALL int orl_mem_imm(em8051 *aCPU, unsigned &PC, uint8_t opcode, uint8_t operand1, uint8_t operand2)
{
    int address = operand1;
    if (address > 0x7f)
    {
        aCPU->mSFR[address - 0x80] |= operand2;
        SFR::write(aCPU, address);
    }
    else
    {
        aCPU->mData[address] |= operand2;
    }
    
    PC += 3;
    return 4;
}

static ALWAYS_INLINE FASTCALL int orl_a_imm(em8051 *aCPU, unsigned &PC, uint8_t opcode, uint8_t operand1, uint8_t operand2)
{
    ACC |= operand1;
    PC += 2;
    return 2;
}

static ALWAYS_INLINE FASTCALL int orl_a_mem(em8051 *aCPU, unsigned &PC, uint8_t opcode, uint8_t operand1, uint8_t operand2)
{
    int value = read_mem(aCPU, operand1);
    ACC |= value;
    PC += 2;
    return 2;
}

static ALWAYS_INLINE FASTCALL int orl_a_indir_rx(em8051 *aCPU, unsigned &PC, uint8_t opcode, uint8_t operand1, uint8_t operand2)
{
    int address = INDIR_RX_ADDRESS;
    ACC |= aCPU->mData[address];
    PC++;
    return 2;
}


static ALWAYS_INLINE FASTCALL int jnc_offset(em8051 *aCPU, unsigned &PC, uint8_t opcode, uint8_t operand1, uint8_t operand2)
{
    if (PSW & PSWMASK_C)
    {
        PC += 2;
    }
    else
    {
        PC += (signed char)operand1 + 2;
    }
    return 3;
}

static ALWAYS_INLINE FASTCALL int anl_mem_a(em8051 *aCPU, unsigned &PC, uint8_t opcode, uint8_t operand1, uint8_t operand2)
{
    int address = operand1;
    if (address > 0x7f)
    {
        aCPU->mSFR[address - 0x80] &= ACC;
        SFR::write(aCPU, address);
    }
    else
    {
        aCPU->mData[address] &= ACC;
    }
    PC += 2;
    return 3;
}

static ALWAYS_INLINE FASTCALL int anl_mem_imm(em8051 *aCPU, unsigned &PC, uint8_t opcode, uint8_t operand1, uint8_t operand2)
{
    int address = operand1;
    if (address > 0x7f)
    {
        aCPU->mSFR[address - 0x80] &= operand2;
        SFR::write(aCPU, address);
    }
    else
    {
        aCPU->mData[address] &= operand2;
    }
    PC += 3;
    return 4;
}

static ALWAYS_INLINE FASTCALL int anl_a_imm(em8051 *aCPU, unsigned &PC, uint8_t opcode, uint8_t operand1, uint8_t operand2)
{
    ACC &= operand1;
    PC += 2;
    return 2;
}

static ALWAYS_INLINE FASTCALL int anl_a_mem(em8051 *aCPU, unsigned &PC, uint8_t opcode, uint8_t operand1, uint8_t operand2)
{
    int value = read_mem(aCPU, operand1);
    ACC &= value;
    PC += 2;
    return 2;
}

static ALWAYS_INLINE FASTCALL int anl_a_indir_rx(em8051 *aCPU, unsigned &PC, uint8_t opcode, uint8_t operand1, uint8_t operand2)
{
    int address = INDIR_RX_ADDRESS;
    ACC &= aCPU->mData[address];
    PC++;
    return 2;
}


static ALWAYS_INLINE FASTCALL int jz_offset(em8051 *aCPU, unsigned &PC, uint8_t opcode, uint8_t operand1, uint8_t operand2)
{
    if (!ACC)
    {
        PC += (signed char)operand1 + 2;
    }
    else
    {
        PC += 2;
    }
    return 3;
}

static ALWAYS_INLINE FASTCALL int xrl_mem_a(em8051 *aCPU, unsigned &PC, uint8_t opcode, uint8_t operand1, uint8_t operand2)
{
    int address = operand1;
    if (address > 0x7f)
    {
        aCPU->mSFR[address - 0x80] ^= ACC;
        SFR::write(aCPU, address);
    }
    else
    {
        aCPU->mData[address] ^= ACC;
    }
    PC += 2;
    return 3;
}

static ALWAYS_INLINE FASTCALL int xrl_mem_imm(em8051 *aCPU, unsigned &PC, uint8_t opcode, uint8_t operand1, uint8_t operand2)
{
    int address = operand1;
    if (address > 0x7f)
    {
        aCPU->mSFR[address - 0x80] ^= operand2;
        SFR::write(aCPU, address);
    }
    else
    {
        aCPU->mData[address] ^= operand2;
    }
    PC += 3;
    return 4;
}

static ALWAYS_INLINE FASTCALL int xrl_a_imm(em8051 *aCPU, unsigned &PC, uint8_t opcode, uint8_t operand1, uint8_t operand2)
{
    ACC ^= operand1;
    PC += 2;
    return 2;
}

static ALWAYS_INLINE FASTCALL int xrl_a_mem(em8051 *aCPU, unsigned &PC, uint8_t opcode, uint8_t operand1, uint8_t operand2)
{
    int value = read_mem(aCPU, operand1);
    ACC ^= value;
    PC += 2;
    return 2;
}

static ALWAYS_INLINE FASTCALL int xrl_a_indir_rx(em8051 *aCPU, unsigned &PC, uint8_t opcode, uint8_t operand1, uint8_t operand2)
{
    int address = INDIR_RX_ADDRESS;
    ACC ^= aCPU->mData[address];
    PC++;
    return 2;
}


static ALWAYS_INLINE FASTCALL int jnz_offset(em8051 *aCPU, unsigned &PC, uint8_t opcode, uint8_t operand1, uint8_t operand2)
{
    if (ACC)
    {
        PC += (signed char)operand1 + 2;
    }
    else
    {
        PC += 2;
    }
    return 3;
}

static ALWAYS_INLINE FASTCALL int orl_c_bitaddr(em8051 *aCPU, unsigned &PC, uint8_t opcode, uint8_t operand1, uint8_t operand2)
{
    int address = operand1;
    int carry = CARRY;
    if (address > 0x7f)
    {
        int bit = address & 7;
        int bitmask = (1 << bit);
        int value;
        address &= 0xf8;        
        value = SFR::read(aCPU, address);
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

static ALWAYS_INLINE FASTCALL int jmp_indir_a_dptr(em8051 *aCPU, unsigned &PC, uint8_t opcode, uint8_t operand1, uint8_t operand2)
{
    PC = ((aCPU->mSFR[CUR_DPH] << 8) | (aCPU->mSFR[CUR_DPL])) + ACC;
    return 2;
}

static ALWAYS_INLINE FASTCALL int mov_a_imm(em8051 *aCPU, unsigned &PC, uint8_t opcode, uint8_t operand1, uint8_t operand2)
{
    ACC = operand1;
    PC += 2;
    return 2;
}

static ALWAYS_INLINE FASTCALL int mov_mem_imm(em8051 *aCPU, unsigned &PC, uint8_t opcode, uint8_t operand1, uint8_t operand2)
{
    int address = operand1;
    if (address > 0x7f)
    {
        aCPU->mSFR[address - 0x80] = operand2;
        SFR::write(aCPU, address);
    }
    else
    {
        aCPU->mData[address] = operand2;
    }

    PC += 3;
    return 3;
}

static ALWAYS_INLINE FASTCALL int mov_indir_rx_imm(em8051 *aCPU, unsigned &PC, uint8_t opcode, uint8_t operand1, uint8_t operand2)
{
    int address = INDIR_RX_ADDRESS;
    int value = operand1;
    aCPU->mData[address] = value;
    PC += 2;
    return 3;
}


static ALWAYS_INLINE FASTCALL int sjmp_offset(em8051 *aCPU, unsigned &PC, uint8_t opcode, uint8_t operand1, uint8_t operand2)
{
    PC += (signed char)(operand1) + 2;
    return 3;
}

static ALWAYS_INLINE FASTCALL int anl_c_bitaddr(em8051 *aCPU, unsigned &PC, uint8_t opcode, uint8_t operand1, uint8_t operand2)
{
    int address = operand1;
    int carry = CARRY;
    if (address > 0x7f)
    {
        int bit = address & 7;
        int bitmask = (1 << bit);
        int value;
        address &= 0xf8;        
        value = SFR::read(aCPU, address);
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

static ALWAYS_INLINE FASTCALL int movc_a_indir_a_pc(em8051 *aCPU, unsigned &PC, uint8_t opcode, uint8_t operand1, uint8_t operand2)
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

static ALWAYS_INLINE FASTCALL int div_ab(em8051 *aCPU, unsigned &PC, uint8_t opcode, uint8_t operand1, uint8_t operand2)
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

static ALWAYS_INLINE FASTCALL int mov_mem_mem(em8051 *aCPU, unsigned &PC, uint8_t opcode, uint8_t operand1, uint8_t operand2)
{
    int address1 = operand2;
    int value = read_mem(aCPU, operand1);

    if (address1 > 0x7f)
    {
        aCPU->mSFR[address1 - 0x80] = value;
        SFR::write(aCPU, address1);
    }
    else
    {
        aCPU->mData[address1] = value;
    }

    PC += 3;
    return 4;
}

static ALWAYS_INLINE FASTCALL int mov_mem_indir_rx(em8051 *aCPU, unsigned &PC, uint8_t opcode, uint8_t operand1, uint8_t operand2)
{
    int address1 = operand1;
    int address2 = INDIR_RX_ADDRESS;

    if (address1 > 0x7f) {
        aCPU->mSFR[address1 - 0x80] = aCPU->mData[address2];
        SFR::write(aCPU, address1);
    } else {
        aCPU->mData[address1] = aCPU->mData[address2];
    }

    PC += 2;
    return 4;
}


static ALWAYS_INLINE FASTCALL int mov_dptr_imm(em8051 *aCPU, unsigned &PC, uint8_t opcode, uint8_t operand1, uint8_t operand2)
{
    aCPU->mSFR[CUR_DPH] = operand1;
    aCPU->mSFR[CUR_DPL] = operand2;
    PC += 3;
    return 3;
}

static ALWAYS_INLINE FASTCALL int mov_bitaddr_c(em8051 *aCPU, unsigned &PC, uint8_t opcode, uint8_t operand1, uint8_t operand2) 
{
    int address = operand1;
    int carry = CARRY;
    if (address > 0x7f)
    {
        // Data sheet does not explicitly say that the modification source
        // is read from output latch, but we'll assume that is what happens.
        int bit = address & 7;
        int bitmask = (1 << bit);
        address &= 0xf8;        
        aCPU->mSFR[address - 0x80] = (aCPU->mSFR[address - 0x80] & ~bitmask) | (carry << bit);
        SFR::write(aCPU, address);
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

static ALWAYS_INLINE FASTCALL int movc_a_indir_a_dptr(em8051 *aCPU, unsigned &PC, uint8_t opcode, uint8_t operand1, uint8_t operand2)
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

static ALWAYS_INLINE FASTCALL int subb_a_imm(em8051 *aCPU, unsigned &PC, uint8_t opcode, uint8_t operand1, uint8_t operand2)
{
    int carry = CARRY;
    sub_solve_flags(aCPU, ACC, operand1 + carry);
    ACC -= operand1 + carry;
    PC += 2;
    return 2;
}

static ALWAYS_INLINE FASTCALL int subb_a_mem(em8051 *aCPU, unsigned &PC, uint8_t opcode, uint8_t operand1, uint8_t operand2) 
{
    int carry = CARRY;
    int value = read_mem(aCPU, operand1) + carry;
    sub_solve_flags(aCPU, ACC, value);
    ACC -= value;

    PC += 2;
    return 2;
}
static ALWAYS_INLINE FASTCALL int subb_a_indir_rx(em8051 *aCPU, unsigned &PC, uint8_t opcode, uint8_t operand1, uint8_t operand2)
{
    int carry = CARRY;
    int address = INDIR_RX_ADDRESS;
    sub_solve_flags(aCPU, ACC, aCPU->mData[address] + carry);
    ACC -= aCPU->mData[address] + carry;
    PC++;
    return 2;
}


static ALWAYS_INLINE FASTCALL int orl_c_compl_bitaddr(em8051 *aCPU, unsigned &PC, uint8_t opcode, uint8_t operand1, uint8_t operand2)
{
    int address = operand1;
    int carry = CARRY;
    if (address > 0x7f)
    {
        int bit = address & 7;
        int bitmask = (1 << bit);
        int value;
        address &= 0xf8;        
        value = SFR::read(aCPU, address);
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

static ALWAYS_INLINE FASTCALL int mov_c_bitaddr(em8051 *aCPU, unsigned &PC, uint8_t opcode, uint8_t operand1, uint8_t operand2) 
{
    int address = operand1;
    if (address > 0x7f)
    {
        int bit = address & 7;
        int bitmask = (1 << bit);
        int value;
        address &= 0xf8;        
        value = SFR::read(aCPU, address);
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

static ALWAYS_INLINE FASTCALL int inc_dptr(em8051 *aCPU, unsigned &PC, uint8_t opcode, uint8_t operand1, uint8_t operand2)
{
    aCPU->mSFR[CUR_DPL]++;
    if (!aCPU->mSFR[CUR_DPL])
        aCPU->mSFR[CUR_DPH]++;
    PC++;
    return 1;
}

static ALWAYS_INLINE FASTCALL int mul_ab(em8051 *aCPU, unsigned &PC, uint8_t opcode, uint8_t operand1, uint8_t operand2)
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

static ALWAYS_INLINE FASTCALL int mov_indir_rx_mem(em8051 *aCPU, unsigned &PC, uint8_t opcode, uint8_t operand1, uint8_t operand2)
{
    int address1 = INDIR_RX_ADDRESS;
    int value = read_mem(aCPU, operand1);
    aCPU->mData[address1] = value;
    PC += 2;
    return 5;
}


static ALWAYS_INLINE FASTCALL int anl_c_compl_bitaddr(em8051 *aCPU, unsigned &PC, uint8_t opcode, uint8_t operand1, uint8_t operand2)
{
    int address = operand1;
    int carry = CARRY;
    if (address > 0x7f)
    {
        int bit = address & 7;
        int bitmask = (1 << bit);
        int value;
        address &= 0xf8;        
        value = SFR::read(aCPU, address);
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


static ALWAYS_INLINE FASTCALL int cpl_bitaddr(em8051 *aCPU, unsigned &PC, uint8_t opcode, uint8_t operand1, uint8_t operand2)
{
    int address = operand1;
    if (address > 0x7f)
    {
        // Data sheet does not explicitly say that the modification source
        // is read from output latch, but we'll assume that is what happens.
        int bit = address & 7;
        int bitmask = (1 << bit);
        address &= 0xf8;        
        aCPU->mSFR[address - 0x80] ^= bitmask;
        SFR::write(aCPU, address);
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

static ALWAYS_INLINE FASTCALL int cpl_c(em8051 *aCPU, unsigned &PC, uint8_t opcode, uint8_t operand1, uint8_t operand2)
{
    PSW ^= PSWMASK_C;
    PC++;
    return 1;
}

static ALWAYS_INLINE FASTCALL int cjne_a_imm_offset(em8051 *aCPU, unsigned &PC, uint8_t opcode, uint8_t operand1, uint8_t operand2)
{
    int value = operand1;

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
        PC += (signed char)operand2 + 3;
    }
    else
    {
        PC += 3;
    }
    return 4;
}

static ALWAYS_INLINE FASTCALL int cjne_a_mem_offset(em8051 *aCPU, unsigned &PC, uint8_t opcode, uint8_t operand1, uint8_t operand2)
{
    int address = operand1;
    int value;
    if (address > 0x7f)
    {
        value = SFR::read(aCPU, address);
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
        PC += (signed char)operand2 + 3;
    }
    else
    {
        PC += 3;
    }
    return 4;
}
static ALWAYS_INLINE FASTCALL int cjne_indir_rx_imm_offset(em8051 *aCPU, unsigned &PC, uint8_t opcode, uint8_t operand1, uint8_t operand2)
{
    int address = INDIR_RX_ADDRESS;
    int value1 = aCPU->mData[address];
    int value2 = operand1;

    if (value1 < value2)
        PSW |= PSWMASK_C;
    else
        PSW &= ~PSWMASK_C;

    if (value1 != value2)
        PC += (signed char)operand2 + 3;
    else
        PC += 3;

    return 4;
}

static ALWAYS_INLINE FASTCALL int push_mem(em8051 *aCPU, unsigned &PC, uint8_t opcode, uint8_t operand1, uint8_t operand2)
{
    int value = read_mem(aCPU, operand1);
    push_to_stack(aCPU, value);   
    PC += 2;
    return 4;
}


static ALWAYS_INLINE FASTCALL int clr_bitaddr(em8051 *aCPU, unsigned &PC, uint8_t opcode, uint8_t operand1, uint8_t operand2)
{
    int address = operand1;
    if (address > 0x7f)
    {
        // Data sheet does not explicitly say that the modification source
        // is read from output latch, but we'll assume that is what happens.
        int bit = address & 7;
        int bitmask = (1 << bit);
        address &= 0xf8;        
        aCPU->mSFR[address - 0x80] &= ~bitmask;
        SFR::write(aCPU, address);
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

static ALWAYS_INLINE FASTCALL int clr_c(em8051 *aCPU, unsigned &PC, uint8_t opcode, uint8_t operand1, uint8_t operand2)
{
    PSW &= ~PSWMASK_C;
    PC++;
    return 1;
}

static ALWAYS_INLINE FASTCALL int swap_a(em8051 *aCPU, unsigned &PC, uint8_t opcode, uint8_t operand1, uint8_t operand2)
{
    ACC = (ACC << 4) | (ACC >> 4);
    PC++;
    return 1;
}

static ALWAYS_INLINE FASTCALL int xch_a_mem(em8051 *aCPU, unsigned &PC, uint8_t opcode, uint8_t operand1, uint8_t operand2)
{
    int address = operand1;
    int value = read_mem(aCPU, operand1);
    if (address > 0x7f)
    {
        aCPU->mSFR[address - 0x80] = ACC;
        ACC = value;
        SFR::write(aCPU, address);
    }
    else
    {
        aCPU->mData[address] = ACC;
        ACC = value;
    }
    PC += 2;
    return 3;
}

static ALWAYS_INLINE FASTCALL int xch_a_indir_rx(em8051 *aCPU, unsigned &PC, uint8_t opcode, uint8_t operand1, uint8_t operand2)
{
    int address = INDIR_RX_ADDRESS;
    int value = aCPU->mData[address];
    aCPU->mData[address] = ACC;
    ACC = value;
    PC++;
    return 3;
}


static ALWAYS_INLINE FASTCALL int pop_mem(em8051 *aCPU, unsigned &PC, uint8_t opcode, uint8_t operand1, uint8_t operand2)
{
    int address = operand1;
    if (address > 0x7f)
    {
        aCPU->mSFR[address - 0x80] = pop_from_stack(aCPU);
        SFR::write(aCPU, address);
    }
    else
    {
        aCPU->mData[address] = pop_from_stack(aCPU);
    }

    PC += 2;
    return 3;
}

static ALWAYS_INLINE FASTCALL int setb_bitaddr(em8051 *aCPU, unsigned &PC, uint8_t opcode, uint8_t operand1, uint8_t operand2)
{
    int address = operand1;
    if (address > 0x7f)
    {
        // Data sheet does not explicitly say that the modification source
        // is read from output latch, but we'll assume that is what happens.
        int bit = address & 7;
        int bitmask = (1 << bit);
        address &= 0xf8;        
        aCPU->mSFR[address - 0x80] |= bitmask;
        SFR::write(aCPU, address);
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

static ALWAYS_INLINE FASTCALL int setb_c(em8051 *aCPU, unsigned &PC, uint8_t opcode, uint8_t operand1, uint8_t operand2)
{
    PSW |= PSWMASK_C;
    PC++;
    return 1;
}

static ALWAYS_INLINE FASTCALL int da_a(em8051 *aCPU, unsigned &PC, uint8_t opcode, uint8_t operand1, uint8_t operand2)
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

static ALWAYS_INLINE FASTCALL int djnz_mem_offset(em8051 *aCPU, unsigned &PC, uint8_t opcode, uint8_t operand1, uint8_t operand2)
{
    int address = operand1;
    int value;
    if (address > 0x7f)
    {
        aCPU->mSFR[address - 0x80]--;
        value = aCPU->mSFR[address - 0x80];
        SFR::write(aCPU, address);
    }
    else
    {
        aCPU->mData[address]--;
        value = aCPU->mData[address];
    }
    if (value)
    {
        PC += (signed char)operand2 + 3;
    }
    else
    {
        PC += 3;
    }
    return 4;
}

static ALWAYS_INLINE FASTCALL int xchd_a_indir_rx(em8051 *aCPU, unsigned &PC, uint8_t opcode, uint8_t operand1, uint8_t operand2)
{
    int address = INDIR_RX_ADDRESS;
    int value = aCPU->mData[address];
    aCPU->mData[address] = (aCPU->mData[address] & 0x0f) | (ACC & 0x0f);
    ACC = (ACC & 0xf0) | (value & 0x0f);
    PC++;
    return 3;
}


static ALWAYS_INLINE FASTCALL int movx_a_indir_dptr(em8051 *aCPU, unsigned &PC, uint8_t opcode, uint8_t operand1, uint8_t operand2)
{
    int dptr = (aCPU->mSFR[CUR_DPH] << 8) | aCPU->mSFR[CUR_DPL];
    ACC = xdata_read(aCPU, dptr);
    PC++;
    return 4;
}

static ALWAYS_INLINE FASTCALL int movx_a_indir_rx(em8051 *aCPU, unsigned &PC, uint8_t opcode, uint8_t operand1, uint8_t operand2)
{
    int dptr = INDIR_RX_ADDRESS_X;
    ACC = xdata_read(aCPU, dptr);
    PC++;
    return 4;
}

static ALWAYS_INLINE FASTCALL int clr_a(em8051 *aCPU, unsigned &PC, uint8_t opcode, uint8_t operand1, uint8_t operand2)
{
    ACC = 0;
    PC++;
    return 1;
}

static ALWAYS_INLINE FASTCALL int mov_a_mem(em8051 *aCPU, unsigned &PC, uint8_t opcode, uint8_t operand1, uint8_t operand2)
{
    // mov a,acc is not a valid instruction
    int address = operand1;
    int value = read_mem(aCPU, address);
    if (UNLIKELY(REG_ACC == address - 0x80))
        except(aCPU, EXCEPTION_ACC_TO_A);
    ACC = value;

    PC += 2;
    return 2;
}

static ALWAYS_INLINE FASTCALL int mov_a_indir_rx(em8051 *aCPU, unsigned &PC, uint8_t opcode, uint8_t operand1, uint8_t operand2)
{
    int address = INDIR_RX_ADDRESS;
    ACC = aCPU->mData[address];
    PC++;
    return 2;
}


static ALWAYS_INLINE FASTCALL int movx_indir_dptr_a(em8051 *aCPU, unsigned &PC, uint8_t opcode, uint8_t operand1, uint8_t operand2)
{
    int dptr = (aCPU->mSFR[CUR_DPH] << 8) | aCPU->mSFR[CUR_DPL];
    PC++;
    return 5 + xdata_write(aCPU, dptr, ACC);
}

static ALWAYS_INLINE FASTCALL int movx_indir_rx_a(em8051 *aCPU, unsigned &PC, uint8_t opcode, uint8_t operand1, uint8_t operand2)
{
    int dptr = INDIR_RX_ADDRESS_X;
    PC++;
    return 5 + xdata_write(aCPU, dptr, ACC);
}

static ALWAYS_INLINE FASTCALL int cpl_a(em8051 *aCPU, unsigned &PC, uint8_t opcode, uint8_t operand1, uint8_t operand2)
{
    ACC = ~ACC;
    PC++;
    return 1;
}

static ALWAYS_INLINE FASTCALL int mov_mem_a(em8051 *aCPU, unsigned &PC, uint8_t opcode, uint8_t operand1, uint8_t operand2)
{
    int address = operand1;
    if (address > 0x7f)
    {
        aCPU->mSFR[address - 0x80] = ACC;
        SFR::write(aCPU, address);
    }
    else
    {
        aCPU->mData[address] = ACC;
    }
    PC += 2;
    return 3;
}

static ALWAYS_INLINE FASTCALL int mov_indir_rx_a(em8051 *aCPU, unsigned &PC, uint8_t opcode, uint8_t operand1, uint8_t operand2)
{
    int address = INDIR_RX_ADDRESS;
    aCPU->mData[address] = ACC;
    PC++;
    return 3;
}

static ALWAYS_INLINE FASTCALL int nop(em8051 *aCPU, unsigned &PC, uint8_t opcode, uint8_t operand1, uint8_t operand2)
{
    PC++;
    return 1;
}

static ALWAYS_INLINE FASTCALL int illegal(em8051 *aCPU, unsigned &PC, uint8_t opcode, uint8_t operand1, uint8_t operand2)
{
    except(aCPU, EXCEPTION_ILLEGAL_OPCODE);
    PC++;
    return 1;
}

static ALWAYS_INLINE FASTCALL int inc_rx(em8051 *aCPU, unsigned &PC, uint8_t opcode, uint8_t operand1, uint8_t operand2)
{
    int rx = RX_ADDRESS;
    aCPU->mData[rx]++;
    PC++;
    return 2;
}

static ALWAYS_INLINE FASTCALL int dec_rx(em8051 *aCPU, unsigned &PC, uint8_t opcode, uint8_t operand1, uint8_t operand2)
{
    int rx = RX_ADDRESS;
    aCPU->mData[rx]--;
    PC++;
    return 2;
}

static ALWAYS_INLINE FASTCALL int add_a_rx(em8051 *aCPU, unsigned &PC, uint8_t opcode, uint8_t operand1, uint8_t operand2)
{
    int rx = RX_ADDRESS;
    add_solve_flags(aCPU, aCPU->mData[rx], ACC, 0);
    ACC += aCPU->mData[rx];
    PC++;
    return 1;
}

static ALWAYS_INLINE FASTCALL int addc_a_rx(em8051 *aCPU, unsigned &PC, uint8_t opcode, uint8_t operand1, uint8_t operand2)
{
    int rx = RX_ADDRESS;
    int carry = CARRY;
    add_solve_flags(aCPU, aCPU->mData[rx], ACC, carry);
    ACC += aCPU->mData[rx] + carry;
    PC++;
    return 1;
}

static ALWAYS_INLINE FASTCALL int orl_a_rx(em8051 *aCPU, unsigned &PC, uint8_t opcode, uint8_t operand1, uint8_t operand2)
{
    int rx = RX_ADDRESS;
    ACC |= aCPU->mData[rx];
    PC++;
    return 1;
}

static ALWAYS_INLINE FASTCALL int anl_a_rx(em8051 *aCPU, unsigned &PC, uint8_t opcode, uint8_t operand1, uint8_t operand2)
{
    int rx = RX_ADDRESS;
    ACC &= aCPU->mData[rx];
    PC++;
    return 1;
}

static ALWAYS_INLINE FASTCALL int xrl_a_rx(em8051 *aCPU, unsigned &PC, uint8_t opcode, uint8_t operand1, uint8_t operand2)
{
    int rx = RX_ADDRESS;
    ACC ^= aCPU->mData[rx];    
    PC++;
    return 1;
}


static ALWAYS_INLINE FASTCALL int mov_rx_imm(em8051 *aCPU, unsigned &PC, uint8_t opcode, uint8_t operand1, uint8_t operand2)
{
    int rx = RX_ADDRESS;
    aCPU->mData[rx] = operand1;
    PC += 2;
    return 2;
}

static ALWAYS_INLINE FASTCALL int mov_mem_rx(em8051 *aCPU, unsigned &PC, uint8_t opcode, uint8_t operand1, uint8_t operand2)
{
    int rx = RX_ADDRESS;
    int address = operand1;
    if (address > 0x7f)
    {
        aCPU->mSFR[address - 0x80] = aCPU->mData[rx];
        SFR::write(aCPU, address);
    }
    else
    {
        aCPU->mData[address] = aCPU->mData[rx];
    }
    PC += 2;
    return 3;
}

static ALWAYS_INLINE FASTCALL int subb_a_rx(em8051 *aCPU, unsigned &PC, uint8_t opcode, uint8_t operand1, uint8_t operand2)
{
    int rx = RX_ADDRESS;
    int carry = CARRY;
    sub_solve_flags(aCPU, ACC, aCPU->mData[rx] + carry);
    ACC -= aCPU->mData[rx] + carry;
    PC++;
    return 1;
}

static ALWAYS_INLINE FASTCALL int mov_rx_mem(em8051 *aCPU, unsigned &PC, uint8_t opcode, uint8_t operand1, uint8_t operand2)
{
    int rx = RX_ADDRESS;
    int value = read_mem(aCPU, operand1);
    aCPU->mData[rx] = value;

    PC += 2;
    return 4;
}

static ALWAYS_INLINE FASTCALL int cjne_rx_imm_offset(em8051 *aCPU, unsigned &PC, uint8_t opcode, uint8_t operand1, uint8_t operand2)
{
    int rx = RX_ADDRESS;
    int value = operand1;
    
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
        PC += (signed char)operand2 + 3;
    }
    else
    {
        PC += 3;
    }
    return 4;
}

static ALWAYS_INLINE FASTCALL int xch_a_rx(em8051 *aCPU, unsigned &PC, uint8_t opcode, uint8_t operand1, uint8_t operand2)
{
    int rx = RX_ADDRESS;
    int a = ACC;
    ACC = aCPU->mData[rx];
    aCPU->mData[rx] = a;
    PC++;
    return 2;
}

static ALWAYS_INLINE FASTCALL int djnz_rx_offset(em8051 *aCPU, unsigned &PC, uint8_t opcode, uint8_t operand1, uint8_t operand2)
{
    int rx = RX_ADDRESS;
    aCPU->mData[rx]--;
    if (aCPU->mData[rx])
    {
        PC += (signed char)operand1 + 2;
    }
    else
    {
        PC += 2;
    }
    return 3;
}

static ALWAYS_INLINE FASTCALL int mov_a_rx(em8051 *aCPU, unsigned &PC, uint8_t opcode, uint8_t operand1, uint8_t operand2)
{
    int rx = RX_ADDRESS;
    ACC = aCPU->mData[rx];

    PC++;
    return 1;
}

static ALWAYS_INLINE FASTCALL int mov_rx_a(em8051 *aCPU, unsigned &PC, uint8_t opcode, uint8_t operand1, uint8_t operand2)
{
    int rx = RX_ADDRESS;
    aCPU->mData[rx] = ACC;
    PC++;
    return 2;
}

};  // struct Opcodes
};  // namespace CPU
};  // namespace Cube

#endif
