#ifndef SVM_H
#define SVM_H

#include <stdint.h>

enum Conditions {
    EQ = 0b0000,    // Equal
    NE = 0b0001,    // Not Equal
    CS = 0b0010,    // Carry Set
    CC = 0b0011,    // Carry Clear
    MI = 0b0100,    // Minus, Negative
    PL = 0b0101,    // Plus, positive, or zero
    VS = 0b0110,    // Overflow
    VC = 0b0111,    // No overflow
    HI = 0b1000,    // Unsigned higher
    LS = 0b1001,    // Unsigned lower or same
    GE = 0b1010,    // Signed greater than or equal
    LT = 0b1011,    // Signed less than
    GT = 0b1100,    // Signed greater than
    LE = 0b1101,    // Signed less than or equal
    NoneAL = 0b1110 // None, always, unconditional
};

class SvmProgram {
public:
    SvmProgram();

    void run();
    void cycle();
    void validate();

private:
    enum ValidationMasks {
        AluMask             = 0b11 << 14,
        AluTest             = 0,
        DataProcMask        = 0b111111 << 10,
        DataProcTest        = 0b010000 << 10,
        MiscMask            = 0b11111111 << 8,
        MiscTest            = 0b10110010 << 8,
        SvcMask             = 0b11111111 << 8,
        SvcTest             = 0b11011111 << 8,
        PcRelLdrMask        = 0b11111 << 11,
        PcRelLdrTest        = 0b01001 << 11,
        SpRelLdrStrMask     = 0b1111 << 12,
        SpRelLdrStrTest     = 0b1001 << 12,
        SpRelAddMask        = 0b11111 << 11,
        SpRelAddTest        = 0b10101 << 11,
        UncondBranchMask    = 0b11111 << 11,
        UncondBranchTest    = 0b11100 << 11,
        CondBranchMask      = 0b1111 << 12,
        CondBranchTest      = 0b1101 << 12,
        Nop                 = 0xbf00
    };

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

    bool loadElfFile(unsigned addr, unsigned len);

    bool decode16(uint16_t halfword);
    bool decode32();

    bool isValid16(uint16_t halfword);
    bool isValid32(uint32_t word);

    ProgramInfo progInfo;
};

class Svm
{
public:
    Svm();
};

#endif // SVM_H
