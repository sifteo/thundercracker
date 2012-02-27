
#include "svmcpu.h"
#include "svmruntime.h"
#include "sifteo/macros.h"

#include <string.h>
#include <inttypes.h>

SvmCpu::SvmCpu() :
    runtime(SvmRuntime::instance)
{
}

void SvmCpu::init()
{
    memset(regs, 0, sizeof(regs));
    cpsr = 0;

    // init sp to top of user data
    regs[REG_SP] = reinterpret_cast<reg_t>(&mem[MEM_IN_BYTES - 4]);
}

void SvmCpu::run()
{
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

reg_t SvmCpu::reg(uint8_t r) const
{
    return regs[r];
}

void SvmCpu::setReg(uint8_t r, reg_t val)
{
    regs[r] = val;
}

reg_t SvmCpu::userRam() const
{
    return reinterpret_cast<reg_t>(&mem);
}

/*
    Fetch the next instruction.
    We can return 16bit values unconditionally, since all our instructions are Thumb,
    and the first nibble of a 32-bit instruction must be checked regardless
    in order to determine its bitness.
*/
uint16_t SvmCpu::fetch()
{
    uint16_t *tst = reinterpret_cast<uint16_t*>(regs[REG_PC]);

#if 1
    LOG(("[%"PRIxPTR": %x]", runtime.cache2virtFlash(reinterpret_cast<reg_t>(tst)), *tst));
    for (unsigned r = 0; r < 8; r++) {
        assert((uint32_t)regs[r] == regs[r]);
        LOG((" r%d=%08x", r, (uint32_t) regs[r]));
    }
    LOG((" | r8=%"PRIxPTR" r9=%"PRIxPTR" sp=%"PRIxPTR"\n", regs[8], regs[9], regs[REG_SP]));
#endif

    regs[REG_PC] += sizeof(uint16_t);
    return *tst;
}

void SvmCpu::execute16(uint16_t instr)
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
    LOG(("*********************************** invalid 16bit instruction: 0x%x\n", instr));
    ASSERT(0 && "unhandled instruction group!");
}

void SvmCpu::execute32(uint32_t instr)
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
    LOG(("*********************************** invalid 32bit instruction: 0x%x\n", instr));
    ASSERT(0 && "unhandled instruction group!");
}

bool SvmCpu::conditionPassed(uint8_t cond)
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

// left shift
void SvmCpu::emulateLSLImm(uint16_t inst)
{
    unsigned imm5 = (inst >> 6) & 0x1f;
    unsigned Rm = (inst >> 3) & 0x7;
    unsigned Rd = inst & 0x7;

    // no immediate? default to mov
    if (imm5 == 0) {
        regs[Rd] = regs[Rm];
    }
    else {
        regs[Rd] = (uint32_t) (regs[Rm] << imm5);
        // TODO: carry flags
    }
    // TODO: N, Z flags - V unaffected
}

void SvmCpu::emulateLSRImm(uint16_t inst)
{
    unsigned imm5 = (inst >> 6) & 0x1f;
    unsigned Rm = (inst >> 3) & 0x7;
    unsigned Rd = inst & 0x7;

    if (imm5 == 0)
        imm5 = 32;

    regs[Rd] = (uint32_t) (regs[Rm] >> imm5);
    // TODO: C, N, Z flags - V is unaffected
}

void SvmCpu::emulateASRImm(uint16_t instr)
{
    unsigned imm5 = (instr >> 6) & 0x1f;
    unsigned Rm = (instr >> 3) & 0x7;
    unsigned Rd = instr & 0x7;

    if (imm5 == 0)
        imm5 = 32;

    // TODO: verify sign bit is being shifted in
    regs[Rd] = (uint32_t) (regs[Rm] >> imm5);
    // TODO: C, Z, N flags - V flag unchanged
}

void SvmCpu::emulateADDReg(uint16_t instr)
{
    unsigned Rm = (instr >> 6) & 0x7;
    unsigned Rn = (instr >> 3) & 0x7;
    unsigned Rd = instr & 0x7;

    regs[Rd] = (uint32_t) (regs[Rn] + regs[Rm]);
    // TODO: set N, Z, C and V flags
}

void SvmCpu::emulateSUBReg(uint16_t instr)
{
    unsigned Rm = (instr >> 6) & 0x7;
    unsigned Rn = (instr >> 3) & 0x7;
    unsigned Rd = instr & 0x7;

    regs[Rd] = (uint32_t) (regs[Rn] - regs[Rm]);
    // TODO: set N, Z, C and V flags
}

void SvmCpu::emulateADD3Imm(uint16_t instr)
{
    unsigned imm3 = (instr >> 6) & 0x7;
    unsigned Rn = (instr >> 3) & 0x7;
    unsigned Rd = instr & 0x7;

    regs[Rd] = (uint32_t) (regs[Rn] + imm3);
    // TODO: set N, Z, C and V flags
}

void SvmCpu::emulateSUB3Imm(uint16_t instr)
{
    unsigned imm3 = (instr >> 6) & 0x7;
    unsigned Rn = (instr >> 3) & 0x7;
    unsigned Rd = instr & 0x7;

    regs[Rd] = (uint32_t) (regs[Rn] - imm3);
    // TODO: set N, Z, C and V flags
}

void SvmCpu::emulateMovImm(uint16_t instr)
{
    unsigned Rd = (instr >> 8) & 0x7;
    unsigned imm8 = instr & 0xff;

    regs[Rd] = imm8;
}

void SvmCpu::emulateCmpImm(uint16_t instr)
{
    unsigned Rn = (instr >> 8) & 0x7;
    unsigned imm8 = instr & 0xff;

    unsigned out = regs[Rn] - imm8;
    // TODO: set N, Z, C, V flags accordingly
}

void SvmCpu::emulateADD8Imm(uint16_t instr)
{
    unsigned Rdn = (instr >> 8) & 0x7;
    unsigned imm8 = instr & 0xff;

    regs[Rdn] = (uint32_t) (regs[Rdn] + imm8);
    // TODO: set N, Z, C and V flags
}

void SvmCpu::emulateSUB8Imm(uint16_t instr)
{
    unsigned Rdn = (instr >> 8) & 0x7;
    unsigned imm8 = instr & 0xff;

    regs[Rdn] = (uint32_t) (regs[Rdn] - imm8);
    // TODO: set N, Z, C and V flags
}

///////////////////////////////////
// D A T A   P R O C E S S I N G
///////////////////////////////////

void SvmCpu::emulateANDReg(uint16_t instr)
{
    unsigned Rm = (instr >> 3) & 0x7;
    unsigned Rdn = instr & 0x7;

    regs[Rdn] = (uint32_t) (regs[Rdn] & regs[Rm]);
}

void SvmCpu::emulateEORReg(uint16_t instr)
{
    unsigned Rm = (instr >> 3) & 0x7;
    unsigned Rdn = instr & 0x7;

    regs[Rdn] = (uint32_t) (regs[Rdn] ^ regs[Rm]);
}

void SvmCpu::emulateLSLReg(uint16_t instr)
{
    unsigned Rm = (instr >> 3) & 0x7;
    unsigned Rdn = instr & 0x7;

    unsigned shift = regs[Rm] & 0xff;
    regs[Rdn] = (uint32_t) (regs[Rdn] << shift);
}

void SvmCpu::emulateLSRReg(uint16_t instr)
{
    unsigned Rm = (instr >> 3) & 0x7;
    unsigned Rdn = instr & 0x7;

    unsigned shift = regs[Rm] & 0xff;
    regs[Rdn] = (uint32_t) (regs[Rdn] >> shift);
}

void SvmCpu::emulateASRReg(uint16_t instr)
{
    unsigned Rm = (instr >> 3) & 0x7;
    unsigned Rdn = instr & 0x7;

    regs[Rdn] = (int32_t)regs[Rdn] >> regs[Rm];
}

void SvmCpu::emulateADCReg(uint16_t instr)
{
    unsigned Rm = (instr >> 3) & 0x7;
    unsigned Rdn = instr & 0x7;

    regs[Rdn] = (uint32_t) (regs[Rdn] + regs[Rm] + (getCarry() ? 1 : 0));
}

void SvmCpu::emulateSBCReg(uint16_t instr)
{
    unsigned Rm = (instr >> 3) & 0x7;
    unsigned Rdn = instr & 0x7;

    regs[Rdn] = (uint32_t) (regs[Rdn] - regs[Rm] - (getCarry() ? 0 : 1));
}

void SvmCpu::emulateRORReg(uint16_t instr)
{
    unsigned Rm = (instr >> 3) & 0x7;
    unsigned Rdn = instr & 0x7;

    regs[Rdn] = ROR(regs[Rdn], regs[Rm]);
}

void SvmCpu::emulateTSTReg(uint16_t instr)
{
    // nothing to do until handling status flags
}

void SvmCpu::emulateRSBImm(uint16_t instr)
{
    unsigned Rm = (instr >> 3) & 0x7;
    unsigned Rdn = instr & 0x7;

    // TODO: encoding T1 only has effect via status flags
}

void SvmCpu::emulateCMPReg(uint16_t instr)
{
    unsigned Rm = (instr >> 3) & 0x7;
    unsigned Rdn = instr & 0x7;

    // TODO: only has effect on the status flags
}

void SvmCpu::emulateCMNReg(uint16_t instr)
{
    unsigned Rm = (instr >> 3) & 0x7;
    unsigned Rdn = instr & 0x7;

    // TODO: only has effect on the status flags
}

void SvmCpu::emulateORRReg(uint16_t instr)
{
    unsigned Rm = (instr >> 3) & 0x7;
    unsigned Rdn = instr & 0x7;

    regs[Rdn] = (uint32_t) (regs[Rdn] | regs[Rm]);
}

void SvmCpu::emulateMUL(uint16_t instr)
{
    unsigned Rm = (instr >> 3) & 0x7;
    unsigned Rdn = instr & 0x7;

    uint64_t result = (uint64_t)regs[Rdn] * (uint64_t)regs[Rm];
    // TODO: Set Z and N flags. Z seems to be set based on full 64-bit result

    regs[Rdn] = (uint32_t) result;
}

void SvmCpu::emulateBICReg(uint16_t instr)
{
    unsigned Rm = (instr >> 3) & 0x7;
    unsigned Rdn = instr & 0x7;

    regs[Rdn] = (uint32_t) (regs[Rdn] & ~(regs[Rm]));
}

void SvmCpu::emulateMVNReg(uint16_t instr)
{
    unsigned Rm = (instr >> 3) & 0x7;
    unsigned Rdn = instr & 0x7;

    regs[Rdn] = (uint32_t) ~regs[Rm];
}

/////////////////////////////////////
// M I S C   I N S T R U C T I O N S
/////////////////////////////////////

void SvmCpu::emulateSXTH(uint16_t instr)
{
    unsigned Rm = (instr >> 3) & 0x7;
    unsigned Rdn = instr & 0x7;

    regs[Rdn] = (uint32_t) SignExtend<signed int, 16>(regs[Rm]);
}

void SvmCpu::emulateSXTB(uint16_t instr)
{
    unsigned Rm = (instr >> 3) & 0x7;
    unsigned Rdn = instr & 0x7;

    regs[Rdn] = (uint32_t) SignExtend<signed int, 8>(regs[Rm]);
}

void SvmCpu::emulateUXTH(uint16_t instr)
{
    unsigned Rm = (instr >> 3) & 0x7;
    unsigned Rdn = instr & 0x7;

    regs[Rdn] = regs[Rm] & 0xFFFF;
}

void SvmCpu::emulateUXTB(uint16_t instr)
{
    unsigned Rm = (instr >> 3) & 0x7;
    unsigned Rdn = instr & 0x7;

    regs[Rdn] = regs[Rm] & 0xFF;
}

///////////////////////////////////////////////
// B R A N C H I N G   I N S T R U C T I O N S
///////////////////////////////////////////////

void SvmCpu::emulateB(uint16_t instr)
{
    // encoding T2 only
    unsigned imm11 = instr & 0x7FF;
    BranchOffsetPC(SignExtend<signed int, 12>(imm11 << 1));
}

void SvmCpu::emulateCondB(uint16_t instr)
{
    unsigned cond = (instr >> 8) & 0xf;
    unsigned imm8 = instr & 0xff;

    if (conditionPassed(cond)) {
        BranchOffsetPC(SignExtend<signed int, 9>(imm8 << 1));
    }
}

void SvmCpu::emulateCBZ_CBNZ(uint16_t instr)
{
    bool nonzero = instr & (1 << 11);
    unsigned i = instr & (1 << 9);
    unsigned imm5 = (instr >> 3) & 0x1f;
    unsigned Rn = instr & 0x7;

    if (nonzero ^ (regs[Rn] == 0)) {
        // ZeroExtend(i:imm5:'0')
        BranchOffsetPC((i << 6) | (imm5 << 1));
    }
}


/////////////////////////////////////////
// M E M O R Y  I N S T R U C T I O N S
/////////////////////////////////////////

void SvmCpu::emulateSTRSPImm(uint16_t instr)
{
    // encoding T2 only
    unsigned Rt = (instr >> 8) & 0x7;
    unsigned imm8 = instr & 0xff;

    uint32_t *addr = reinterpret_cast<uint32_t*>(regs[REG_SP] + (imm8 << 2));
    ASSERT((reinterpret_cast<uint8_t*>(addr) - &mem[0]) < MEM_IN_BYTES && "store to invalid memory address");
    *addr = regs[Rt];
}

void SvmCpu::emulateLDRSPImm(uint16_t instr)
{
    // encoding T2 only
    unsigned Rt = (instr >> 8) & 0x7;
    unsigned imm8 = instr & 0xff;

    uint32_t *addr = reinterpret_cast<uint32_t*>(regs[REG_SP] + (imm8 << 2));
    ASSERT((reinterpret_cast<uint8_t*>(addr) - &mem[0]) < MEM_IN_BYTES && "load from invalid memory address");
    regs[Rt] = *addr;
}

void SvmCpu::emulateADDSpImm(uint16_t instr)
{
    // encoding T1 only
    unsigned Rd = (instr >> 8) & 0x7;
    unsigned imm8 = instr & 0xff;

    regs[Rd] = regs[REG_SP] + (imm8 << 2);
}

void SvmCpu::emulateLDRLitPool(uint16_t instr)
{
    unsigned Rt = (instr >> 8) & 0x7;
    unsigned imm8 = instr & 0xFF;

    // Round up to the next 32-bit boundary
    uint32_t *addr = reinterpret_cast<uint32_t*>
        (((regs[REG_PC] + 3) & ~3) + (imm8 << 2));

    // this should only come from our current flash block
    ASSERT((reinterpret_cast<reg_t>(addr) - runtime.cacheBlockBase()) < FlashLayer::BLOCK_SIZE && "PC relative load from invalid address");
    regs[Rt] = *addr;
}


////////////////////////////////////////
// 3 2 - B I T  I N S T R U C T I O N S
////////////////////////////////////////

void SvmCpu::emulateSTR(uint32_t instr)
{
    unsigned imm12 = instr & 0xFFF;
    unsigned Rn = (instr >> 16) & 0xF;
    unsigned Rt = (instr >> 12) & 0xF;
    reg_t addr = regs[Rn] + imm12;

    *reinterpret_cast<uint32_t*>(addr) = regs[Rt];
}

void SvmCpu::emulateLDR(uint32_t instr)
{
    unsigned imm12 = instr & 0xFFF;
    unsigned Rn = (instr >> 16) & 0xF;
    unsigned Rt = (instr >> 12) & 0xF;
    reg_t addr = regs[Rn] + imm12;

    regs[Rt] = *reinterpret_cast<uint32_t*>(addr);
}

void SvmCpu::emulateSTRBH(uint32_t instr)
{
    const unsigned HalfwordBit = 1 << 21;

    unsigned imm12 = instr & 0xFFF;
    unsigned Rn = (instr >> 16) & 0xF;
    unsigned Rt = (instr >> 12) & 0xF;
    reg_t addr = regs[Rn] + imm12;

    if (instr & HalfwordBit) {
        *reinterpret_cast<uint16_t*>(addr) = regs[Rt];
    } else {
        *reinterpret_cast<uint8_t*>(addr) = regs[Rt];
    }
}

void SvmCpu::emulateLDRBH(uint32_t instr)
{
    const unsigned HalfwordBit = 1 << 21;
    const unsigned SignExtBit = 1 << 24;

    unsigned imm12 = instr & 0xFFF;
    unsigned Rn = (instr >> 16) & 0xF;
    unsigned Rt = (instr >> 12) & 0xF;
    reg_t addr = regs[Rn] + imm12;

    switch (instr & (HalfwordBit | SignExtBit)) {
    case 0:
        regs[Rt] = *reinterpret_cast<uint8_t*>(addr);
        break;
    case HalfwordBit:
        regs[Rt] = *reinterpret_cast<uint16_t*>(addr);
        break;
    case SignExtBit:
        regs[Rt] = (uint32_t) SignExtend<signed int, 8>
            (*reinterpret_cast<uint8_t*>(addr));
        break;
    case HalfwordBit | SignExtBit:
        regs[Rt] = (uint32_t) SignExtend<signed int, 16>
            (*reinterpret_cast<uint16_t*>(addr));
        break;
    }
}

void SvmCpu::emulateMOVWT(uint32_t instr)
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

void SvmCpu::emulateDIV(uint32_t instr)
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

//////////////////////////
// S Y S T E M  C A L L S
//////////////////////////

void SvmCpu::emulateSVC(uint16_t instr)
{
    uint8_t imm8 = instr & 0xff;
    runtime.svc(imm8);
}
