#ifndef SVM_H
#define SVM_H

#include <stdint.h>
#include "flashlayer.h"

class SvmProgram;

class SvmMemory {
public:
    uint32_t word(uint32_t address);

private:
    static const unsigned MEM_SIZE = 16 * 1024;
    uint8_t mem[MEM_SIZE];

    friend class SvmProgram;
};

class SvmProgram {
public:
    SvmProgram();

    uint16_t fetch();
    void run();
    void cycle();
    void validate();

private:
    // 16-bit thumb instruction validators
    static const uint16_t AluMask           = 0x3 << 14;    // 0b11xxxxxx xxxxxxxx
    static const uint16_t AluTest           = 0;

    static const uint16_t DataProcMask      = 0x3f << 10;   // 0b111111xx xxxxxxxx
    static const uint16_t DataProcTest      = 0x10 << 10;   // 0b010000xx xxxxxxxx

    static const uint16_t MiscMask          = 0xff << 8;    // 0b11111111 xxxxxxxx
    static const uint16_t MiscTest          = 0xb2 << 8;    // 0b10110010 xxxxxxxx

    static const uint16_t SvcMask           = 0xff << 8;    // 0b11111111 xxxxxxxx
    static const uint16_t SvcTest           = 0xdf << 8;    // 0b11011111 xxxxxxxx

    static const uint16_t PcRelLdrMask      = 0x1f << 11;   // 0b11111xxx xxxxxxxx
    static const uint16_t PcRelLdrTest      = 0x9 << 11;    // 0b01001xxx xxxxxxxx

    static const uint16_t SpRelLdrStrMask   = 0xf << 12;    // 0b1111xxxx xxxxxxxx
    static const uint16_t SpRelLdrStrTest   = 0x9 << 12;    // 0b1001xxxx xxxxxxxx

    static const uint16_t SpRelAddMask      = 0x1f << 11;   // 0b11111xxx xxxxxxxx
    static const uint16_t SpRelAddTest      = 0x15 << 11;   // 0b10101xxx xxxxxxxx

    static const uint16_t UncondBranchMask  = 0x1f << 11;   // 0b11111xxx xxxxxxxx
    static const uint16_t UncondBranchTest  = 0x1c << 11;   // 0b11100xxx xxxxxxxx

    static const uint16_t CondBranchMask    = 0xf << 12;    // 0b1111xxxx xxxxxxxx
    static const uint16_t CondBranchTest    = 0xd << 12;    // 0b1101xxxx xxxxxxxx

    static const uint16_t CompareBranchMask = 0xf5 << 8;    // 0b11110101 xxxxxxxx
    static const uint16_t CompareBranchTest = 0xb1 << 8;    // 0b1011x0x1 xxxxxxxx

    static const uint16_t Nop               = 0xbf00;       // 0b10111111 00000000

    // 32-bit thumb instruction validators
    static const uint32_t StrMask   = 0x1ffff << 15;    // 0b11111111 11111111, 1xxxxxxx xxxxxxxx
    static const uint32_t StrTest   = 0x1f192 << 15;    // 0b11111000 11001001, 0xxxxxxx xxxxxxxx

    static const uint32_t StrBhMask = 0x1ffbf << 15;    // 0b11111111 11011111, 1xxxxxxx xxxxxxxx
    static const uint32_t StrBhTest = 0x1f112 << 15;    // 0b11111000 10x01001, 0xxxxxxx xxxxxxxx

    static const uint32_t LdrBhMask = 0x1fdbd << 15;    // 0b11111110 11011110, 1xxxxxxx xxxxxxxx
    static const uint32_t LdrBhTest = 0x1f130 << 15;    // 0b1111100x 10x1100x, 0xxxxxxx xxxxxxxx

    static const uint32_t LdrMask   = 0x1fffd << 15;    // 0b11111111 1111111x, 1xxxxxxx xxxxxxxx
    static const uint32_t LdrTest   = 0x1f1b0 << 15;    // 0b11111000 1101100x, 0xxxxxxx xxxxxxxx

    static const uint32_t MovWtMask = 0x1f6e11 << 11;   // 0b11111011 01110000, 10001xxx xxxxxxxx
    static const uint32_t MovWtTest = 0x1e4800 << 11;   // 0b11110x10 x100xxxx, 0xxx0xxx xxxxxxxx

    struct Segment {
        uint32_t start;
        uint32_t size;
    };

    struct ProgramInfo {
        uint32_t entry;
        Segment textRodata;
        Segment bss;
        Segment rwdata;
    };

    enum InstructionSize {
        InstrBits16,
        InstrBits32
    };

    bool loadElfFile(unsigned addr, unsigned len);

    void execute16(uint16_t instr);
    void execute32(uint32_t instr);

    bool isValid16(uint16_t instr);
    bool isValid32(uint32_t instr);

    FlashRegion flashRegion;
    uint16_t currentRegionOffset;
    ProgramInfo progInfo;

    static const unsigned NUM_GP_REGS = 16;
    static const unsigned REG_PC = 15;
    static const unsigned REG_LR = 14;
    static const unsigned REG_SP = 13;
    uint32_t regs[NUM_GP_REGS];     // general purpose registers
    uint32_t cpsr;                  // current program status register

    uint32_t phys2virtAddr(uint32_t virtualAddr) {
        return ((virtualAddr - 0x10000) & 0xFFFFF) + 0x20008000;
    }

    bool conditionPassed(uint8_t cond);

    InstructionSize instructionSize(uint16_t instr) const;

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
    inline bool getNeg() const {
        return cpsr & (1 << 31);
    }
    inline void setNeg() {
        cpsr |= (1 << 31);
    }
    inline void clrNeg() {
        cpsr &= ~(1 << 31);
    }

    inline bool getZero() const {
        return cpsr & (1 << 30);
    }
    inline void setZero() {
        cpsr |= 1 << 30;
    }
    inline void clrZero() {
        cpsr &= ~(1 << 30);
    }

    inline bool getCarry() const {
        return cpsr & (1 << 29);
    }
    inline void setCarry() {
        cpsr |= 1 << 29;
    }
    inline void clrCarry() {
        cpsr &= ~(1 << 29);
    }

    inline int getOverflow() const {
        return cpsr & (1 << 28);
    }
    inline void setOverflow() {
        cpsr |= (1 << 28);
    }
    inline void clrOverflow() {
        cpsr &= ~(1 << 28);
    }

    // shift/add/subtract/move/compare
    void emulateLSLImm(uint16_t inst);      // LSL (immediate)
    void emulateLSRImm(uint16_t inst);      // LSR (immediate)
    void emulateASRImm(uint16_t instr);     // ASR (immediate)
    void emulateADDReg(uint16_t instr);     // ADD (register)
    void emulateSUBReg(uint16_t instr);     // SUB (register)
    void emulateADD3Imm(uint16_t instr);    // ADD (immediate)
    void emulateSUB3Imm(uint16_t instr);    // SUB (immediate)
    void emulateMovImm(uint16_t instr);     // MOV (immediate)
    void emulateCmpImm(uint16_t instr);     // CMP (immediate)
    void emulateADD8Imm(uint16_t instr);    // ADD (immediate)
    void emulateSUB8Imm(uint16_t instr);    // SUB (immediate)

    // data processing
    void emulateANDReg(uint16_t instr);     // AND (register)
    void emulateEORReg(uint16_t instr);     // EOR (register)
    void emulateLSLReg(uint16_t instr);     // LSL (register)
    void emulateLSRReg(uint16_t instr);     // LSR (register)
    void emulateASRReg(uint16_t instr);     // ASR (register)
    void emulateADCReg(uint16_t instr);     // ADC (register)
    void emulateSBCReg(uint16_t instr);     // SBC (register)
    void emulateRORReg(uint16_t instr);     // ROR (register)
    void emulateTSTReg(uint16_t instr);     // TST (register)
    void emulateRSBImm(uint16_t instr);     // RSB (immediate)
    void emulateCMPReg(uint16_t instr);     // CMP (register)
    void emulateCMNReg(uint16_t instr);     // CMN (register)
    void emulateORRReg(uint16_t instr);     // ORR (register)
    void emulateMUL(uint16_t instr);        // MUL
    void emulateBICReg(uint16_t instr);     // BIC (register)
    void emulateMVNReg(uint16_t instr);     // MVN (register)

    // misc
    void emulateSXTH(uint16_t isntr);       // SXTH
    void emulateSXTB(uint16_t isntr);       // SXTB
    void emulateUXTH(uint16_t isntr);       // UXTH
    void emulateUXTB(uint16_t isntr);       // UXTB

    void emulateB(uint16_t instr);          // B
    void emulateCondB(uint16_t instr);      // B
    void emulateCBZ_CBNZ(uint16_t instr);   // CBNZ, CBZ

    void emulateSTRImm(uint16_t instr);     // STR (immediate)
    void emulateLDRImm(uint16_t instr);     // LDR (immediate)
    void emulateADDSpImm(uint16_t instr);   // ADD (SP plus immediate)

    void emulateLDRLitPool(uint16_t instr); // LDR (literal)

    void emulateSVC(uint16_t instr);

    // utils
    inline void BranchWritePC(uint32_t addr) {
        regs[REG_PC] = addr & 0xfffffffe;
    }

    // http://graphics.stanford.edu/~seander/bithacks.html#FixedSignExtend
    template <typename T, unsigned B>
    inline T SignExtend(const T x) {
      struct { T x:B; } s;
      return s.x = x;
    }
};

class Svm
{
public:
    Svm();
};

#endif // SVM_H
