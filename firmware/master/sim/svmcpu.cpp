/*
 * Thundercracker Firmware -- Confidential, not for redistribution.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#include "svmcpu.h"
#include "svmruntime.h"
#include "svmdebug.h"

#include <sifteo/macros.h>
#include <sifteo/machine.h>

#include <string.h>
#include <inttypes.h>

#ifndef PRIxPTR // for mingw
#define PRIxPTR "x"
#endif

namespace SvmCpu {

static reg_t regs[NUM_REGS];

// registers that get saved to the stack automatically by hardware
struct HwContext {
    reg_t r0;
    reg_t r1;
    reg_t r2;
    reg_t r3;
    reg_t r12;
    reg_t lr;
    reg_t returnAddr;
    reg_t xpsr;
};

// registers that we want to store on the stack during exception handling
// which do not get saved by hardware
// TODO: may be able to get away without stacking all these
struct IrqContext {
    reg_t r4;
    reg_t r5;
    reg_t r6;
    reg_t r7;
    reg_t r8;
    reg_t r9;
    reg_t r10;
    reg_t r11;
};

struct StackedRegs {
    IrqContext irq;
    HwContext hw;
};

/***************************************************************************
 * Utilities
 ***************************************************************************/

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

static inline bool getNeg() {
    return (regs[REG_CPSR] >> 31) & 1;
}
static inline void setNeg(bool f) {
    if (f)
        regs[REG_CPSR] |= (1 << 31);
    else
        regs[REG_CPSR] &= ~(1 << 31);
}

static inline bool getZero() {
    return (regs[REG_CPSR] >> 30) & 1;
}
static inline void setZero(bool f) {
    if (f)
        regs[REG_CPSR] |= 1 << 30;
    else
        regs[REG_CPSR] &= ~(1 << 30);
}

static inline bool getCarry() {
    return (regs[REG_CPSR] >> 29) & 1;
}
static inline void setCarry(bool f) {
    if (f)
        regs[REG_CPSR] |= 1 << 29;
    else
        regs[REG_CPSR] &= ~(1 << 29);
}

static inline int getOverflow() {
    return (regs[REG_CPSR] >> 28) & 1;
}
static inline void setOverflow(bool f) {
    if (f)
        regs[REG_CPSR] |= (1 << 28);
    else
        regs[REG_CPSR] &= ~(1 << 28);
}

static inline void setNZ(int32_t result) {
    setNeg(result < 0);
    setZero(result == 0);
}

static inline void branchOffsetPC(int offset) {
    regs[REG_PC] = (regs[REG_PC] + offset + 2) & ~(reg_t)1;
}

static inline reg_t opLSL(reg_t a, reg_t b) {
    setCarry(b ? ((0x80000000 >> (b - 1)) & a) != 0 : 0);
    reg_t result = b < 32 ? a << b : 0;
    setNZ(result);
    return result;
}

static inline reg_t opLSR(reg_t a, reg_t b) {
    setCarry(b ? ((1 << (b - 1)) & a) != 0 : 0);
    reg_t result = b < 32 ? a >> b : 0;
    setNZ(result);
    return result;
}

static inline reg_t opASR(reg_t a, reg_t b) {
    setCarry(b ? ((1 << (b - 1)) & a) != 0 : 0);
    reg_t result = b < 32 ? (int32_t)a >> b : 0;
    setNZ(result);
    return result;
}

static inline reg_t opADD(reg_t a, reg_t b, reg_t carry) {
    // Based on AddWithCarry() in the ARMv7 ARM, page A2-8
    uint64_t uSum32 = (uint64_t)(uint32_t)a + (uint32_t)b + (uint32_t)carry;
    int64_t sSum32 = (int64_t)(int32_t)a + (int32_t)b + (uint32_t)carry;
    setNZ(sSum32);
    setOverflow((int32_t)sSum32 != sSum32);
    setCarry((uint32_t)uSum32 != uSum32);

    // Preserve full reg_t width in result, even though we use 32-bit value for flags
    return a + b + carry;
}

static inline reg_t opAND(reg_t a, reg_t b) {
    reg_t result = a & b;
    setNZ(result);
    return result;
}

static inline reg_t opEOR(reg_t a, reg_t b) {
    reg_t result = a ^ b;
    setNZ(result);
    return result;
}

static bool conditionPassed(uint8_t cond)
{
    switch (cond) {
    case EQ: return  getZero();
    case NE: return !getZero();
    case CS: return  getCarry();
    case CC: return !getCarry();
    case MI: return  getNeg();
    case PL: return !getNeg();
    case VS: return  getOverflow();
    case VC: return !getOverflow();
    case HI: return  getCarry() && !getZero();
    case LS: return !getCarry() || getZero();
    case GE: return  getNeg() == getOverflow();
    case LT: return  getNeg() != getOverflow();
    case GT: return (getZero() == 0) && (getNeg() == getOverflow());
    case LE: return (getZero() == 1) || (getNeg() != getOverflow());
    case NoneAL: return true;
    default:
        ASSERT(0 && "invalid condition code");
        return false;
    }
}


/***************************************************************************
 * Instruction Emulation
 ***************************************************************************/

// left shift
static void emulateLSLImm(uint16_t inst)
{
    unsigned imm5 = (inst >> 6) & 0x1f;
    unsigned Rm = (inst >> 3) & 0x7;
    unsigned Rd = inst & 0x7;

    regs[Rd] = opLSL(regs[Rm], imm5);
}

static void emulateLSRImm(uint16_t inst)
{
    unsigned imm5 = (inst >> 6) & 0x1f;
    unsigned Rm = (inst >> 3) & 0x7;
    unsigned Rd = inst & 0x7;

    if (imm5 == 0)
        imm5 = 32;

    regs[Rd] = opLSR(regs[Rm], imm5);
}

static void emulateASRImm(uint16_t instr)
{
    unsigned imm5 = (instr >> 6) & 0x1f;
    unsigned Rm = (instr >> 3) & 0x7;
    unsigned Rd = instr & 0x7;

    if (imm5 == 0)
        imm5 = 32;

    regs[Rd] = opASR(regs[Rm], imm5);
}

static void emulateADDReg(uint16_t instr)
{
    unsigned Rm = (instr >> 6) & 0x7;
    unsigned Rn = (instr >> 3) & 0x7;
    unsigned Rd = instr & 0x7;

    regs[Rd] = opADD(regs[Rn], regs[Rm], 0);
}

static void emulateSUBReg(uint16_t instr)
{
    unsigned Rm = (instr >> 6) & 0x7;
    unsigned Rn = (instr >> 3) & 0x7;
    unsigned Rd = instr & 0x7;

    regs[Rd] = opADD(regs[Rn], ~regs[Rm], 1);
}

static void emulateADD3Imm(uint16_t instr)
{
    reg_t imm3 = (instr >> 6) & 0x7;
    unsigned Rn = (instr >> 3) & 0x7;
    unsigned Rd = instr & 0x7;

    regs[Rd] = opADD(regs[Rn], imm3, 0);
}

static void emulateSUB3Imm(uint16_t instr)
{
    reg_t imm3 = (instr >> 6) & 0x7;
    unsigned Rn = (instr >> 3) & 0x7;
    unsigned Rd = instr & 0x7;

    regs[Rd] = opADD(regs[Rn], ~imm3, 1);
}

static void emulateMovImm(uint16_t instr)
{
    unsigned Rd = (instr >> 8) & 0x7;
    unsigned imm8 = instr & 0xff;

    regs[Rd] = imm8;
}

static void emulateCmpImm(uint16_t instr)
{
    unsigned Rn = (instr >> 8) & 0x7;
    reg_t imm8 = instr & 0xff;

    reg_t result = opADD(regs[Rn], ~imm8, 1);
}

static void emulateADD8Imm(uint16_t instr)
{
    unsigned Rdn = (instr >> 8) & 0x7;
    reg_t imm8 = instr & 0xff;

    regs[Rdn] = opADD(regs[Rdn], imm8, 0);
}

static void emulateSUB8Imm(uint16_t instr)
{
    unsigned Rdn = (instr >> 8) & 0x7;
    reg_t imm8 = instr & 0xff;

    regs[Rdn] = opADD(regs[Rdn], ~imm8, 1);
}

///////////////////////////////////
// D A T A   P R O C E S S I N G
///////////////////////////////////

static void emulateANDReg(uint16_t instr)
{
    unsigned Rm = (instr >> 3) & 0x7;
    unsigned Rdn = instr & 0x7;

    regs[Rdn] = opAND(regs[Rdn], regs[Rm]);
}

static void emulateEORReg(uint16_t instr)
{
    unsigned Rm = (instr >> 3) & 0x7;
    unsigned Rdn = instr & 0x7;

    regs[Rdn] = opEOR(regs[Rdn], regs[Rm]);
}

static void emulateLSLReg(uint16_t instr)
{
    unsigned Rm = (instr >> 3) & 0x7;
    unsigned Rdn = instr & 0x7;

    unsigned shift = regs[Rm] & 0xff;
    regs[Rdn] = opLSL(regs[Rdn], shift);
}

static void emulateLSRReg(uint16_t instr)
{
    unsigned Rm = (instr >> 3) & 0x7;
    unsigned Rdn = instr & 0x7;

    unsigned shift = regs[Rm] & 0xff;
    regs[Rdn] = opLSR(regs[Rdn], shift);
}

static void emulateASRReg(uint16_t instr)
{
    unsigned Rm = (instr >> 3) & 0x7;
    unsigned Rdn = instr & 0x7;

    unsigned shift = regs[Rm] & 0xff;
    regs[Rdn] = opASR(regs[Rdn], shift);
}

static void emulateADCReg(uint16_t instr)
{
    unsigned Rm = (instr >> 3) & 0x7;
    unsigned Rdn = instr & 0x7;

    regs[Rdn] = opADD(regs[Rdn], regs[Rm], getCarry());
}

static void emulateSBCReg(uint16_t instr)
{
    unsigned Rm = (instr >> 3) & 0x7;
    unsigned Rdn = instr & 0x7;

    regs[Rdn] = opADD(regs[Rdn], ~regs[Rm], getCarry());
}

static void emulateRORReg(uint16_t instr)
{
    unsigned Rm = (instr >> 3) & 0x7;
    unsigned Rdn = instr & 0x7;

    regs[Rdn] = Sifteo::Intrinsic::ROR(regs[Rdn], regs[Rm]);
}

static void emulateTSTReg(uint16_t instr)
{
    unsigned Rm = (instr >> 3) & 0x7;
    unsigned Rdn = instr & 0x7;

    opAND(regs[Rdn], regs[Rm]);
}

static void emulateRSBImm(uint16_t instr)
{
    unsigned Rn = (instr >> 3) & 0x7;
    unsigned Rd = instr & 0x7;

    regs[Rd] = opADD(~regs[Rn], 0, 1);
}

static void emulateCMPReg(uint16_t instr)
{
    unsigned Rm = (instr >> 3) & 0x7;
    unsigned Rdn = instr & 0x7;
    
    opADD(regs[Rdn], ~regs[Rm], 1);
}

static void emulateCMNReg(uint16_t instr)
{
    unsigned Rm = (instr >> 3) & 0x7;
    unsigned Rdn = instr & 0x7;

    opADD(regs[Rdn], regs[Rm], 0);
}

static void emulateORRReg(uint16_t instr)
{
    unsigned Rm = (instr >> 3) & 0x7;
    unsigned Rdn = instr & 0x7;

    reg_t result = regs[Rdn] | regs[Rm];
    regs[Rdn] = result;
    setNZ(result);
}

static void emulateMUL(uint16_t instr)
{
    unsigned Rm = (instr >> 3) & 0x7;
    unsigned Rdn = instr & 0x7;

    uint64_t result = (uint64_t)regs[Rdn] * (uint64_t)regs[Rm];
    regs[Rdn] = (uint32_t) result;

    // Flag calculations always use the full 64-bit result
    setNeg((int64_t)result < 0);
    setZero(result == 0);
}

static void emulateBICReg(uint16_t instr)
{
    unsigned Rm = (instr >> 3) & 0x7;
    unsigned Rdn = instr & 0x7;

    regs[Rdn] = (uint32_t) (regs[Rdn] & ~(regs[Rm]));
}

static void emulateMVNReg(uint16_t instr)
{
    unsigned Rm = (instr >> 3) & 0x7;
    unsigned Rdn = instr & 0x7;

    regs[Rdn] = (uint32_t) ~regs[Rm];
}

/////////////////////////////////////
// M I S C   I N S T R U C T I O N S
/////////////////////////////////////

static void emulateSXTH(uint16_t instr)
{
    unsigned Rm = (instr >> 3) & 0x7;
    unsigned Rdn = instr & 0x7;

    regs[Rdn] = (uint32_t) SignExtend<signed int, 16>(regs[Rm]);
}

static void emulateSXTB(uint16_t instr)
{
    unsigned Rm = (instr >> 3) & 0x7;
    unsigned Rdn = instr & 0x7;

    regs[Rdn] = (uint32_t) SignExtend<signed int, 8>(regs[Rm]);
}

static void emulateUXTH(uint16_t instr)
{
    unsigned Rm = (instr >> 3) & 0x7;
    unsigned Rdn = instr & 0x7;

    regs[Rdn] = regs[Rm] & 0xFFFF;
}

static void emulateUXTB(uint16_t instr)
{
    unsigned Rm = (instr >> 3) & 0x7;
    unsigned Rdn = instr & 0x7;

    regs[Rdn] = regs[Rm] & 0xFF;
}

static void emulateMOV(uint16_t instr)
{
    // Thumb T5 encoding, does not affect flags.
    // This subset does not support high register access.

    unsigned Rs = (instr >> 3) & 0x7;
    unsigned Rd = instr & 0x7;

    regs[Rd] = regs[Rs];
}


///////////////////////////////////////////////
// B R A N C H I N G   I N S T R U C T I O N S
///////////////////////////////////////////////

static void emulateB(uint16_t instr)
{
    // encoding T2 only
    unsigned imm11 = instr & 0x7FF;
    branchOffsetPC(SignExtend<signed int, 12>(imm11 << 1));
}

static void emulateCondB(uint16_t instr)
{
    unsigned cond = (instr >> 8) & 0xf;
    unsigned imm8 = instr & 0xff;

    if (conditionPassed(cond)) {
        branchOffsetPC(SignExtend<signed int, 9>(imm8 << 1));
    }
}

static void emulateCBZ_CBNZ(uint16_t instr)
{
    bool nonzero = instr & (1 << 11);
    unsigned i = instr & (1 << 9);
    unsigned imm5 = (instr >> 3) & 0x1f;
    unsigned Rn = instr & 0x7;

    if (nonzero ^ (regs[Rn] == 0)) {
        // ZeroExtend(i:imm5:'0')
        branchOffsetPC((i << 6) | (imm5 << 1));
    }
}


/////////////////////////////////////////
// M E M O R Y  I N S T R U C T I O N S
/////////////////////////////////////////

static void emulateSTRSPImm(uint16_t instr)
{
    // encoding T2 only
    unsigned Rt = (instr >> 8) & 0x7;
    unsigned imm8 = instr & 0xff;
    reg_t addr = regs[REG_SP] + (imm8 << 2);

    if (!SvmMemory::isAddrValid(addr))
        SvmDebug::fault(F_STORE_ADDRESS);
    if (!SvmMemory::isAddrAligned(addr, 4))
        SvmDebug::fault(F_STORE_ALIGNMENT);

    SvmMemory::squashPhysicalAddr(regs[Rt]);
    *reinterpret_cast<uint32_t*>(addr) = regs[Rt];
}

static void emulateLDRSPImm(uint16_t instr)
{
    // encoding T2 only
    unsigned Rt = (instr >> 8) & 0x7;
    unsigned imm8 = instr & 0xff;
    reg_t addr = regs[REG_SP] + (imm8 << 2);

    if (!SvmMemory::isAddrValid(addr))
        SvmDebug::fault(F_LOAD_ADDRESS);
    if (!SvmMemory::isAddrAligned(addr, 4))
        SvmDebug::fault(F_LOAD_ALIGNMENT);

    regs[Rt] = *reinterpret_cast<uint32_t*>(addr);
}

static void emulateADDSpImm(uint16_t instr)
{
    // encoding T1 only
    unsigned Rd = (instr >> 8) & 0x7;
    unsigned imm8 = instr & 0xff;

    regs[Rd] = regs[REG_SP] + (imm8 << 2);
}

static void emulateLDRLitPool(uint16_t instr)
{
    unsigned Rt = (instr >> 8) & 0x7;
    unsigned imm8 = instr & 0xFF;

    // Round up to the next 32-bit boundary
    reg_t addr = ((regs[REG_PC] + 3) & ~3) + (imm8 << 2);

    if (!SvmMemory::isAddrValid(addr))
        SvmDebug::fault(F_LOAD_ADDRESS);
    if (!SvmMemory::isAddrAligned(addr, 4))
        SvmDebug::fault(F_LOAD_ALIGNMENT);

    regs[Rt] = *reinterpret_cast<uint32_t*>(addr);
}


////////////////////////////////////////
// 3 2 - B I T  I N S T R U C T I O N S
////////////////////////////////////////

static void emulateSTR(uint32_t instr)
{
    unsigned imm12 = instr & 0xFFF;
    unsigned Rn = (instr >> 16) & 0xF;
    unsigned Rt = (instr >> 12) & 0xF;
    reg_t addr = regs[Rn] + imm12;

    if (!SvmMemory::isAddrValid(addr))
        SvmDebug::fault(F_STORE_ADDRESS);
    if (!SvmMemory::isAddrAligned(addr, 4))
        SvmDebug::fault(F_STORE_ALIGNMENT);

    SvmMemory::squashPhysicalAddr(regs[Rt]);
    *reinterpret_cast<uint32_t*>(addr) = regs[Rt];
}

static void emulateLDR(uint32_t instr)
{
    unsigned imm12 = instr & 0xFFF;
    unsigned Rn = (instr >> 16) & 0xF;
    unsigned Rt = (instr >> 12) & 0xF;
    reg_t addr = regs[Rn] + imm12;

    if (!SvmMemory::isAddrValid(addr))
        SvmDebug::fault(F_LOAD_ADDRESS);
    if (!SvmMemory::isAddrAligned(addr, 4))
        SvmDebug::fault(F_LOAD_ALIGNMENT);

    regs[Rt] = *reinterpret_cast<uint32_t*>(addr);
}

static void emulateSTRBH(uint32_t instr)
{
    const unsigned HalfwordBit = 1 << 21;

    unsigned imm12 = instr & 0xFFF;
    unsigned Rn = (instr >> 16) & 0xF;
    unsigned Rt = (instr >> 12) & 0xF;
    reg_t addr = regs[Rn] + imm12;

    if (!SvmMemory::isAddrValid(addr))
        SvmDebug::fault(F_STORE_ADDRESS);

    if (instr & HalfwordBit) {
        if (!SvmMemory::isAddrAligned(addr, 2))
            SvmDebug::fault(F_STORE_ALIGNMENT);
        *reinterpret_cast<uint16_t*>(addr) = regs[Rt];
    } else {
        *reinterpret_cast<uint8_t*>(addr) = regs[Rt];
    }
}

static void emulateLDRBH(uint32_t instr)
{
    const unsigned HalfwordBit = 1 << 21;
    const unsigned SignExtBit = 1 << 24;

    unsigned imm12 = instr & 0xFFF;
    unsigned Rn = (instr >> 16) & 0xF;
    unsigned Rt = (instr >> 12) & 0xF;
    reg_t addr = regs[Rn] + imm12;

    if (!SvmMemory::isAddrValid(addr))
        SvmDebug::fault(F_LOAD_ADDRESS);

    switch (instr & (HalfwordBit | SignExtBit)) {
    case 0:
        regs[Rt] = *reinterpret_cast<uint8_t*>(addr);
        break;
    case HalfwordBit:
        if (!SvmMemory::isAddrAligned(addr, 2))
            SvmDebug::fault(F_LOAD_ALIGNMENT);
        regs[Rt] = *reinterpret_cast<uint16_t*>(addr);
        break;
    case SignExtBit:
        regs[Rt] = (uint32_t) SignExtend<signed int, 8>
            (*reinterpret_cast<uint8_t*>(addr));
        break;
    case HalfwordBit | SignExtBit:
        if (!SvmMemory::isAddrAligned(addr, 2))
            SvmDebug::fault(F_LOAD_ALIGNMENT);
        regs[Rt] = (uint32_t) SignExtend<signed int, 16>
            (*reinterpret_cast<uint16_t*>(addr));
        break;
    }
}

static void emulateMOVWT(uint32_t instr)
{
    const unsigned TopBit = 1 << 23;

    unsigned Rd = (instr >> 8) & 0xF;
    unsigned imm16 =
        (instr & 0x000000FF) |
        (instr & 0x00007000) >> 4 |
        (instr & 0x04000000) >> 15 |
        (instr & 0x000F0000) >> 4;

    if (TopBit & instr) {
        regs[Rd] = (regs[Rd] & 0xFFFF) | (imm16 << 16);
    } else {
        regs[Rd] = imm16;
    }
}

static void emulateDIV(uint32_t instr)
{
    const unsigned UnsignedBit = 1 << 21;

    unsigned Rn = (instr >> 16) & 0xF;
    unsigned Rd = (instr >> 8) & 0xF;
    unsigned Rm = instr & 0xF;

    uint32_t m32 = (uint32_t) regs[Rm];

    if (m32 == 0) {
        // Divide by zero, defined to return 0
        regs[Rd] = 0;
    } else if (UnsignedBit & instr) {
        regs[Rd] = (uint32_t)regs[Rn] / m32;
    } else {
        regs[Rd] = (int32_t)regs[Rn] / (int32_t)m32;
    }
}

/*
 * Only vaguely faithful exception emulation :)
 * Assume we're always in User mode & accessing the User stack pointer.
 *
 * cortex-m3 pushes a HwContext to the appropriate stack, User or Main,
 * to allow C-language interrupt handlers to be AAPCS compliant.
 */
static void enterException(reg_t returnAddr)
{
    // TODO: verify that we're not stacking to invalid memory
    regs[REG_SP] -= sizeof(HwContext);
    HwContext *ctx = reinterpret_cast<HwContext*>(regs[REG_SP]);
    ctx->r0         = regs[0];
    ctx->r1         = regs[1];
    ctx->r2         = regs[2];
    ctx->r3         = regs[3];
    ctx->r12        = regs[12];
    ctx->lr         = regs[REG_LR];
    ctx->returnAddr = returnAddr;
    ctx->xpsr       = regs[REG_CPSR];   // XXX; must also or in frameptralign

    ASSERT((ctx->returnAddr & 1) == 0 && "ReturnAddress from exception must be halfword aligned");

    regs[REG_LR] = 0xfffffffd;  // indicate that we're coming from User mode, using User stack
}

/*
 * Pop HW context
 */
static void exitException()
{
    HwContext *ctx = reinterpret_cast<HwContext*>(regs[REG_SP]);
    regs[0]         = ctx->r0;
    regs[1]         = ctx->r1;
    regs[2]         = ctx->r2;
    regs[3]         = ctx->r3;
    regs[12]        = ctx->r12;
    regs[REG_LR]    = ctx->lr;
    regs[REG_CPSR]  = ctx->xpsr;

    regs[REG_SP] += sizeof(HwContext);

    regs[REG_PC]    = ctx->returnAddr;
}

static void emulateSVC(uint16_t instr)
{
    uint32_t nextInstruction = regs[REG_PC];    // already incremented in fetch()
    enterException(nextInstruction);

    uint8_t imm8 = instr & 0xff;
    SvmRuntime::svc(imm8);

    exitException();
}


/***************************************************************************
 * Instruction Dispatch
 ***************************************************************************/

static uint16_t fetch()
{
    /*
     * Fetch the next instruction.
     * We can return 16bit values unconditionally, since all our instructions are Thumb,
     * and the first nibble of a 32-bit instruction must be checked regardless
     * in order to determine its bitness.
     */

    if (!SvmMemory::isAddrValid(regs[REG_PC]))
        SvmDebug::fault(F_CODE_FETCH);
    if (!SvmMemory::isAddrAligned(regs[REG_PC], 2))
        SvmDebug::fault(F_LOAD_ALIGNMENT);

    uint16_t *pc = reinterpret_cast<uint16_t*>(regs[REG_PC]);

#ifdef SVM_TRACE
    LOG(("[%08x: %04x]", SvmRuntime::reconstructCodeAddr(), *pc));
    for (unsigned r = 0; r < 8; r++) {
        LOG((" r%d=%016"PRIxPTR"", r, regs[r]));
    }
    LOG((" (%c%c%c%c) | r8=%"PRIxPTR" r9=%"PRIxPTR" sp=%"PRIxPTR"\n",
        getNeg() ? 'N' : ' ',
        getZero() ? 'Z' : ' ',
        getCarry() ? 'C' : ' ',
        getOverflow() ? 'V' : ' ',
        regs[8], regs[9], regs[REG_SP]));
#endif

    regs[REG_PC] += sizeof(uint16_t);
    return *pc;
}

static void execute16(uint16_t instr)
{
    if ((instr & AluMask) == AluTest) {
        // lsl, lsr, asr, add, sub, mov, cmp
        // take bits [13:11] to group these
        uint8_t prefix = (instr >> 11) & 0x7;
        switch (prefix) {
        case 0: // 0b000 - LSL
            emulateLSLImm(instr);
            return;
        case 1: // 0b001 - LSR
            emulateLSRImm(instr);
            return;
        case 2: // 0b010 - ASR
            emulateASRImm(instr);
            return;
        case 3: { // 0b011 - ADD/SUB reg/imm
            uint8_t subop = (instr >> 9) & 0x3;
            switch (subop) {
            case 0:
                emulateADDReg(instr);
                return;
            case 1:
                emulateSUBReg(instr);
                return;
            case 2:
                emulateADD3Imm(instr);
                return;
            case 3:
                emulateADD8Imm(instr);
                return;
            }
        }
        case 4: // 0b100 - MOV
            emulateMovImm(instr);
            return;
        case 5: // 0b101
            emulateCmpImm(instr);
            return;
        case 6: // 0b110 - ADD 8bit
            emulateADD8Imm(instr);
            return;
        case 7: // 0b111 - SUB 8bit
            emulateSUB8Imm(instr);
            return;
        }
        ASSERT(0 && "unhandled ALU instruction!");
    }
    if ((instr & DataProcMask) == DataProcTest) {
        uint8_t opcode = (instr >> 6) & 0xf;
        switch (opcode) {
        case 0:  emulateANDReg(instr); return;
        case 1:  emulateEORReg(instr); return;
        case 2:  emulateLSLReg(instr); return;
        case 3:  emulateLSRReg(instr); return;
        case 4:  emulateASRReg(instr); return;
        case 5:  emulateADCReg(instr); return;
        case 6:  emulateSBCReg(instr); return;
        case 7:  emulateRORReg(instr); return;
        case 8:  emulateTSTReg(instr); return;
        case 9:  emulateRSBImm(instr); return;
        case 10: emulateCMPReg(instr); return;
        case 11: emulateCMNReg(instr); return;
        case 12: emulateORRReg(instr); return;
        case 13: emulateMUL(instr);    return;
        case 14: emulateBICReg(instr); return;
        case 15: emulateMVNReg(instr); return;
        }
    }
    if ((instr & MiscMask) == MiscTest) {
        uint8_t opcode = (instr >> 5) & 0x7f;
        if ((opcode & 0x78) == 0x2) {   // bits [6:3] of opcode identify this group
            switch (opcode & 0x6) {     // bits [2:1] of the opcode identify the instr
            case 0: emulateSXTH(instr); return;
            case 1: emulateSXTB(instr); return;
            case 2: emulateUXTH(instr); return;
            case 3: emulateUXTB(instr); return;
            }
        }
    }
    if ((instr & MovMask) == MovTest) {
        emulateMOV(instr);
        return;
    }    
    if ((instr & SvcMask) == SvcTest) {
        emulateSVC(instr);
        return;
    }
    if ((instr & PcRelLdrMask) == PcRelLdrTest) {
        emulateLDRLitPool(instr);
        return;
    }
    if ((instr & SpRelLdrStrMask) == SpRelLdrStrTest) {
        uint16_t isLoad = instr & (1 << 11);
        if (isLoad)
            emulateLDRSPImm(instr);
        else
            emulateSTRSPImm(instr);
        return;
    }
    if ((instr & SpRelAddMask) == SpRelAddTest) {
        emulateADDSpImm(instr);
        return;
    }
    if ((instr & UncondBranchMask) == UncondBranchTest) {
        emulateB(instr);
        return;
    }
    if ((instr & CompareBranchMask) == CompareBranchTest) {
        emulateCBZ_CBNZ(instr);
        return;
    }
    if ((instr & CondBranchMask) == CondBranchTest) {
        emulateCondB(instr);
        return;
    }
    if (instr == Nop) {
        // nothing to do
        return;
    }

    // should never get here since we should only be executing validated instructions
    LOG(("SVMCPU: invalid 16bit instruction: 0x%x\n", instr));
    SvmDebug::fault(F_CPU_SIM);
}

static void execute32(uint32_t instr)
{
    if ((instr & StrMask) == StrTest) {
        emulateSTR(instr);
        return;
    }
    if ((instr & StrBhMask) == StrBhTest) {
        emulateSTRBH(instr);
        return;
    }
    if ((instr & LdrBhMask) == LdrBhTest) {
        emulateLDRBH(instr);
        return;
    }
    if ((instr & LdrMask) == LdrTest) {
        emulateLDR(instr);
        return;
    }
    if ((instr & MovWtMask) == MovWtTest) {
        emulateMOVWT(instr);
        return;
    }
    if ((instr & DivMask) == DivTest) {
        emulateDIV(instr);
        return;
    }

    // should never get here since we should only be executing validated instructions
    LOG(("SVMCPU: invalid 32bit instruction: 0x%x\n", instr));
    SvmDebug::fault(F_CPU_SIM);
}


/***************************************************************************
 * Public Functions
 ***************************************************************************/

void init()
{
    memset(regs, 0, sizeof(regs));
}

void run(reg_t sp, reg_t pc)
{
    regs[REG_SP] = sp;
    regs[REG_PC] = pc;

    for (;;) {
        uint16_t instr = fetch();
        if (instructionSize(instr) == InstrBits16) {
            execute16(instr);
        }
        else {
            uint16_t instrLow = fetch();
            execute32(instr << 16 | instrLow);
        }
    }
}

/*
 * During SVC handling, the runtime wants to operate on user space's registers,
 * which have been pushed to the stack, which we provide access to here.
 *
 * Register accessors really want to be inline, but putting that off for now, as
 * it will require a bit more code re-org to access platform specific members
 * in the common header file, etc.
 */

reg_t reg(uint8_t r)
{
    StackedRegs *sr = reinterpret_cast<StackedRegs*>(regs[REG_SP]);

    switch (r) {
    case 0:         return sr->hw.r0;
    case 1:         return sr->hw.r1;
    case 2:         return sr->hw.r2;
    case 3:         return sr->hw.r3;
    case 4:         return sr->irq.r4;
    case 5:         return sr->irq.r5;
    case 6:         return sr->irq.r6;
    case 7:         return sr->irq.r7;
    case 8:         return sr->irq.r8;
    case 9:         return sr->irq.r9;
    case 10:        return sr->irq.r10;
    case 11:        return sr->irq.r11;
    case 12:        return sr->hw.r12;
    // SP is a special case - must account for the size of our stacked regs
    case REG_SP:    return regs[REG_SP] + sizeof(StackedRegs);
    case REG_PC:    return sr->hw.returnAddr;
    default:        ASSERT(0 && "invalid register"); break;
    }
}

void setReg(uint8_t r, reg_t val)
{
    StackedRegs *sr = reinterpret_cast<StackedRegs*>(regs[REG_SP]);

    switch (r) {
    case 0:         sr->hw.r0 = val; break;
    case 1:         sr->hw.r1 = val; break;
    case 2:         sr->hw.r2 = val; break;
    case 3:         sr->hw.r3 = val; break;
    case 4:         sr->irq.r4 = val; break;
    case 5:         sr->irq.r5 = val; break;
    case 6:         sr->irq.r6 = val; break;
    case 7:         sr->irq.r7 = val; break;
    case 8:         sr->irq.r8 = val; break;
    case 9:         sr->irq.r9 = val; break;
    case 10:        sr->irq.r10 = val; break;
    case 11:        sr->irq.r11 = val; break;
    case 12:        sr->hw.r12 = val; break;
    case REG_PC:    sr->hw.returnAddr = val; break;
    case REG_SP: {
        /*
         * SP is a special case. We need to move our stacked registers to
         * the newly requested location and update the user SP such that when
         * HW unstacking occurs, it finds the stacked values correctly.
         */
        val -= sizeof(StackedRegs);
        StackedRegs *dest = reinterpret_cast<StackedRegs*>(val);
        memmove(dest, sr, sizeof *sr);
        regs[REG_SP] = val;
        break;
    }
    default:        ASSERT(0 && "invalid register"); break;
    }
}

/*
 * These will be single instruction ldm/stm on ARM.
 */

void pushIrqContext()
{
    regs[REG_SP] -= sizeof(IrqContext);
    IrqContext *ctx = reinterpret_cast<IrqContext*>(regs[REG_SP]);

    ctx->r4 = regs[4];
    ctx->r5 = regs[5];
    ctx->r6 = regs[6];
    ctx->r7 = regs[7];
    ctx->r8 = regs[8];
    ctx->r9 = regs[9];
    ctx->r10 = regs[10];
    ctx->r11 = regs[11];

    SvmMemory::squashPhysicalAddr(ctx->r4);
    SvmMemory::squashPhysicalAddr(ctx->r5);
    SvmMemory::squashPhysicalAddr(ctx->r6);
    SvmMemory::squashPhysicalAddr(ctx->r7);
    SvmMemory::squashPhysicalAddr(ctx->r8);
    SvmMemory::squashPhysicalAddr(ctx->r9);
    SvmMemory::squashPhysicalAddr(ctx->r10);
    SvmMemory::squashPhysicalAddr(ctx->r11);
}

void popIrqContext()
{
    IrqContext *ctx = reinterpret_cast<IrqContext*>(regs[REG_SP]);

    regs[4] = ctx->r4;
    regs[5] = ctx->r5;
    regs[6] = ctx->r6;
    regs[7] = ctx->r7;
    regs[8] = ctx->r8;
    regs[9] = ctx->r9;
    regs[10] = ctx->r10;
    regs[11] = ctx->r11;

    regs[REG_SP] += sizeof(IrqContext);
}

}  // namespace SvmCpu
