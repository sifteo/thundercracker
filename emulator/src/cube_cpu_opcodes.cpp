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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cube_cpu_opcodes.h"

namespace Cube {
namespace CPU {

void op_setptrs(em8051 *aCPU)
{
    int i;
    for (i = 0; i < 8; i++)
    {
        aCPU->op[0x08 + i] = &Opcodes::inc_rx;
        aCPU->op[0x18 + i] = &Opcodes::dec_rx;
        aCPU->op[0x28 + i] = &Opcodes::add_a_rx;
        aCPU->op[0x38 + i] = &Opcodes::addc_a_rx;
        aCPU->op[0x48 + i] = &Opcodes::orl_a_rx;
        aCPU->op[0x58 + i] = &Opcodes::anl_a_rx;
        aCPU->op[0x68 + i] = &Opcodes::xrl_a_rx;
        aCPU->op[0x78 + i] = &Opcodes::mov_rx_imm;
        aCPU->op[0x88 + i] = &Opcodes::mov_mem_rx;
        aCPU->op[0x98 + i] = &Opcodes::subb_a_rx;
        aCPU->op[0xa8 + i] = &Opcodes::mov_rx_mem;
        aCPU->op[0xb8 + i] = &Opcodes::cjne_rx_imm_offset;
        aCPU->op[0xc8 + i] = &Opcodes::xch_a_rx;
        aCPU->op[0xd8 + i] = &Opcodes::djnz_rx_offset;
        aCPU->op[0xe8 + i] = &Opcodes::mov_a_rx;
        aCPU->op[0xf8 + i] = &Opcodes::mov_rx_a;
    }
    aCPU->op[0x00] = &Opcodes::nop;
    aCPU->op[0x01] = &Opcodes::ajmp_offset;
    aCPU->op[0x02] = &Opcodes::ljmp_address;
    aCPU->op[0x03] = &Opcodes::rr_a;
    aCPU->op[0x04] = &Opcodes::inc_a;
    aCPU->op[0x05] = &Opcodes::inc_mem;
    aCPU->op[0x06] = &Opcodes::inc_indir_rx;
    aCPU->op[0x07] = &Opcodes::inc_indir_rx;

    aCPU->op[0x10] = &Opcodes::jbc_bitaddr_offset;
    aCPU->op[0x11] = &Opcodes::acall_offset;
    aCPU->op[0x12] = &Opcodes::lcall_address;
    aCPU->op[0x13] = &Opcodes::rrc_a;
    aCPU->op[0x14] = &Opcodes::dec_a;
    aCPU->op[0x15] = &Opcodes::dec_mem;
    aCPU->op[0x16] = &Opcodes::dec_indir_rx;
    aCPU->op[0x17] = &Opcodes::dec_indir_rx;

    aCPU->op[0x20] = &Opcodes::jb_bitaddr_offset;
    aCPU->op[0x21] = &Opcodes::ajmp_offset;
    aCPU->op[0x22] = &Opcodes::ret;
    aCPU->op[0x23] = &Opcodes::rl_a;
    aCPU->op[0x24] = &Opcodes::add_a_imm;
    aCPU->op[0x25] = &Opcodes::add_a_mem;
    aCPU->op[0x26] = &Opcodes::add_a_indir_rx;
    aCPU->op[0x27] = &Opcodes::add_a_indir_rx;

    aCPU->op[0x30] = &Opcodes::jnb_bitaddr_offset;
    aCPU->op[0x31] = &Opcodes::acall_offset;
    aCPU->op[0x32] = &Opcodes::reti;
    aCPU->op[0x33] = &Opcodes::rlc_a;
    aCPU->op[0x34] = &Opcodes::addc_a_imm;
    aCPU->op[0x35] = &Opcodes::addc_a_mem;
    aCPU->op[0x36] = &Opcodes::addc_a_indir_rx;
    aCPU->op[0x37] = &Opcodes::addc_a_indir_rx;

    aCPU->op[0x40] = &Opcodes::jc_offset;
    aCPU->op[0x41] = &Opcodes::ajmp_offset;
    aCPU->op[0x42] = &Opcodes::orl_mem_a;
    aCPU->op[0x43] = &Opcodes::orl_mem_imm;
    aCPU->op[0x44] = &Opcodes::orl_a_imm;
    aCPU->op[0x45] = &Opcodes::orl_a_mem;
    aCPU->op[0x46] = &Opcodes::orl_a_indir_rx;
    aCPU->op[0x47] = &Opcodes::orl_a_indir_rx;

    aCPU->op[0x50] = &Opcodes::jnc_offset;
    aCPU->op[0x51] = &Opcodes::acall_offset;
    aCPU->op[0x52] = &Opcodes::anl_mem_a;
    aCPU->op[0x53] = &Opcodes::anl_mem_imm;
    aCPU->op[0x54] = &Opcodes::anl_a_imm;
    aCPU->op[0x55] = &Opcodes::anl_a_mem;
    aCPU->op[0x56] = &Opcodes::anl_a_indir_rx;
    aCPU->op[0x57] = &Opcodes::anl_a_indir_rx;

    aCPU->op[0x60] = &Opcodes::jz_offset;
    aCPU->op[0x61] = &Opcodes::ajmp_offset;
    aCPU->op[0x62] = &Opcodes::xrl_mem_a;
    aCPU->op[0x63] = &Opcodes::xrl_mem_imm;
    aCPU->op[0x64] = &Opcodes::xrl_a_imm;
    aCPU->op[0x65] = &Opcodes::xrl_a_mem;
    aCPU->op[0x66] = &Opcodes::xrl_a_indir_rx;
    aCPU->op[0x67] = &Opcodes::xrl_a_indir_rx;

    aCPU->op[0x70] = &Opcodes::jnz_offset;
    aCPU->op[0x71] = &Opcodes::acall_offset;
    aCPU->op[0x72] = &Opcodes::orl_c_bitaddr;
    aCPU->op[0x73] = &Opcodes::jmp_indir_a_dptr;
    aCPU->op[0x74] = &Opcodes::mov_a_imm;
    aCPU->op[0x75] = &Opcodes::mov_mem_imm;
    aCPU->op[0x76] = &Opcodes::mov_indir_rx_imm;
    aCPU->op[0x77] = &Opcodes::mov_indir_rx_imm;

    aCPU->op[0x80] = &Opcodes::sjmp_offset;
    aCPU->op[0x81] = &Opcodes::ajmp_offset;
    aCPU->op[0x82] = &Opcodes::anl_c_bitaddr;
    aCPU->op[0x83] = &Opcodes::movc_a_indir_a_pc;
    aCPU->op[0x84] = &Opcodes::div_ab;
    aCPU->op[0x85] = &Opcodes::mov_mem_mem;
    aCPU->op[0x86] = &Opcodes::mov_mem_indir_rx;
    aCPU->op[0x87] = &Opcodes::mov_mem_indir_rx;

    aCPU->op[0x90] = &Opcodes::mov_dptr_imm;
    aCPU->op[0x91] = &Opcodes::acall_offset;
    aCPU->op[0x92] = &Opcodes::mov_bitaddr_c;
    aCPU->op[0x93] = &Opcodes::movc_a_indir_a_dptr;
    aCPU->op[0x94] = &Opcodes::subb_a_imm;
    aCPU->op[0x95] = &Opcodes::subb_a_mem;
    aCPU->op[0x96] = &Opcodes::subb_a_indir_rx;
    aCPU->op[0x97] = &Opcodes::subb_a_indir_rx;

    aCPU->op[0xa0] = &Opcodes::orl_c_compl_bitaddr;
    aCPU->op[0xa1] = &Opcodes::ajmp_offset;
    aCPU->op[0xa2] = &Opcodes::mov_c_bitaddr;
    aCPU->op[0xa3] = &Opcodes::inc_dptr;
    aCPU->op[0xa4] = &Opcodes::mul_ab;
    aCPU->op[0xa5] = &Opcodes::illegal;
    aCPU->op[0xa6] = &Opcodes::mov_indir_rx_mem;
    aCPU->op[0xa7] = &Opcodes::mov_indir_rx_mem;

    aCPU->op[0xb0] = &Opcodes::anl_c_compl_bitaddr;
    aCPU->op[0xb1] = &Opcodes::acall_offset;
    aCPU->op[0xb2] = &Opcodes::cpl_bitaddr;
    aCPU->op[0xb3] = &Opcodes::cpl_c;
    aCPU->op[0xb4] = &Opcodes::cjne_a_imm_offset;
    aCPU->op[0xb5] = &Opcodes::cjne_a_mem_offset;
    aCPU->op[0xb6] = &Opcodes::cjne_indir_rx_imm_offset;
    aCPU->op[0xb7] = &Opcodes::cjne_indir_rx_imm_offset;

    aCPU->op[0xc0] = &Opcodes::push_mem;
    aCPU->op[0xc1] = &Opcodes::ajmp_offset;
    aCPU->op[0xc2] = &Opcodes::clr_bitaddr;
    aCPU->op[0xc3] = &Opcodes::clr_c;
    aCPU->op[0xc4] = &Opcodes::swap_a;
    aCPU->op[0xc5] = &Opcodes::xch_a_mem;
    aCPU->op[0xc6] = &Opcodes::xch_a_indir_rx;
    aCPU->op[0xc7] = &Opcodes::xch_a_indir_rx;

    aCPU->op[0xd0] = &Opcodes::pop_mem;
    aCPU->op[0xd1] = &Opcodes::acall_offset;
    aCPU->op[0xd2] = &Opcodes::setb_bitaddr;
    aCPU->op[0xd3] = &Opcodes::setb_c;
    aCPU->op[0xd4] = &Opcodes::da_a;
    aCPU->op[0xd5] = &Opcodes::djnz_mem_offset;
    aCPU->op[0xd6] = &Opcodes::xchd_a_indir_rx;
    aCPU->op[0xd7] = &Opcodes::xchd_a_indir_rx;

    aCPU->op[0xe0] = &Opcodes::movx_a_indir_dptr;
    aCPU->op[0xe1] = &Opcodes::ajmp_offset;
    aCPU->op[0xe2] = &Opcodes::movx_a_indir_rx;
    aCPU->op[0xe3] = &Opcodes::movx_a_indir_rx;
    aCPU->op[0xe4] = &Opcodes::clr_a;
    aCPU->op[0xe5] = &Opcodes::mov_a_mem;
    aCPU->op[0xe6] = &Opcodes::mov_a_indir_rx;
    aCPU->op[0xe7] = &Opcodes::mov_a_indir_rx;

    aCPU->op[0xf0] = &Opcodes::movx_indir_dptr_a;
    aCPU->op[0xf1] = &Opcodes::acall_offset;
    aCPU->op[0xf2] = &Opcodes::movx_indir_rx_a;
    aCPU->op[0xf3] = &Opcodes::movx_indir_rx_a;
    aCPU->op[0xf4] = &Opcodes::cpl_a;
    aCPU->op[0xf5] = &Opcodes::mov_mem_a;
    aCPU->op[0xf6] = &Opcodes::mov_indir_rx_a;
    aCPU->op[0xf7] = &Opcodes::mov_indir_rx_a;
}


};  // namespace CPU
};  // namespace Cube

