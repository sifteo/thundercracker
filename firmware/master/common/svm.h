/*
 * Thundercracker Firmware -- Confidential, not for redistribution.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#ifndef SVM_H
#define SVM_H

#include <stdint.h>
#include "macros.h"

/*
 * This header file defines functionality shared by multiple SVM components,
 * and which is defined by or closely related to the virtual machine
 * architecture itself, rather than our particular implementation.
 */

namespace Svm {

// Registers are wide enough to hold a native pointer
typedef uintptr_t reg_t;

// Register numbers
static const unsigned REG_FP = 11;
static const unsigned REG_SP = 13;
static const unsigned REG_LR = 14;
static const unsigned REG_PC = 15;
static const unsigned REG_CPSR = 16;
static const unsigned NUM_REGS = 17;

// ABI 'call' stack frame layout. The size of this struct must remain constant.
// The compiler relies it in order to calculate stack offsets for parameter passing.
struct CallFrame {
    uint32_t pc;
    uint32_t fp;
    uint32_t r2;
    uint32_t r3;
    uint32_t r4;
    uint32_t r5;
    uint32_t r6;
    uint32_t r7;
};

enum InstructionSize {
    InstrBits16,
    InstrBits32
};


/***************************************************************************
 * Debugger Support
 ***************************************************************************/

enum FaultCode {
    F_UNKNOWN = 0,
    F_STACK_OVERFLOW,       // Stack allocation failure
    F_BAD_STACK,            // Validation-time stack address error
    F_BAD_CODE_ADDRESS,     // Branch-time code address error
    F_BAD_SYSCALL,          // Unsupported syscall number
    F_LOAD_ADDRESS,         // Runtime load address error
    F_STORE_ADDRESS,        // Runtime store address error
    F_LOAD_ALIGNMENT,       // Runtime load alignment error
    F_STORE_ALIGNMENT,      // Runtime store alignment error
    F_CODE_FETCH,           // Runtime code fetch error
    F_CODE_ALIGNMENT,       // Runtime code alignment error
    F_CPU_SIM,              // Unhandled ARM instruction in sim (validator bug)
    F_RESERVED_SVC,         // Reserved SVC encoding
    F_RESERVED_ADDROP,      // Reserved ADDROP encoding
    F_ABORT,                // User call to _SYS_abort()
    F_LONG_STACK_LOAD,      // Bad address in long stack LDR addrop
    F_LONG_STACK_STORE,     // Bad address in long stack STR addrop
    F_PRELOAD_ADDRESS,      // Bad address for async preload
    F_RETURN_FRAME,         // Bad saved FP value detected during ret()
    F_LOG_FETCH,            // Memory fault while fetching _SYS_log() data
};

/**
 * Debugger messages are command/response pairs which are sent from a
 * host to the SVM runtime. All debugger messages are formatted as a
 * bounded-length packet made up of 32-bit words.
 */

namespace Debugger {
    enum MessageTypes {
        M_TYPE_MASK         = 0xff000000,
        M_ARG_MASK          = 0x00ffffff,

        M_READ_REGISTERS    = 0x01000000,  // arg=bitmap, [] -> [regs]
        M_WRITE_REGISTERS   = 0x02000000,  // arg=bitmap, [regs] -> []
        M_READ_RAM          = 0x03000000,  // arg=address, [byteCount] -> [bytes]
        M_WRITE_RAM         = 0x04000000,  // arg=address, [byteCount, bytes] -> []
        M_SIGNAL            = 0x05000000,  // arg=signal, [] -> []
        M_IS_STOPPED        = 0x06000000,  // [] -> [signal]
        M_DETACH            = 0x07000000,  // [] -> []
        M_SET_BREAKPOINTS   = 0x08000000,  // arg=bitmap, [addresses] -> []
        M_STEP              = 0x09000000,  // [] -> []
    };

    /*
     * Symmetric maximum lengths. Long enough for all 14 registers plus a
     * command word. Leaves room for one header word before we fill up a
     * 64-byte USB packet.
     */
    const uint32_t MAX_CMD_WORDS = 15;
    const uint32_t MAX_REPLY_WORDS = 15;

    const uint32_t MAX_CMD_BYTES = MAX_CMD_WORDS * sizeof(uint32_t);
    const uint32_t MAX_REPLY_BYTES = MAX_REPLY_WORDS * sizeof(uint32_t);

    /*
     * A note on register bitmaps: They are CLZ-style bitmaps, shifted right
     * by BITMAP_SHIFT bits. Registers are numbered in the standard SVM/ARM way.
     */
    const uint32_t BITMAP_SHIFT = 8;

    static inline uint32_t argBit(unsigned r) {
        return (0x80000000 >> BITMAP_SHIFT) >> r;
    }
    
    static const uint32_t ALL_REGISTER_BITS =
        Debugger::argBit(0) | Debugger::argBit(1) |
        Debugger::argBit(2) | Debugger::argBit(3) |
        Debugger::argBit(4) | Debugger::argBit(5) |
        Debugger::argBit(6) | Debugger::argBit(7) |
        Debugger::argBit(8) | Debugger::argBit(9) |
        Debugger::argBit(REG_FP) | Debugger::argBit(REG_SP) |
        Debugger::argBit(REG_PC) | Debugger::argBit(REG_CPSR);

    /*
     * The runtime has storage for up to four hardware breakpoints.
     * Breakpoints are disabled by writing an address of zero.
     */
    const uint32_t NUM_BREAKPOINTS = 8;
    const uint32_t ALL_BREAKPOINT_BITS = 0x00ff0000;

    /*
     * UNIX-style signal names, used to keep track of the reason why the
     * debugger has stopped the process.
     */
    enum Signals {
        S_RUNNING   = 0,
        S_HUP       = 1,
        S_INT       = 2,
        S_QUIT      = 3,
        S_ILL       = 4,
        S_TRAP      = 5,
        S_ABORT     = 6,
        S_FPE       = 8,
        S_KILL      = 9,
        S_BUS       = 10,
        S_SEGV      = 11,
        S_SYS       = 12,
        S_PIPE      = 13,
        S_TERM      = 15,
        S_STOP      = 17,
    };
};


/***************************************************************************
 * Utilities
 ***************************************************************************/

// Extend a W-bit wide two's complement value to 32-bit.
static inline int32_t signExtend(uint32_t value, unsigned w) {
    const uint32_t msb = 1 << (w - 1);
    const uint32_t upper = (uint32_t)-1 << w;
    if (value & msb)
        value |= upper;
    return value;
}

static inline InstructionSize instructionSize(uint16_t instr) {
    // if bits [15:11] are 0b11101, 0b11110 or 0b11111, it's a 32-bit instruction
    // 0xe800 == 0b11101 << 11
    return (instr & 0xf800) >= 0xe800 ? InstrBits32 : InstrBits16;
}

static inline reg_t calculateBranchOffset(reg_t pc, int32_t offset) {
    return (pc + offset + 2) & ~(reg_t)1;
}


/***************************************************************************
 * Flags and Condition Codes
 ***************************************************************************/

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

static inline bool getNeg(reg_t cpsr) {
    return (cpsr >> 31) & 1;
}

static inline bool getZero(reg_t cpsr) {
    return (cpsr >> 30) & 1;
}

static inline bool getCarry(reg_t cpsr) {
    return (cpsr >> 29) & 1;
}

static inline int getOverflow(reg_t cpsr) {
    return (cpsr >> 28) & 1;
}

static inline bool conditionPassed(uint8_t cond, reg_t cpsr)
{
    switch (cond) {
    case EQ: return  getZero(cpsr);
    case NE: return !getZero(cpsr);
    case CS: return  getCarry(cpsr);
    case CC: return !getCarry(cpsr);
    case MI: return  getNeg(cpsr);
    case PL: return !getNeg(cpsr);
    case VS: return  getOverflow(cpsr);
    case VC: return !getOverflow(cpsr);
    case HI: return  getCarry(cpsr) && !getZero(cpsr);
    case LS: return !getCarry(cpsr) || getZero(cpsr);
    case GE: return  getNeg(cpsr) == getOverflow(cpsr);
    case LT: return  getNeg(cpsr) != getOverflow(cpsr);
    case GT: return (getZero(cpsr) == 0) && (getNeg(cpsr) == getOverflow(cpsr));
    case LE: return (getZero(cpsr) == 1) || (getNeg(cpsr) != getOverflow(cpsr));
    case NoneAL: return true;
    default:
        ASSERT(0 && "invalid condition code");
        return false;
    }
}

/***************************************************************************
 * Branch Emulation utilities
 ***************************************************************************/

static inline reg_t branchTargetB(uint16_t instr, reg_t pc)
{
    // encoding T2 only
    unsigned imm11 = instr & 0x7FF;
    return calculateBranchOffset(pc, signExtend(imm11 << 1, 12));
}

static inline reg_t branchTargetCondB(uint16_t instr, reg_t pc, reg_t cpsr)
{
    unsigned cond = (instr >> 8) & 0xf;
    unsigned imm8 = instr & 0xff;

    if (conditionPassed(cond, cpsr))
        return calculateBranchOffset(pc, signExtend(imm8 << 1, 9));
    else
        return pc;
}

static inline reg_t branchTargetCBZ_CBNZ(uint16_t instr, reg_t pc, reg_t cpsr, reg_t Rn)
{
    bool nonzero = instr & (1 << 11);
    unsigned i = instr & (1 << 9);
    unsigned imm5 = (instr >> 3) & 0x1f;

    if (nonzero ^ (Rn == 0)) {
        // ZeroExtend(i:imm5:'0')
        return calculateBranchOffset(pc, (i << 6) | (imm5 << 1));
    }
    return pc;
}


///////////////////////////////////////
// Specific instructions
///////////////////////////////////////

static const uint16_t BreakpointInstr   = 0xdfe8;

///////////////////////////////////////
// 16-bit thumb instruction validators
///////////////////////////////////////
static const uint16_t AluMask           = 0x3 << 14;    // 0b11xxxxxx xxxxxxxx
static const uint16_t AluTest           = 0x0;          // 0b00xxxxxx xxxxxxxx

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

static const uint16_t MovMask           = 0xffc0;       // 0b11111111 11xxxxxx  
static const uint16_t MovTest           = 0x4600;       // 0b01000110 00xxxxxx  

static const uint16_t Nop               = 0xbf00;       // 0b10111111 00000000

///////////////////////////////////////
// 32-bit thumb instruction validators
///////////////////////////////////////
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

static const uint32_t DivMask   = 0xffd8f8f8;       // 0b11111111 11011000, 11111000 11111000
static const uint32_t DivTest   = 0xfb90f0f0;       // 0b11111011 10x10xxx, 11110xxx 11110xxx

////////////////////////////
// indirect operation masks
////////////////////////////
static const uint32_t CallMask = (1 << 31) | 0x3;               // 1xxxxxxx xxxxxxxx xxxxxxxx xxxxxx11
static const uint32_t CallTest = 0x0;                           // 0nnnnnnn aaaaaaaa aaaaaaaa aaaaaa00

static const uint32_t TailCallMask = (1 << 31) | 0x1;           // 1xxxxxxx xxxxxxxx xxxxxxxx xxxxxx01
static const uint32_t TailCallTest = 0x1;                       // 0nnnnnnn aaaaaaaa aaaaaaaa aaaaaa01

static const uint32_t IndirectSyscallMask = (0x3 << 30) | 0x1;  // 11xxxxxx xxxxxxxx xxxxxxxx xxxxxxx1
static const uint32_t IndirectSyscallTest = (0x1 << 31);        // 10nnnnnn nnnnnnnn iiiiiiii iiiiiii0

static const uint32_t TailSyscallMask = (0x3 << 30) | 0x1;      // 11xxxxxx xxxxxxxx xxxxxxxx xxxxxxx1
static const uint32_t TailSyscallTest = (0x1 << 31) | 0x1;      // 10nnnnnn nnnnnnnn iiiiiiii iiiiiii1

static const uint32_t AddropMask = 0x7 << 29;                   // 111xxxxx xxxxxxxx xxxxxxxx xxxxxxxx
static const uint32_t AddropTest = 0x3 << 30;                   // 110nnnnn aaaaaaaa aaaaaaaa aaaaaaaa

static const uint32_t AddropFlashMask = 0x7 << 29;              // 111xxxxx xxxxxxxx xxxxxxxx xxxxxxxx
static const uint32_t AddropFlashTest = 0x7 << 29;              // 111nnnnn aaaaaaaa aaaaaaaa aaaaaaaa

}   // namespace Svm

#endif // SVM_H
