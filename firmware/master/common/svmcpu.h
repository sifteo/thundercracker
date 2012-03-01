/*
 * Thundercracker Firmware -- Confidential, not for redistribution.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#ifndef SVMCPU_H
#define SVMCPU_H

#include "svm.h"

using namespace Svm;

class SvmRuntime;

class SvmCpu {
public:
    SvmCpu();  // Do not implement

    static void init();
    static void run();

    /*
        For now, register accessors are assumed to only really be used to access
        stacked registers during interrupt handling.
    */
    static reg_t reg(uint8_t r);
    static void setReg(uint8_t r, reg_t val);

    static const unsigned REG_FP = 11;
    static const unsigned REG_SP = 13;
    static const unsigned REG_LR = 14;
    static const unsigned REG_PC = 15;
    static const unsigned REG_CPSR = 16;
    static const unsigned NUM_REGS = 17;

private:
    template <typename T>
    static inline T Align(T x, T y) {
        return y * (x / y);
    }

    // http://graphics.stanford.edu/~seander/bithacks.html#FixedSignExtend
    template <typename T, unsigned B>
    static inline T SignExtend(const T x) {
        struct { T x:B; } s;
        return s.x = x;
    }

#if 0
    // this gets saved at interrupt by HW on cortex-m3
    struct HwStackFrame {
        reg_t r0;
        reg_t r1;
        reg_t r2;
        reg_t r3;
        reg_t r12;
        reg_t lr;   // link register
        reg_t pc;   // program counter
        reg_t psr;  // program status register
    };

    // the rest of the stack that must be saved by software
    struct SwStackFrame {
        reg_t r4;
        reg_t r5;
        reg_t r6;
        reg_t r7;
        reg_t r8;
        reg_t r9;
        reg_t r10;
        reg_t r11;
    };
#endif

#ifdef SIFTEO_SIMULATOR
    static reg_t regs[NUM_REGS];

    // ARM emulation support
    static uint16_t fetch();
    static void execute16(uint16_t instr);
    static void execute32(uint32_t instr);
    static bool conditionPassed(uint8_t cond);

    enum Conditions {
        EQ = 0,    // Equal
        NE = 1,    // Not Equal
        CS = 2,    // Carry Set
        CC = 3,    // Carry Clear
        MI = 4,    // Minus, Negative
        PL = 5,    // Plus, positive, or zero
        VS = 6,    // Overflow
        VC = 7,    // No overflow
        HI = 8,    // Unsigned higher
        LS = 9,    // Unsigned lower or same
        GE = 10,    // Signed greater than or equal
        LT = 11,    // Signed less than
        GT = 12,    // Signed greater than
        LE = 13,    // Signed less than or equal
        NoneAL = 14 // None, always, unconditional
    };

    // status flag helpers
    static inline bool getNeg() {
        return regs[REG_CPSR] & (1 << 31);
    }
    static inline void setNeg() {
        regs[REG_CPSR] |= (1 << 31);
    }
    static inline void clrNeg() {
        regs[REG_CPSR] &= ~(1 << 31);
    }

    static inline bool getZero() {
        return regs[REG_CPSR] & (1 << 30);
    }
    static inline void setZero() {
        regs[REG_CPSR] |= 1 << 30;
    }
    static inline void clrZero() {
        regs[REG_CPSR] &= ~(1 << 30);
    }

    static inline bool getCarry() {
        return regs[REG_CPSR] & (1 << 29);
    }
    static inline void setCarry() {
        regs[REG_CPSR] |= 1 << 29;
    }
    static inline void clrCarry() {
        regs[REG_CPSR] &= ~(1 << 29);
    }

    static inline int getOverflow() {
        return regs[REG_CPSR] & (1 << 28);
    }
    static inline void setOverflow() {
        regs[REG_CPSR] |= (1 << 28);
    }
    static inline void clrOverflow() {
        regs[REG_CPSR] &= ~(1 << 28);
    }
    
    static inline void branchOffsetPC(int offset) {
        setReg(REG_PC, (reg(REG_PC) + offset + 2) & ~(reg_t)1);
    }

    // shift/add/subtract/move/compare
    static void emulateLSLImm(uint16_t inst);      // LSL (immediate)
    static void emulateLSRImm(uint16_t inst);      // LSR (immediate)
    static void emulateASRImm(uint16_t instr);     // ASR (immediate)
    static void emulateADDReg(uint16_t instr);     // ADD (register)
    static void emulateSUBReg(uint16_t instr);     // SUB (register)
    static void emulateADD3Imm(uint16_t instr);    // ADD (immediate)
    static void emulateSUB3Imm(uint16_t instr);    // SUB (immediate)
    static void emulateMovImm(uint16_t instr);     // MOV (immediate)
    static void emulateCmpImm(uint16_t instr);     // CMP (immediate)
    static void emulateADD8Imm(uint16_t instr);    // ADD (immediate)
    static void emulateSUB8Imm(uint16_t instr);    // SUB (immediate)

    // data processing
    static void emulateANDReg(uint16_t instr);     // AND (register)
    static void emulateEORReg(uint16_t instr);     // EOR (register)
    static void emulateLSLReg(uint16_t instr);     // LSL (register)
    static void emulateLSRReg(uint16_t instr);     // LSR (register)
    static void emulateASRReg(uint16_t instr);     // ASR (register)
    static void emulateADCReg(uint16_t instr);     // ADC (register)
    static void emulateSBCReg(uint16_t instr);     // SBC (register)
    static void emulateRORReg(uint16_t instr);     // ROR (register)
    static void emulateTSTReg(uint16_t instr);     // TST (register)
    static void emulateRSBImm(uint16_t instr);     // RSB (immediate)
    static void emulateCMPReg(uint16_t instr);     // CMP (register)
    static void emulateCMNReg(uint16_t instr);     // CMN (register)
    static void emulateORRReg(uint16_t instr);     // ORR (register)
    static void emulateMUL(uint16_t instr);        // MUL
    static void emulateBICReg(uint16_t instr);     // BIC (register)
    static void emulateMVNReg(uint16_t instr);     // MVN (register)

    // misc
    static void emulateSXTH(uint16_t instr);       // SXTH
    static void emulateSXTB(uint16_t instr);       // SXTB
    static void emulateUXTH(uint16_t instr);       // UXTH
    static void emulateUXTB(uint16_t instr);       // UXTB

    static void emulateB(uint16_t instr);          // B
    static void emulateCondB(uint16_t instr);      // B
    static void emulateCBZ_CBNZ(uint16_t instr);   // CBNZ, CBZ

    static void emulateSTRSPImm(uint16_t instr);   // STR (SP plus immediate)
    static void emulateLDRSPImm(uint16_t instr);   // LDR (SP plus immediate)
    static void emulateADDSpImm(uint16_t instr);   // ADD (SP plus immediate)

    static void emulateLDRLitPool(uint16_t instr); // LDR (literal)

    static void emulateSVC(uint16_t instr);

    // 32-bit instructions
    static void emulateSTR(uint32_t instr);
    static void emulateSTRBH(uint32_t instr);
    static void emulateLDRBH(uint32_t instr);
    static void emulateLDR(uint32_t instr);
    static void emulateMOVWT(uint32_t instr);
    static void emulateDIV(uint32_t instr);
#endif
};

#endif // SVMCPU_H
