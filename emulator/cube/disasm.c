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
 * disasm.c
 * Disassembler functions
 *
 * These functions decode 8051 operations into text strings, useful in
 * interactive debugger.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "emu8051.h"

//#define static

static const char *sfr_names[128] = {
    "P0",        "SP",        "DPL",       "DPH",     
    "DPL1",      "DPH1",      "DEBUG",     "SFR_87",   
    "TCON",      "TMOD",      "TL0",       "TL1",      
    "TH0",       "TH1",       "SFR_8E",    "P3CON",    
    "P1",        "SFR_91",    "DPS",       "P0DIR",    
    "P1DIR",     "P2DIR",     "P3DIR",     "P2CON",    
    "S0CON",     "S0BUF",     "SFR_9A",    "SFR_9B",   
    "SFR_9C",    "SFR_9D",    "P0CON",     "P1CON",    
    "P2",        "PWMDC0",    "PWMDC1",    "CLKCTRL",  
    "PWRDWN",    "WUCON",     "INTEXP",    "MEMCON",   
    "IEN0",      "IP0",       "S0RELL",    "RTC2CPT01",
    "RTC2CPT10", "CLKLFCTRL", "OPMCON",    "WDSV",     
    "P3",        "RSTREAS",   "PWMCON",    "RTC2CON",  
    "RTC2CMP0",  "RTC2CMP1",  "RTC2CPT00", "SFR_B7",   
    "IEN1",      "IP1",       "S0RELH",    "SFR_BB",   
    "SPISCON0",  "SFR_BD",    "SPISSTAT",  "SPISDAT",  
    "IRCON",     "CCEN",      "CCL1",      "CCH1",     
    "CCL2",      "CCH2",      "CCL3",      "CCH3",     
    "T2CON",     "MPAGE",     "CRCL",      "CRCH",     
    "TL2",       "TH2",       "WUOPC1",    "WUOPC0",   
    "PSW",       "ADCCON3",   "ADCCON2",   "ADCCON1",  
    "ADCDATH",   "ADCDATL",   "RNGCTL",    "RNGDAT",   
    "ADCON",     "W2SADR",    "W2DAT",     "COMPCON",  
    "POFCON",    "CCPDATIA",  "CCPDATIB",  "CCPDATO",  
    "ACC",       "W2CON1",    "W2CON0",    "SFR_E3",   
    "SPIRCON0",  "SPIRCON1",  "SPIRSTAT",  "SPIRDAT",  
    "RFCON",     "MD0",       "MD1",       "MD2",      
    "MD3",       "MD4",       "MD5",       "ARCON",    
    "B",         "SFR_F1",    "SFR_F2",    "SFR_F3",   
    "SFR_F4",    "SFR_F5",    "SFR_F6",    "SFR_F7",   
    "FSR",       "FPCR",      "FCR",       "SFR_FB",   
    "SPIMCON0",  "SPIMCON1",  "SPIMSTAT",  "SPIMDAT",  
};

void mem_mnemonic(int aValue, char *aBuffer)
{
    if (aValue > 0x7f)
        strcpy(aBuffer, sfr_names[aValue - 0x80]);
    else
        sprintf(aBuffer, "%02Xh", aValue);
}

void bitaddr_mnemonic(int aValue, char *aBuffer)
{
    char regname[32];

    if (aValue > 0x7f)
        mem_mnemonic(aValue & 0xf8, regname);
    else
        sprintf(regname, "%02Xh", aValue >> 3);

    sprintf(aBuffer, "%s.%d", regname, aValue & 7);
}


static int disasm_ajmp_offset(struct em8051 *aCPU, int aPosition, char *aBuffer)
{
    sprintf(aBuffer,"AJMP  #%04Xh",
            ((aPosition + 2) & 0xf800) |
            aCPU->mCodeMem[(aPosition + 1)&(aCPU->mCodeMemSize-1)] | 
            ((aCPU->mCodeMem[(aPosition)&(aCPU->mCodeMemSize-1)] & 0xe0) << 3));
    return 2;
}

static int disasm_ljmp_address(struct em8051 *aCPU, int aPosition, char *aBuffer)
{
    sprintf(aBuffer,"LJMP  #%04Xh",        
        (aCPU->mCodeMem[(aPosition + 1)&(aCPU->mCodeMemSize-1)] << 8) | 
        aCPU->mCodeMem[(aPosition + 2)&(aCPU->mCodeMemSize-1)]);
    return 3;
}

static int disasm_rr_a(struct em8051 *aCPU, int aPosition, char *aBuffer)
{
    sprintf(aBuffer,"RR    A");
    return 1;
}

static int disasm_inc_a(struct em8051 *aCPU, int aPosition, char *aBuffer)
{
    sprintf(aBuffer,"INC   A");
    return 1;
}

static int disasm_inc_mem(struct em8051 *aCPU, int aPosition, char *aBuffer)
{
    char mem[32];mem_mnemonic(aCPU->mCodeMem[(aPosition + 1)&(aCPU->mCodeMemSize-1)], mem);
    sprintf(aBuffer,"INC   %s",        
        mem);
    
    return 2;
}

static int disasm_inc_indir_rx(struct em8051 *aCPU, int aPosition, char *aBuffer)
{
    sprintf(aBuffer,"INC   @R%d",        
        aCPU->mCodeMem[(aPosition)&(aCPU->mCodeMemSize-1)] & 1);
    return 1;
}

static int disasm_jbc_bitaddr_offset(struct em8051 *aCPU, int aPosition, char *aBuffer)
{
    char baddr[32];
    bitaddr_mnemonic(aCPU->mCodeMem[(aPosition + 1)&(aCPU->mCodeMemSize-1)], baddr);
    sprintf(aBuffer,"JBC   %s, #%+d",        
        baddr,
        (signed char)aCPU->mCodeMem[(aPosition + 2)&(aCPU->mCodeMemSize-1)]);
    return 3;
}

static int disasm_acall_offset(struct em8051 *aCPU, int aPosition, char *aBuffer)
{
    sprintf(aBuffer,"ACALL %04Xh",
            ((aPosition + 2) & 0xf800) | 
            aCPU->mCodeMem[(aPosition + 1)&(aCPU->mCodeMemSize-1)] | 
            ((aCPU->mCodeMem[(aPosition)&(aCPU->mCodeMemSize-1)] & 0xe0) << 3));
    return 2;
}

static int disasm_lcall_address(struct em8051 *aCPU, int aPosition, char *aBuffer)
{
    sprintf(aBuffer,"LCALL #%04Xh",        
            (aCPU->mCodeMem[(aPosition + 1)&(aCPU->mCodeMemSize-1)] << 8) | 
            aCPU->mCodeMem[(aPosition + 2)&(aCPU->mCodeMemSize-1)]);
    return 3;
}

static int disasm_rrc_a(struct em8051 *aCPU, int aPosition, char *aBuffer)
{
    sprintf(aBuffer,"RRC   A");
    return 1;
}

static int disasm_dec_a(struct em8051 *aCPU, int aPosition, char *aBuffer)
{
    sprintf(aBuffer,"DEC   A");
    return 1;
}

static int disasm_dec_mem(struct em8051 *aCPU, int aPosition, char *aBuffer)
{
    char mem[32];mem_mnemonic(aCPU->mCodeMem[(aPosition + 1)&(aCPU->mCodeMemSize-1)], mem);
    sprintf(aBuffer,"DEC   %s",        
        mem);
    
    return 2;
}

static int disasm_dec_indir_rx(struct em8051 *aCPU, int aPosition, char *aBuffer)
{
    sprintf(aBuffer,"DEC   @R%d",        
        aCPU->mCodeMem[(aPosition)&(aCPU->mCodeMemSize-1)] & 1);
    return 1;
}


static int disasm_jb_bitaddr_offset(struct em8051 *aCPU, int aPosition, char *aBuffer)
{
    char baddr[32];
    bitaddr_mnemonic(aCPU->mCodeMem[(aPosition + 1)&(aCPU->mCodeMemSize-1)], baddr);
    sprintf(aBuffer,"JB   %s, #%+d",        
        baddr,
        (signed char)aCPU->mCodeMem[(aPosition + 2)&(aCPU->mCodeMemSize-1)]);
    return 3;
}

static int disasm_ret(struct em8051 *aCPU, int aPosition, char *aBuffer)
{
    sprintf(aBuffer,"RET");
    return 1;
}

static int disasm_rl_a(struct em8051 *aCPU, int aPosition, char *aBuffer)
{
    sprintf(aBuffer,"RL    A");
    return 1;
}

static int disasm_add_a_imm(struct em8051 *aCPU, int aPosition, char *aBuffer)
{
    sprintf(aBuffer,"ADD   A, #%02Xh",        
        aCPU->mCodeMem[(aPosition + 1)&(aCPU->mCodeMemSize-1)]);
    return 2;
}

static int disasm_add_a_mem(struct em8051 *aCPU, int aPosition, char *aBuffer)
{
    char mem[32];mem_mnemonic(aCPU->mCodeMem[(aPosition + 1)&(aCPU->mCodeMemSize-1)], mem);
    sprintf(aBuffer,"ADD   A, %s",        
        mem);
    
    return 2;
}

static int disasm_add_a_indir_rx(struct em8051 *aCPU, int aPosition, char *aBuffer)
{
    sprintf(aBuffer,"ADD   A, @R%d",        
        aCPU->mCodeMem[(aPosition)&(aCPU->mCodeMemSize-1)]&1);
    return 1;
}

static int disasm_jnb_bitaddr_offset(struct em8051 *aCPU, int aPosition, char *aBuffer)
{
    char baddr[32];
    bitaddr_mnemonic(aCPU->mCodeMem[(aPosition + 1)&(aCPU->mCodeMemSize-1)], baddr);
    sprintf(aBuffer,"JNB   %s, #%+d",        
        baddr,
        (signed char)aCPU->mCodeMem[(aPosition + 2)&(aCPU->mCodeMemSize-1)]);
    return 3;
}

static int disasm_reti(struct em8051 *aCPU, int aPosition, char *aBuffer)
{
    sprintf(aBuffer,"RETI");
    return 1;
}

static int disasm_rlc_a(struct em8051 *aCPU, int aPosition, char *aBuffer)
{
    sprintf(aBuffer,"RLC   A");
    return 1;
}

static int disasm_addc_a_imm(struct em8051 *aCPU, int aPosition, char *aBuffer)
{
    sprintf(aBuffer,"ADDC  A, #%02Xh",        
        aCPU->mCodeMem[(aPosition + 1)&(aCPU->mCodeMemSize-1)]);
    return 2;
}

static int disasm_addc_a_mem(struct em8051 *aCPU, int aPosition, char *aBuffer)
{
    char mem[32];mem_mnemonic(aCPU->mCodeMem[(aPosition + 1)&(aCPU->mCodeMemSize-1)], mem);
    sprintf(aBuffer,"ADDC  A, %s",        
        mem);
    
    return 2;
}

static int disasm_addc_a_indir_rx(struct em8051 *aCPU, int aPosition, char *aBuffer)
{
    sprintf(aBuffer,"ADDC  A, @R%d",        
        aCPU->mCodeMem[(aPosition)&(aCPU->mCodeMemSize-1)]&1);
    return 1;
}


static int disasm_jc_offset(struct em8051 *aCPU, int aPosition, char *aBuffer)
{
    sprintf(aBuffer,"JC    #%+d",        
        (signed char)aCPU->mCodeMem[(aPosition + 1)&(aCPU->mCodeMemSize-1)]);
    return 2;
}

static int disasm_orl_mem_a(struct em8051 *aCPU, int aPosition, char *aBuffer)
{
    char mem[32];mem_mnemonic(aCPU->mCodeMem[(aPosition + 1)&(aCPU->mCodeMemSize-1)], mem);
    sprintf(aBuffer,"ORL   %s, A",        
        mem);
    
    return 2;
}

static int disasm_orl_mem_imm(struct em8051 *aCPU, int aPosition, char *aBuffer)
{
    char mem[32];mem_mnemonic(aCPU->mCodeMem[(aPosition + 1)&(aCPU->mCodeMemSize-1)], mem);
    sprintf(aBuffer,"ORL   %s, #%02Xh",        
        mem,
        aCPU->mCodeMem[(aPosition + 2)&(aCPU->mCodeMemSize-1)]);
    
    return 3;
}

static int disasm_orl_a_imm(struct em8051 *aCPU, int aPosition, char *aBuffer)
{
    sprintf(aBuffer,"ORL   A, #%02Xh",        
        aCPU->mCodeMem[(aPosition + 1)&(aCPU->mCodeMemSize-1)]);
    return 2;
}

static int disasm_orl_a_mem(struct em8051 *aCPU, int aPosition, char *aBuffer)
{
    char mem[32];mem_mnemonic(aCPU->mCodeMem[(aPosition + 1)&(aCPU->mCodeMemSize-1)], mem);
    sprintf(aBuffer,"ORL   A, %s",        
        mem);
    
    return 2;
}

static int disasm_orl_a_indir_rx(struct em8051 *aCPU, int aPosition, char *aBuffer)
{
    sprintf(aBuffer,"ORL   A, @R%d",        
        aCPU->mCodeMem[(aPosition)&(aCPU->mCodeMemSize-1)]&1);
    return 1;
}


static int disasm_jnc_offset(struct em8051 *aCPU, int aPosition, char *aBuffer)
{
    sprintf(aBuffer,"JNC   #%+d",        
        (signed char)aCPU->mCodeMem[(aPosition + 1)&(aCPU->mCodeMemSize-1)]);
    return 2;
}

static int disasm_anl_mem_a(struct em8051 *aCPU, int aPosition, char *aBuffer)
{
    char mem[32];mem_mnemonic(aCPU->mCodeMem[(aPosition + 1)&(aCPU->mCodeMemSize-1)], mem);
    sprintf(aBuffer,"ANL   %s, A",
        mem);
    
    return 2;
}

static int disasm_anl_mem_imm(struct em8051 *aCPU, int aPosition, char *aBuffer)
{
    char mem[32];mem_mnemonic(aCPU->mCodeMem[(aPosition + 1)&(aCPU->mCodeMemSize-1)], mem);
    sprintf(aBuffer,"ANL   %s, #%02Xh",        
        mem,
        aCPU->mCodeMem[(aPosition + 2)&(aCPU->mCodeMemSize-1)]);
    
    return 3;
}

static int disasm_anl_a_imm(struct em8051 *aCPU, int aPosition, char *aBuffer)
{
    sprintf(aBuffer,"ANL   A, #%02Xh",
        aCPU->mCodeMem[(aPosition + 1)&(aCPU->mCodeMemSize-1)]);
    return 2;
}

static int disasm_anl_a_mem(struct em8051 *aCPU, int aPosition, char *aBuffer)
{
    char mem[32];mem_mnemonic(aCPU->mCodeMem[(aPosition + 1)&(aCPU->mCodeMemSize-1)], mem);
    sprintf(aBuffer,"ANL   A, %s",
        mem);
    
    return 2;
}

static int disasm_anl_a_indir_rx(struct em8051 *aCPU, int aPosition, char *aBuffer)
{
    sprintf(aBuffer,"ANL   A, @R%d",        
        aCPU->mCodeMem[(aPosition)&(aCPU->mCodeMemSize-1)]&1);
    return 1;
}


static int disasm_jz_offset(struct em8051 *aCPU, int aPosition, char *aBuffer)
{
    sprintf(aBuffer,"JZ    #%+d",        
        (signed char)aCPU->mCodeMem[(aPosition + 1)&(aCPU->mCodeMemSize-1)]);
    return 2;
}

static int disasm_xrl_mem_a(struct em8051 *aCPU, int aPosition, char *aBuffer)
{
    char mem[32];mem_mnemonic(aCPU->mCodeMem[(aPosition + 1)&(aCPU->mCodeMemSize-1)], mem);
    sprintf(aBuffer,"XRL   %s, A",
        mem);
    
    return 2;
}

static int disasm_xrl_mem_imm(struct em8051 *aCPU, int aPosition, char *aBuffer)
{
    char mem[32];mem_mnemonic(aCPU->mCodeMem[(aPosition + 1)&(aCPU->mCodeMemSize-1)], mem);
    sprintf(aBuffer,"XRL   %s, #%02Xh",        
        mem,
        aCPU->mCodeMem[(aPosition + 2)&(aCPU->mCodeMemSize-1)]);
    
    return 3;
}

static int disasm_xrl_a_imm(struct em8051 *aCPU, int aPosition, char *aBuffer)
{
    sprintf(aBuffer,"XRL   A, #%02Xh",
        aCPU->mCodeMem[(aPosition + 1)&(aCPU->mCodeMemSize-1)]);
    return 2;
}

static int disasm_xrl_a_mem(struct em8051 *aCPU, int aPosition, char *aBuffer)
{
    char mem[32];mem_mnemonic(aCPU->mCodeMem[(aPosition + 1)&(aCPU->mCodeMemSize-1)], mem);
    sprintf(aBuffer,"XRL   A, %s",
        mem);
    
    return 2;
}

static int disasm_xrl_a_indir_rx(struct em8051 *aCPU, int aPosition, char *aBuffer)
{
    sprintf(aBuffer,"XRL   A, @R%d",        
            aCPU->mCodeMem[(aPosition)&(aCPU->mCodeMemSize-1)]&1);
    return 1;
}


static int disasm_jnz_offset(struct em8051 *aCPU, int aPosition, char *aBuffer)
{
    sprintf(aBuffer,"JNZ   #%+d",        
        (signed char)aCPU->mCodeMem[(aPosition + 1)&(aCPU->mCodeMemSize-1)]);
    return 2;
}

static int disasm_orl_c_bitaddr(struct em8051 *aCPU, int aPosition, char *aBuffer)
{
    char baddr[32];
    bitaddr_mnemonic(aCPU->mCodeMem[(aPosition + 1)&(aCPU->mCodeMemSize-1)], baddr);
    sprintf(aBuffer,"ORL   C, %s",
        baddr);
    return 2;
}

static int disasm_jmp_indir_a_dptr(struct em8051 *aCPU, int aPosition, char *aBuffer)
{
    sprintf(aBuffer,"JMP   @A+DPTR");
    return 1;
}

static int disasm_mov_a_imm(struct em8051 *aCPU, int aPosition, char *aBuffer)
{
    sprintf(aBuffer,"MOV   A, #%02Xh",
        aCPU->mCodeMem[(aPosition + 1)&(aCPU->mCodeMemSize-1)]);
    return 2;
}

static int disasm_mov_mem_imm(struct em8051 *aCPU, int aPosition, char *aBuffer)
{
    char mem[32];mem_mnemonic(aCPU->mCodeMem[(aPosition + 1)&(aCPU->mCodeMemSize-1)], mem);
    sprintf(aBuffer,"MOV   %s, #%02Xh",        
        mem,
        aCPU->mCodeMem[(aPosition + 2)&(aCPU->mCodeMemSize-1)]);
    
    return 3;
}

static int disasm_mov_indir_rx_imm(struct em8051 *aCPU, int aPosition, char *aBuffer)
{
    sprintf(aBuffer,"MOV   @R%d, #%02Xh",        
        aCPU->mCodeMem[(aPosition)&(aCPU->mCodeMemSize-1)]&1,
        aCPU->mCodeMem[(aPosition + 1)&(aCPU->mCodeMemSize-1)]);
    return 2;
}


static int disasm_sjmp_offset(struct em8051 *aCPU, int aPosition, char *aBuffer)
{
    sprintf(aBuffer,"SJMP  #%+d",        
        (signed char)(aCPU->mCodeMem[(aPosition + 1)&(aCPU->mCodeMemSize-1)]));
    return 2;
}

static int disasm_anl_c_bitaddr(struct em8051 *aCPU, int aPosition, char *aBuffer)
{
    char baddr[32];
    bitaddr_mnemonic(aCPU->mCodeMem[(aPosition + 1)&(aCPU->mCodeMemSize-1)], baddr);
    sprintf(aBuffer,"ANL   C, %s",
        baddr);
    return 2;
}

static int disasm_movc_a_indir_a_pc(struct em8051 *aCPU, int aPosition, char *aBuffer)
{
    sprintf(aBuffer,"MOVC  A, @A+PC");
    return 1;
}

static int disasm_div_ab(struct em8051 *aCPU, int aPosition, char *aBuffer)
{
    sprintf(aBuffer,"DIV   AB");
    return 1;
}

static int disasm_mov_mem_mem(struct em8051 *aCPU, int aPosition, char *aBuffer)
{
    char mem1[32];
    char mem2[32];
    mem_mnemonic(aCPU->mCodeMem[(aPosition + 2)&(aCPU->mCodeMemSize-1)], mem1);
    mem_mnemonic(aCPU->mCodeMem[(aPosition + 1)&(aCPU->mCodeMemSize-1)], mem2);
    sprintf(aBuffer,"MOV   %s, %s",        
        mem1,
        mem2);
    return 3;
}

static int disasm_mov_mem_indir_rx(struct em8051 *aCPU, int aPosition, char *aBuffer)
{
    char mem[32];mem_mnemonic(aCPU->mCodeMem[(aPosition + 1)&(aCPU->mCodeMemSize-1)], mem);
    sprintf(aBuffer,"MOV   %s, @R%d",        
        mem,
        aCPU->mCodeMem[(aPosition)&(aCPU->mCodeMemSize-1)]&1);
    
    return 2;
}


static int disasm_mov_dptr_imm(struct em8051 *aCPU, int aPosition, char *aBuffer)
{
    sprintf(aBuffer,"MOV   DPTR, #0%02X%02Xh",        
        aCPU->mCodeMem[(aPosition + 1)&(aCPU->mCodeMemSize-1)],
        aCPU->mCodeMem[(aPosition + 2)&(aCPU->mCodeMemSize-1)]);
    return 3;
}

static int disasm_mov_bitaddr_c(struct em8051 *aCPU, int aPosition, char *aBuffer) 
{
    char baddr[32];
    bitaddr_mnemonic(aCPU->mCodeMem[(aPosition + 1)&(aCPU->mCodeMemSize-1)], baddr);
    sprintf(aBuffer,"MOV   %s, C",
        baddr);
    return 2;
}

static int disasm_movc_a_indir_a_dptr(struct em8051 *aCPU, int aPosition, char *aBuffer)
{
    sprintf(aBuffer,"MOVC  A, @A+DPTR");
    return 1;
}

static int disasm_subb_a_imm(struct em8051 *aCPU, int aPosition, char *aBuffer)
{
    sprintf(aBuffer,"SUBB  A, #%02Xh",
        aCPU->mCodeMem[(aPosition + 1)&(aCPU->mCodeMemSize-1)]);
    return 2;
}

static int disasm_subb_a_mem(struct em8051 *aCPU, int aPosition, char *aBuffer) 
{
    char mem[32];mem_mnemonic(aCPU->mCodeMem[(aPosition + 1)&(aCPU->mCodeMemSize-1)], mem);
    sprintf(aBuffer,"SUBB  A, %s",
        mem);
    
    return 2;
}
static int disasm_subb_a_indir_rx(struct em8051 *aCPU, int aPosition, char *aBuffer)
{
    sprintf(aBuffer,"SUBB  A, @R%d",        
        aCPU->mCodeMem[(aPosition)&(aCPU->mCodeMemSize-1)]&1);
    return 1;
}


static int disasm_orl_c_compl_bitaddr(struct em8051 *aCPU, int aPosition, char *aBuffer)
{
    char baddr[32];
    bitaddr_mnemonic(aCPU->mCodeMem[(aPosition + 1)&(aCPU->mCodeMemSize-1)], baddr);
    sprintf(aBuffer,"ORL   C, /%s",
        baddr);
    return 2;
}

static int disasm_mov_c_bitaddr(struct em8051 *aCPU, int aPosition, char *aBuffer) 
{
    char baddr[32];
    bitaddr_mnemonic(aCPU->mCodeMem[(aPosition + 1)&(aCPU->mCodeMemSize-1)], baddr);
    sprintf(aBuffer,"MOV   C, %s",
        baddr);
    return 2;
}

static int disasm_inc_dptr(struct em8051 *aCPU, int aPosition, char *aBuffer)
{
    sprintf(aBuffer,"INC   DPTR");
    return 1;
}

static int disasm_mul_ab(struct em8051 *aCPU, int aPosition, char *aBuffer)
{
    sprintf(aBuffer,"MUL   AB");
    return 1;
}

static int disasm_mov_indir_rx_mem(struct em8051 *aCPU, int aPosition, char *aBuffer)
{
    char mem[32];mem_mnemonic(aCPU->mCodeMem[(aPosition + 1)&(aCPU->mCodeMemSize-1)], mem);
    sprintf(aBuffer,"MOV   @R%d, %s",        
        aCPU->mCodeMem[(aPosition)&(aCPU->mCodeMemSize-1)]&1,
        mem);
    
    return 2;
}


static int disasm_anl_c_compl_bitaddr(struct em8051 *aCPU, int aPosition, char *aBuffer)
{
    char baddr[32];
    bitaddr_mnemonic(aCPU->mCodeMem[(aPosition + 1)&(aCPU->mCodeMemSize-1)], baddr);
    sprintf(aBuffer,"ANL   C, /%s",
        baddr);
    return 2;
}


static int disasm_cpl_bitaddr(struct em8051 *aCPU, int aPosition, char *aBuffer)
{
    char baddr[32];
    bitaddr_mnemonic(aCPU->mCodeMem[(aPosition + 1)&(aCPU->mCodeMemSize-1)], baddr);
    sprintf(aBuffer,"CPL   %s",
        baddr);
    return 2;
}

static int disasm_cpl_c(struct em8051 *aCPU, int aPosition, char *aBuffer)
{
    sprintf(aBuffer,"CPL   C");
    return 1;
}

static int disasm_cjne_a_imm_offset(struct em8051 *aCPU, int aPosition, char *aBuffer)
{
    sprintf(aBuffer,"CJNE  A, #%02Xh, #%+d",
        aCPU->mCodeMem[(aPosition + 1)&(aCPU->mCodeMemSize-1)],
        (signed char)(aCPU->mCodeMem[(aPosition + 2)&(aCPU->mCodeMemSize-1)]));
    return 3;
}

static int disasm_cjne_a_mem_offset(struct em8051 *aCPU, int aPosition, char *aBuffer)
{
    char mem[32];mem_mnemonic(aCPU->mCodeMem[(aPosition + 1)&(aCPU->mCodeMemSize-1)], mem);
    sprintf(aBuffer,"CJNE  A, %s, #%+d",
        mem,
        (signed char)(aCPU->mCodeMem[(aPosition + 2)&(aCPU->mCodeMemSize-1)]));
    
    return 3;
}
static int disasm_cjne_indir_rx_imm_offset(struct em8051 *aCPU, int aPosition, char *aBuffer)
{
    sprintf(aBuffer,"CJNE  @R%d, #%02Xh, #%+d",
        aCPU->mCodeMem[(aPosition)&(aCPU->mCodeMemSize-1)]&1,
        aCPU->mCodeMem[(aPosition + 1)&(aCPU->mCodeMemSize-1)],
        (signed char)(aCPU->mCodeMem[(aPosition + 2)&(aCPU->mCodeMemSize-1)]));
    return 3;
}

static int disasm_push_mem(struct em8051 *aCPU, int aPosition, char *aBuffer)
{
    char mem[32];mem_mnemonic(aCPU->mCodeMem[(aPosition + 1)&(aCPU->mCodeMemSize-1)], mem);
    sprintf(aBuffer,"PUSH  %s",
        mem);
    
    return 2;
}


static int disasm_clr_bitaddr(struct em8051 *aCPU, int aPosition, char *aBuffer)
{
    char baddr[32];
    bitaddr_mnemonic(aCPU->mCodeMem[(aPosition + 1)&(aCPU->mCodeMemSize-1)], baddr);
    sprintf(aBuffer,"CLR   %s",
        baddr);
    return 2;
}

static int disasm_clr_c(struct em8051 *aCPU, int aPosition, char *aBuffer)
{
    sprintf(aBuffer,"CLR   C");
    return 1;
}

static int disasm_swap_a(struct em8051 *aCPU, int aPosition, char *aBuffer)
{
    sprintf(aBuffer,"SWAP  A");
    return 1;
}

static int disasm_xch_a_mem(struct em8051 *aCPU, int aPosition, char *aBuffer)
{
    char mem[32];mem_mnemonic(aCPU->mCodeMem[(aPosition + 1)&(aCPU->mCodeMemSize-1)], mem);
    sprintf(aBuffer,"XCH   A, %s",
        mem);
    
    return 2;
}

static int disasm_xch_a_indir_rx(struct em8051 *aCPU, int aPosition, char *aBuffer)
{
    sprintf(aBuffer,"XCH   A, @R%d",        
        aCPU->mCodeMem[(aPosition)&(aCPU->mCodeMemSize-1)]&1);
    return 1;
}


static int disasm_pop_mem(struct em8051 *aCPU, int aPosition, char *aBuffer)
{
    char mem[32];mem_mnemonic(aCPU->mCodeMem[(aPosition + 1)&(aCPU->mCodeMemSize-1)], mem);
    sprintf(aBuffer,"POP   %s",
        mem);
    
    return 2;
}

static int disasm_setb_bitaddr(struct em8051 *aCPU, int aPosition, char *aBuffer)
{
    char baddr[32];
    bitaddr_mnemonic(aCPU->mCodeMem[(aPosition + 1)&(aCPU->mCodeMemSize-1)], baddr);
    sprintf(aBuffer,"SETB  %s",
        baddr);
    return 2;
}

static int disasm_setb_c(struct em8051 *aCPU, int aPosition, char *aBuffer)
{
    sprintf(aBuffer,"SETB  C");
    return 1;
}

static int disasm_da_a(struct em8051 *aCPU, int aPosition, char *aBuffer)
{
    sprintf(aBuffer,"DA    A");
    return 1;
}

static int disasm_djnz_mem_offset(struct em8051 *aCPU, int aPosition, char *aBuffer)
{
    char mem[32];mem_mnemonic(aCPU->mCodeMem[(aPosition + 1)&(aCPU->mCodeMemSize-1)], mem);
    sprintf(aBuffer,"DJNZ  %s, #%+d",        
        mem,
        (signed char)(aCPU->mCodeMem[(aPosition + 2)&(aCPU->mCodeMemSize-1)]));
    return 3;
}

static int disasm_xchd_a_indir_rx(struct em8051 *aCPU, int aPosition, char *aBuffer)
{
    sprintf(aBuffer,"XCHD  A, @R%d",        
        aCPU->mCodeMem[(aPosition)&(aCPU->mCodeMemSize-1)]&1);
    return 1;
}


static int disasm_movx_a_indir_dptr(struct em8051 *aCPU, int aPosition, char *aBuffer)
{
    sprintf(aBuffer,"MOVX  A, @DPTR");
    return 1;
}

static int disasm_movx_a_indir_rx(struct em8051 *aCPU, int aPosition, char *aBuffer)
{
    sprintf(aBuffer,"MOVX  A, @R%d",        
        aCPU->mCodeMem[(aPosition)&(aCPU->mCodeMemSize-1)]&1);
    return 1;
}

static int disasm_clr_a(struct em8051 *aCPU, int aPosition, char *aBuffer)
{
    sprintf(aBuffer,"CLR   A");
    return 1;
}

static int disasm_mov_a_mem(struct em8051 *aCPU, int aPosition, char *aBuffer)
{
    char mem[32];mem_mnemonic(aCPU->mCodeMem[(aPosition + 1)&(aCPU->mCodeMemSize-1)], mem);
    sprintf(aBuffer,"MOV   A, %s",
        mem);
    
    return 2;
}

static int disasm_mov_a_indir_rx(struct em8051 *aCPU, int aPosition, char *aBuffer)
{
    sprintf(aBuffer,"MOV   A, @R%d",        
        aCPU->mCodeMem[(aPosition)&(aCPU->mCodeMemSize-1)]&1);
    return 1;
}


static int disasm_movx_indir_dptr_a(struct em8051 *aCPU, int aPosition, char *aBuffer)
{
    sprintf(aBuffer,"MOVX  @DPTR, A");
    return 1;
}

static int disasm_movx_indir_rx_a(struct em8051 *aCPU, int aPosition, char *aBuffer)
{
    sprintf(aBuffer,"MOVX  @R%d, A",        
        aCPU->mCodeMem[(aPosition)&(aCPU->mCodeMemSize-1)]&1);
    return 1;
}

static int disasm_cpl_a(struct em8051 *aCPU, int aPosition, char *aBuffer)
{
    sprintf(aBuffer,"CPL   A");
    return 1;
}

static int disasm_mov_mem_a(struct em8051 *aCPU, int aPosition, char *aBuffer)
{
    char mem[32];mem_mnemonic(aCPU->mCodeMem[(aPosition + 1)&(aCPU->mCodeMemSize-1)], mem);
    sprintf(aBuffer,"MOV   %s, A",
        mem);
    
    return 2;
}

static int disasm_mov_indir_rx_a(struct em8051 *aCPU, int aPosition, char *aBuffer)
{
    sprintf(aBuffer,"MOV   @R%d, A",
        aCPU->mCodeMem[(aPosition)&(aCPU->mCodeMemSize-1)]&1);
    return 1;
}

static int disasm_nop(struct em8051 *aCPU, int aPosition, char *aBuffer)
{
    if (aCPU->mCodeMem[aPosition&(aCPU->mCodeMemSize-1)])
        sprintf(aBuffer,"??UNKNOWN");
    else
        sprintf(aBuffer,"NOP");
    return 1;
}

static int disasm_inc_rx(struct em8051 *aCPU, int aPosition, char *aBuffer)
{
    sprintf(aBuffer,"INC   R%d",aCPU->mCodeMem[aPosition&(aCPU->mCodeMemSize-1)]&7);
    return 1;
}

static int disasm_dec_rx(struct em8051 *aCPU, int aPosition, char *aBuffer)
{
    sprintf(aBuffer,"DEC   R%d",aCPU->mCodeMem[aPosition&(aCPU->mCodeMemSize-1)]&7);
    return 1;
}

static int disasm_add_a_rx(struct em8051 *aCPU, int aPosition, char *aBuffer)
{
    sprintf(aBuffer,"ADD   A, R%d",aCPU->mCodeMem[aPosition&(aCPU->mCodeMemSize-1)]&7);
    return 1;
}

static int disasm_addc_a_rx(struct em8051 *aCPU, int aPosition, char *aBuffer)
{
    sprintf(aBuffer,"ADDC  A, R%d",aCPU->mCodeMem[aPosition&(aCPU->mCodeMemSize-1)]&7);
    return 1;
}

static int disasm_orl_a_rx(struct em8051 *aCPU, int aPosition, char *aBuffer)
{
    sprintf(aBuffer,"ORL   A, R%d",aCPU->mCodeMem[aPosition&(aCPU->mCodeMemSize-1)]&7);
    return 1;
}

static int disasm_anl_a_rx(struct em8051 *aCPU, int aPosition, char *aBuffer)
{
    sprintf(aBuffer,"ANL   A, R%d",aCPU->mCodeMem[aPosition&(aCPU->mCodeMemSize-1)]&7);
    return 1;
}

static int disasm_xrl_a_rx(struct em8051 *aCPU, int aPosition, char *aBuffer)
{
    sprintf(aBuffer,"XRL   A, R%d",aCPU->mCodeMem[aPosition&(aCPU->mCodeMemSize-1)]&7);
    return 1;
}


static int disasm_mov_rx_imm(struct em8051 *aCPU, int aPosition, char *aBuffer)
{
    sprintf(aBuffer,"MOV   R%d, #%02Xh",
        aCPU->mCodeMem[aPosition&(aCPU->mCodeMemSize-1)]&7,
        aCPU->mCodeMem[(aPosition + 1)&(aCPU->mCodeMemSize-1)]);
    return 2;
}

static int disasm_mov_mem_rx(struct em8051 *aCPU, int aPosition, char *aBuffer)
{
    char mem[32];mem_mnemonic(aCPU->mCodeMem[(aPosition + 1)&(aCPU->mCodeMemSize-1)], mem);
    sprintf(aBuffer,"MOV   %s, R%d",
        mem,
        aCPU->mCodeMem[aPosition&(aCPU->mCodeMemSize-1)]&7);
    
    return 2;
}

static int disasm_subb_a_rx(struct em8051 *aCPU, int aPosition, char *aBuffer)
{
    sprintf(aBuffer,"SUBB  A, R%d",aCPU->mCodeMem[aPosition&(aCPU->mCodeMemSize-1)]&7);
    return 1;
}

static int disasm_mov_rx_mem(struct em8051 *aCPU, int aPosition, char *aBuffer)
{
    char mem[32];mem_mnemonic(aCPU->mCodeMem[(aPosition + 1)&(aCPU->mCodeMemSize-1)], mem);
    sprintf(aBuffer,"MOV   R%d, %s",
        aCPU->mCodeMem[aPosition&(aCPU->mCodeMemSize-1)]&7,
        mem);
    
    return 2;
}

static int disasm_cjne_rx_imm_offset(struct em8051 *aCPU, int aPosition, char *aBuffer)
{
    sprintf(aBuffer,"CJNE  R%d, #%02Xh, #%+d",
        aCPU->mCodeMem[aPosition&(aCPU->mCodeMemSize-1)]&7,
        aCPU->mCodeMem[(aPosition + 1)&(aCPU->mCodeMemSize-1)],
        (signed char)aCPU->mCodeMem[(aPosition + 2)&(aCPU->mCodeMemSize-1)]);
    return 3;
}

static int disasm_xch_a_rx(struct em8051 *aCPU, int aPosition, char *aBuffer)
{
    sprintf(aBuffer,"XCH   A, R%d",aCPU->mCodeMem[aPosition&(aCPU->mCodeMemSize-1)]&7);
    return 1;
}

static int disasm_djnz_rx_offset(struct em8051 *aCPU, int aPosition, char *aBuffer)
{
    sprintf(aBuffer,"DJNZ  R%d, #%+d",
        aCPU->mCodeMem[aPosition&(aCPU->mCodeMemSize-1)]&7,
        (signed char)aCPU->mCodeMem[(aPosition + 1)&(aCPU->mCodeMemSize-1)]);
    return 2;
}

static int disasm_mov_a_rx(struct em8051 *aCPU, int aPosition, char *aBuffer)
{
    sprintf(aBuffer,"MOV   A, R%d",aCPU->mCodeMem[aPosition&(aCPU->mCodeMemSize-1)]&7);
    return 1;
}

static int disasm_mov_rx_a(struct em8051 *aCPU, int aPosition, char *aBuffer)
{
    sprintf(aBuffer,"MOV   R%d, A",aCPU->mCodeMem[aPosition&(aCPU->mCodeMemSize-1)]&7);
    return 1;
}

void disasm_setptrs(struct em8051 *aCPU)
{
    int i;
    for (i = 0; i < 8; i++)
    {
        aCPU->dec[0x08 + i] = &disasm_inc_rx;
        aCPU->dec[0x18 + i] = &disasm_dec_rx;
        aCPU->dec[0x28 + i] = &disasm_add_a_rx;
        aCPU->dec[0x38 + i] = &disasm_addc_a_rx;
        aCPU->dec[0x48 + i] = &disasm_orl_a_rx;
        aCPU->dec[0x58 + i] = &disasm_anl_a_rx;
        aCPU->dec[0x68 + i] = &disasm_xrl_a_rx;
        aCPU->dec[0x78 + i] = &disasm_mov_rx_imm;
        aCPU->dec[0x88 + i] = &disasm_mov_mem_rx;
        aCPU->dec[0x98 + i] = &disasm_subb_a_rx;
        aCPU->dec[0xa8 + i] = &disasm_mov_rx_mem;
        aCPU->dec[0xb8 + i] = &disasm_cjne_rx_imm_offset;
        aCPU->dec[0xc8 + i] = &disasm_xch_a_rx;
        aCPU->dec[0xd8 + i] = &disasm_djnz_rx_offset;
        aCPU->dec[0xe8 + i] = &disasm_mov_a_rx;
        aCPU->dec[0xf8 + i] = &disasm_mov_rx_a;
    }
    aCPU->dec[0x00] = &disasm_nop;
    aCPU->dec[0x01] = &disasm_ajmp_offset;
    aCPU->dec[0x02] = &disasm_ljmp_address;
    aCPU->dec[0x03] = &disasm_rr_a;
    aCPU->dec[0x04] = &disasm_inc_a;
    aCPU->dec[0x05] = &disasm_inc_mem;
    aCPU->dec[0x06] = &disasm_inc_indir_rx;
    aCPU->dec[0x07] = &disasm_inc_indir_rx;

    aCPU->dec[0x10] = &disasm_jbc_bitaddr_offset;
    aCPU->dec[0x11] = &disasm_acall_offset;
    aCPU->dec[0x12] = &disasm_lcall_address;
    aCPU->dec[0x13] = &disasm_rrc_a;
    aCPU->dec[0x14] = &disasm_dec_a;
    aCPU->dec[0x15] = &disasm_dec_mem;
    aCPU->dec[0x16] = &disasm_dec_indir_rx;
    aCPU->dec[0x17] = &disasm_dec_indir_rx;

    aCPU->dec[0x20] = &disasm_jb_bitaddr_offset;
    aCPU->dec[0x21] = &disasm_ajmp_offset;
    aCPU->dec[0x22] = &disasm_ret;
    aCPU->dec[0x23] = &disasm_rl_a;
    aCPU->dec[0x24] = &disasm_add_a_imm;
    aCPU->dec[0x25] = &disasm_add_a_mem;
    aCPU->dec[0x26] = &disasm_add_a_indir_rx;
    aCPU->dec[0x27] = &disasm_add_a_indir_rx;

    aCPU->dec[0x30] = &disasm_jnb_bitaddr_offset;
    aCPU->dec[0x31] = &disasm_acall_offset;
    aCPU->dec[0x32] = &disasm_reti;
    aCPU->dec[0x33] = &disasm_rlc_a;
    aCPU->dec[0x34] = &disasm_addc_a_imm;
    aCPU->dec[0x35] = &disasm_addc_a_mem;
    aCPU->dec[0x36] = &disasm_addc_a_indir_rx;
    aCPU->dec[0x37] = &disasm_addc_a_indir_rx;

    aCPU->dec[0x40] = &disasm_jc_offset;
    aCPU->dec[0x41] = &disasm_ajmp_offset;
    aCPU->dec[0x42] = &disasm_orl_mem_a;
    aCPU->dec[0x43] = &disasm_orl_mem_imm;
    aCPU->dec[0x44] = &disasm_orl_a_imm;
    aCPU->dec[0x45] = &disasm_orl_a_mem;
    aCPU->dec[0x46] = &disasm_orl_a_indir_rx;
    aCPU->dec[0x47] = &disasm_orl_a_indir_rx;

    aCPU->dec[0x50] = &disasm_jnc_offset;
    aCPU->dec[0x51] = &disasm_acall_offset;
    aCPU->dec[0x52] = &disasm_anl_mem_a;
    aCPU->dec[0x53] = &disasm_anl_mem_imm;
    aCPU->dec[0x54] = &disasm_anl_a_imm;
    aCPU->dec[0x55] = &disasm_anl_a_mem;
    aCPU->dec[0x56] = &disasm_anl_a_indir_rx;
    aCPU->dec[0x57] = &disasm_anl_a_indir_rx;

    aCPU->dec[0x60] = &disasm_jz_offset;
    aCPU->dec[0x61] = &disasm_ajmp_offset;
    aCPU->dec[0x62] = &disasm_xrl_mem_a;
    aCPU->dec[0x63] = &disasm_xrl_mem_imm;
    aCPU->dec[0x64] = &disasm_xrl_a_imm;
    aCPU->dec[0x65] = &disasm_xrl_a_mem;
    aCPU->dec[0x66] = &disasm_xrl_a_indir_rx;
    aCPU->dec[0x67] = &disasm_xrl_a_indir_rx;

    aCPU->dec[0x70] = &disasm_jnz_offset;
    aCPU->dec[0x71] = &disasm_acall_offset;
    aCPU->dec[0x72] = &disasm_orl_c_bitaddr;
    aCPU->dec[0x73] = &disasm_jmp_indir_a_dptr;
    aCPU->dec[0x74] = &disasm_mov_a_imm;
    aCPU->dec[0x75] = &disasm_mov_mem_imm;
    aCPU->dec[0x76] = &disasm_mov_indir_rx_imm;
    aCPU->dec[0x77] = &disasm_mov_indir_rx_imm;

    aCPU->dec[0x80] = &disasm_sjmp_offset;
    aCPU->dec[0x81] = &disasm_ajmp_offset;
    aCPU->dec[0x82] = &disasm_anl_c_bitaddr;
    aCPU->dec[0x83] = &disasm_movc_a_indir_a_pc;
    aCPU->dec[0x84] = &disasm_div_ab;
    aCPU->dec[0x85] = &disasm_mov_mem_mem;
    aCPU->dec[0x86] = &disasm_mov_mem_indir_rx;
    aCPU->dec[0x87] = &disasm_mov_mem_indir_rx;

    aCPU->dec[0x90] = &disasm_mov_dptr_imm;
    aCPU->dec[0x91] = &disasm_acall_offset;
    aCPU->dec[0x92] = &disasm_mov_bitaddr_c;
    aCPU->dec[0x93] = &disasm_movc_a_indir_a_dptr;
    aCPU->dec[0x94] = &disasm_subb_a_imm;
    aCPU->dec[0x95] = &disasm_subb_a_mem;
    aCPU->dec[0x96] = &disasm_subb_a_indir_rx;
    aCPU->dec[0x97] = &disasm_subb_a_indir_rx;

    aCPU->dec[0xa0] = &disasm_orl_c_compl_bitaddr;
    aCPU->dec[0xa1] = &disasm_ajmp_offset;
    aCPU->dec[0xa2] = &disasm_mov_c_bitaddr;
    aCPU->dec[0xa3] = &disasm_inc_dptr;
    aCPU->dec[0xa4] = &disasm_mul_ab;
    aCPU->dec[0xa5] = &disasm_nop; // unused
    aCPU->dec[0xa6] = &disasm_mov_indir_rx_mem;
    aCPU->dec[0xa7] = &disasm_mov_indir_rx_mem;

    aCPU->dec[0xb0] = &disasm_anl_c_compl_bitaddr;
    aCPU->dec[0xb1] = &disasm_acall_offset;
    aCPU->dec[0xb2] = &disasm_cpl_bitaddr;
    aCPU->dec[0xb3] = &disasm_cpl_c;
    aCPU->dec[0xb4] = &disasm_cjne_a_imm_offset;
    aCPU->dec[0xb5] = &disasm_cjne_a_mem_offset;
    aCPU->dec[0xb6] = &disasm_cjne_indir_rx_imm_offset;
    aCPU->dec[0xb7] = &disasm_cjne_indir_rx_imm_offset;

    aCPU->dec[0xc0] = &disasm_push_mem;
    aCPU->dec[0xc1] = &disasm_ajmp_offset;
    aCPU->dec[0xc2] = &disasm_clr_bitaddr;
    aCPU->dec[0xc3] = &disasm_clr_c;
    aCPU->dec[0xc4] = &disasm_swap_a;
    aCPU->dec[0xc5] = &disasm_xch_a_mem;
    aCPU->dec[0xc6] = &disasm_xch_a_indir_rx;
    aCPU->dec[0xc7] = &disasm_xch_a_indir_rx;

    aCPU->dec[0xd0] = &disasm_pop_mem;
    aCPU->dec[0xd1] = &disasm_acall_offset;
    aCPU->dec[0xd2] = &disasm_setb_bitaddr;
    aCPU->dec[0xd3] = &disasm_setb_c;
    aCPU->dec[0xd4] = &disasm_da_a;
    aCPU->dec[0xd5] = &disasm_djnz_mem_offset;
    aCPU->dec[0xd6] = &disasm_xchd_a_indir_rx;
    aCPU->dec[0xd7] = &disasm_xchd_a_indir_rx;

    aCPU->dec[0xe0] = &disasm_movx_a_indir_dptr;
    aCPU->dec[0xe1] = &disasm_ajmp_offset;
    aCPU->dec[0xe2] = &disasm_movx_a_indir_rx;
    aCPU->dec[0xe3] = &disasm_movx_a_indir_rx;
    aCPU->dec[0xe4] = &disasm_clr_a;
    aCPU->dec[0xe5] = &disasm_mov_a_mem;
    aCPU->dec[0xe6] = &disasm_mov_a_indir_rx;
    aCPU->dec[0xe7] = &disasm_mov_a_indir_rx;

    aCPU->dec[0xf0] = &disasm_movx_indir_dptr_a;
    aCPU->dec[0xf1] = &disasm_acall_offset;
    aCPU->dec[0xf2] = &disasm_movx_indir_rx_a;
    aCPU->dec[0xf3] = &disasm_movx_indir_rx_a;
    aCPU->dec[0xf4] = &disasm_cpl_a;
    aCPU->dec[0xf5] = &disasm_mov_mem_a;
    aCPU->dec[0xf6] = &disasm_mov_indir_rx_a;
    aCPU->dec[0xf7] = &disasm_mov_indir_rx_a;
}
