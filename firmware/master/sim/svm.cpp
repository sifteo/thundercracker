#include "svm.h"
#include "elfdefs.h"
#include "flashlayer.h"
#include "svmutils.h"
#include "svmvalidator.h"

#include <string.h>

SvmProgram::SvmProgram()
{
}

void SvmProgram::run(uint16_t appId)
{
    currentAppPhysAddr = 0;  // TODO: look this up via appId
    if (!loadElfFile(currentAppPhysAddr, 12345))
        return;

    LOG(("loaded. entry: 0x%x text start: 0x%x\n", progInfo.entry, progInfo.textRodata.start));

    memset(regs, 0, sizeof(regs));  // zero out general purpose regs
    cpsr = 0;

    unsigned entryPoint = progInfo.textRodata.start + progInfo.entry;
    if (!FlashLayer::getRegion(entryPoint, FlashLayer::BLOCK_SIZE, &flashRegion)) {
        LOG(("failed to get entry block\n"));
        return;
    }

    // TODO: This actually needs to be like a call SVC, not just a branch
    regs[REG_PC] = reinterpret_cast<reg_t>(flashRegion.data())
        + (progInfo.textRodata.vaddr & 0xFFFFFF);

    unsigned size = SvmValidator::validBytes(flashRegion.data(), flashRegion.size());

    for (;;) {
        uint16_t instr = fetch();
        if (Svm::instructionSize(instr) == Svm::InstrBits16) {
            execute16(instr);
        }
        else {
            uint16_t instrLow = fetch();
            execute32(instr << 16 | instrLow);
        }
    }
}

/*
    Fetch the next instruction.
    We can return 16bit values unconditionally, since all our instructions are Thumb,
    and the first nibble of a 32-bit instruction must be checked regardless
    in order to determine its bitness.
*/
uint16_t SvmProgram::fetch()
{
    uint16_t *tst = reinterpret_cast<uint16_t*>(regs[REG_PC]);

#if 1
    LOG(("[%p] %04x", tst, *tst));
    for (unsigned r = 0; r < 8; r++) {
        assert((uint32_t)regs[r] == regs[r]);
        LOG((" r%d=%08x", r, (uint32_t) regs[r]));
    }
    LOG(("\n"));
#endif
    
    regs[REG_PC] += sizeof(uint16_t);
    return *tst;
}

void SvmProgram::execute16(uint16_t instr)
{
    if ((instr & Svm::AluMask) == Svm::AluTest) {
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
    if ((instr & Svm::DataProcMask) == Svm::DataProcTest) {
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
    if ((instr & Svm::MiscMask) == Svm::MiscTest) {
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
    if ((instr & Svm::SvcMask) == Svm::SvcTest) {
        emulateSVC(instr);
        return;
    }
    if ((instr & Svm::PcRelLdrMask) == Svm::PcRelLdrTest) {
        emulateLDRLitPool(instr);
        return;
    }
    if ((instr & Svm::SpRelLdrStrMask) == Svm::SpRelLdrStrTest) {
        uint16_t isLoad = instr & (1 << 11);
        if (isLoad)
            emulateLDRSPImm(instr);
        else
            emulateSTRSPImm(instr);
        return;
    }
    if ((instr & Svm::SpRelAddMask) == Svm::SpRelAddTest) {
        emulateADDSpImm(instr);
        return;
    }
    if ((instr & Svm::UncondBranchMask) == Svm::UncondBranchTest) {
        emulateB(instr);
        return;
    }
    if ((instr & Svm::CompareBranchMask) == Svm::CompareBranchTest) {
        emulateCBZ_CBNZ(instr);
        return;
    }
    if ((instr & Svm::CondBranchMask) == Svm::CondBranchTest) {
        emulateCondB(instr);
        return;
    }
    if (instr == Svm::Nop) {
        // nothing to do
        return;
    }
    // should never get here since we should only be executing validated instructions
    LOG(("*********************************** invalid 16bit instruction: 0x%x\n", instr));
    ASSERT(0 && "unhandled instruction group!");
}

void SvmProgram::execute32(uint32_t instr)
{

}

bool SvmProgram::conditionPassed(uint8_t cond)
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
void SvmProgram::emulateLSLImm(uint16_t inst)
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

void SvmProgram::emulateLSRImm(uint16_t inst)
{
    unsigned imm5 = (inst >> 6) & 0x1f;
    unsigned Rm = (inst >> 3) & 0x7;
    unsigned Rd = inst & 0x7;

    if (imm5 == 0)
        imm5 = 32;

    regs[Rd] = (uint32_t) (regs[Rm] >> imm5);
    // TODO: C, N, Z flags - V is unaffected
}

void SvmProgram::emulateASRImm(uint16_t instr)
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

void SvmProgram::emulateADDReg(uint16_t instr)
{
    unsigned Rm = (instr >> 6) & 0x7;
    unsigned Rn = (instr >> 3) & 0x7;
    unsigned Rd = instr & 0x7;

    regs[Rd] = (uint32_t) (regs[Rn] + regs[Rm]);
    // TODO: set N, Z, C and V flags
}

void SvmProgram::emulateSUBReg(uint16_t instr)
{
    unsigned Rm = (instr >> 6) & 0x7;
    unsigned Rn = (instr >> 3) & 0x7;
    unsigned Rd = instr & 0x7;

    regs[Rd] = (uint32_t) (regs[Rn] - regs[Rm]);
    // TODO: set N, Z, C and V flags
}

void SvmProgram::emulateADD3Imm(uint16_t instr)
{
    // TODO
}

void SvmProgram::emulateSUB3Imm(uint16_t instr)
{
    // TODO
}

void SvmProgram::emulateMovImm(uint16_t instr)
{
    unsigned Rd = (instr >> 8) & 0x7;
    unsigned imm8 = instr & 0xff;

    regs[Rd] = imm8;
}

void SvmProgram::emulateCmpImm(uint16_t instr)
{
    unsigned Rn = (instr >> 8) & 0x7;
    unsigned imm8 = instr & 0xff;

    unsigned out = regs[Rn] - imm8;
    // TODO: set N, Z, C, V flags accordingly
}

void SvmProgram::emulateADD8Imm(uint16_t instr)
{
    // TODO
}

void SvmProgram::emulateSUB8Imm(uint16_t instr)
{
    // TODO
}

///////////////////////////////////
// D A T A   P R O C E S S I N G
///////////////////////////////////

void SvmProgram::emulateANDReg(uint16_t instr)
{
    unsigned Rm = (instr >> 3) & 0x7;
    unsigned Rdn = instr & 0x7;

    regs[Rdn] = (uint32_t) (regs[Rdn] & regs[Rm]);
}

void SvmProgram::emulateEORReg(uint16_t instr)
{
    unsigned Rm = (instr >> 3) & 0x7;
    unsigned Rdn = instr & 0x7;

    regs[Rdn] = (uint32_t) (regs[Rdn] ^ regs[Rm]);
}

void SvmProgram::emulateLSLReg(uint16_t instr)
{
    unsigned Rm = (instr >> 3) & 0x7;
    unsigned Rdn = instr & 0x7;

    unsigned shift = regs[Rm] & 0xff;
    regs[Rdn] = (uint32_t) (regs[Rdn] << shift);
}

void SvmProgram::emulateLSRReg(uint16_t instr)
{
    unsigned Rm = (instr >> 3) & 0x7;
    unsigned Rdn = instr & 0x7;

    unsigned shift = regs[Rm] & 0xff;
    regs[Rdn] = (uint32_t) (regs[Rdn] >> shift);
}

void SvmProgram::emulateASRReg(uint16_t instr)
{
    unsigned Rm = (instr >> 3) & 0x7;
    unsigned Rdn = instr & 0x7;

    regs[Rdn] = (int32_t)regs[Rdn] >> regs[Rm];
}

void SvmProgram::emulateADCReg(uint16_t instr)
{
    unsigned Rm = (instr >> 3) & 0x7;
    unsigned Rdn = instr & 0x7;

    regs[Rdn] = (uint32_t) (regs[Rdn] + regs[Rm] + (getCarry() ? 1 : 0));
}

void SvmProgram::emulateSBCReg(uint16_t instr)
{
    unsigned Rm = (instr >> 3) & 0x7;
    unsigned Rdn = instr & 0x7;

    regs[Rdn] = (uint32_t) (regs[Rdn] - regs[Rm] - (getCarry() ? 0 : 1));
}

void SvmProgram::emulateRORReg(uint16_t instr)
{
    unsigned Rm = (instr >> 3) & 0x7;
    unsigned Rdn = instr & 0x7;

    regs[Rdn] = ROR(regs[Rdn], regs[Rm]);
}

void SvmProgram::emulateTSTReg(uint16_t instr)
{
    // nothing to do until handling status flags
}

void SvmProgram::emulateRSBImm(uint16_t instr)
{
    unsigned Rm = (instr >> 3) & 0x7;
    unsigned Rdn = instr & 0x7;

    // TODO: encoding T1 only has effect via status flags
}

void SvmProgram::emulateCMPReg(uint16_t instr)
{
    unsigned Rm = (instr >> 3) & 0x7;
    unsigned Rdn = instr & 0x7;

    // TODO: only has effect on the status flags
}

void SvmProgram::emulateCMNReg(uint16_t instr)
{
    unsigned Rm = (instr >> 3) & 0x7;
    unsigned Rdn = instr & 0x7;

    // TODO: only has effect on the status flags
}

void SvmProgram::emulateORRReg(uint16_t instr)
{
    unsigned Rm = (instr >> 3) & 0x7;
    unsigned Rdn = instr & 0x7;

    regs[Rdn] = (uint32_t) (regs[Rdn] | regs[Rm]);
}

void SvmProgram::emulateMUL(uint16_t instr)
{
    unsigned Rm = (instr >> 3) & 0x7;
    unsigned Rdn = instr & 0x7;
    
    uint64_t result = (uint64_t)regs[Rdn] * (uint64_t)regs[Rm];
    // TODO: Set Z and N flags. Z seems to be set based on full 64-bit result

    regs[Rdn] = (uint32_t) result;
}

void SvmProgram::emulateBICReg(uint16_t instr)
{
    unsigned Rm = (instr >> 3) & 0x7;
    unsigned Rdn = instr & 0x7;

    regs[Rdn] = (uint32_t) (regs[Rdn] & ~(regs[Rm]));
}

void SvmProgram::emulateMVNReg(uint16_t instr)
{
    unsigned Rm = (instr >> 3) & 0x7;
    unsigned Rdn = instr & 0x7;

    regs[Rdn] = (uint32_t) ~regs[Rm];
}

/////////////////////////////////////
// M I S C   I N S T R U C T I O N S
/////////////////////////////////////

void SvmProgram::emulateSXTH(uint16_t instr)
{
    unsigned Rm = (instr >> 3) & 0x7;
    unsigned Rdn = instr & 0x7;

    regs[Rdn] = (uint32_t) SignExtend<signed int, 16>(regs[Rm]);
}

void SvmProgram::emulateSXTB(uint16_t instr)
{
    unsigned Rm = (instr >> 3) & 0x7;
    unsigned Rdn = instr & 0x7;

    regs[Rdn] = (uint32_t) SignExtend<signed int, 8>(regs[Rm]);
}

void SvmProgram::emulateUXTH(uint16_t instr)
{
    unsigned Rm = (instr >> 3) & 0x7;
    unsigned Rdn = instr & 0x7;

    regs[Rdn] = regs[Rm] & 0xFFFF;
}

void SvmProgram::emulateUXTB(uint16_t instr)
{
    unsigned Rm = (instr >> 3) & 0x7;
    unsigned Rdn = instr & 0x7;

    regs[Rdn] = regs[Rm] & 0xFF;
}

///////////////////////////////////////////////
// B R A N C H I N G   I N S T R U C T I O N S
///////////////////////////////////////////////

void SvmProgram::emulateB(uint16_t instr)
{
    // encoding T2 only
    unsigned imm11 = instr & 0x7FF;
    BranchOffsetPC(SignExtend<signed int, 12>(imm11 << 1));
}

void SvmProgram::emulateCondB(uint16_t instr)
{
    unsigned cond = (instr >> 8) & 0xf;
    unsigned imm8 = instr & 0xff;

    if (conditionPassed(cond)) {
        BranchOffsetPC(SignExtend<signed int, 9>(imm8 << 1));
    }
}

void SvmProgram::emulateCBZ_CBNZ(uint16_t instr)
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

void SvmProgram::emulateSTRSPImm(uint16_t instr)
{
    // encoding T2 only
    unsigned Rt = (instr >> 8) & 0x7;
    unsigned imm8 = instr & 0xff;

    uint32_t *addr = reinterpret_cast<uint32_t*>(regs[REG_SP] + (imm8 << 2));
    *addr = regs[Rt];
}

void SvmProgram::emulateLDRSPImm(uint16_t instr)
{
    // encoding T2 only
    unsigned Rt = (instr >> 8) & 0x7;
    unsigned imm8 = instr & 0xff;

    uint32_t *addr = reinterpret_cast<uint32_t*>(regs[REG_SP] + (imm8 << 2));
    regs[Rt] = *addr;
}

void SvmProgram::emulateADDSpImm(uint16_t instr)
{
    // encoding T1 only
    unsigned Rd = (instr >> 8) & 0x7;
    unsigned imm8 = instr & 0xff;

    // "Allowed constant values are multiples of 4 in the range of 0-1020 for
    // encoding T1", so shift it on over.
    regs[Rd] = regs[REG_SP] + (imm8 << 2);
}

void SvmProgram::emulateLDRLitPool(uint16_t instr)
{
    unsigned Rt = (instr >> 8) & 0x7;
    unsigned imm8 = instr & 0xFF;

    // Round up to the next 32-bit boundary
    uint32_t *addr = reinterpret_cast<uint32_t*>
        (((regs[REG_PC] + 3) & ~3) + (imm8 << 2));

    regs[Rt] = *addr;
}

/*
SVC encodings:

  11011111 0xxxxxxx     (1) Indirect operation
  11011111 10xxxxxx     (2) Direct syscall #0-63
  11011111 110xxxxx     (3) SP = validate(SP - imm5*4)
  11011111 11100rrr     (4) r8-9 = validate(rN)
  11011111 11101rrr     (5) Reserved
  11011111 11110rrr     (6) Call validate(rN), with SP adjust
  11011111 11111rrr     (7) Tail call validate(rN), with SP adjust
*/
void SvmProgram::emulateSVC(uint16_t instr)
{
    unsigned imm8 = instr & 0xFF;
    if ((imm8 & (1 << 7)) == 0) {
        svcIndirectOperation(imm8);
        LOG(("indirect syscall\n"));
        return;
    }
    if ((imm8 & (0x2 << 6)) == (0x2 << 6)) {
        LOG(("direct syscall\n"));
        return;
    }
    if ((imm8 & (0x6 << 5)) == (0x6 << 5)) {
        LOG(("SP = validate(SP - imm5*4)\n"));
        return;
    }

    uint8_t sub = (imm8 >> 3) & 0x1f;
    switch (sub) {
    case 0x1c:  // 0b11100
        LOG(("svc: r8-9 = validate(rN)\n"));
        return;
    case 0x1d:  // 0b11101
        LOG(("svc: reserved\n"));
        return;
    case 0x1e:  // 0b11110
        LOG(("svc: Call validate(rN), with SP adjust\n"));
        return;
    case 0x1f:  // 0b11110
        LOG(("svc: Tail call validate(rN), with SP adjust\n"));
        return;
    default:
        ASSERT(0 && "unknown sub op for svc");
        break;
    }
    ASSERT(0 && "unhandled SVC");
}

/*
When the MSB of the SVC immediate is clear, the other 7 bits are multiplied by
four and added to the base of the current page, to form the address of a
32-bit literal. This 32-bit literal encodes an indirect operation to perform:

  0nnnnnnn aaaaaaaa aaaaaaaa aaaaaa00   (1) Call validate(F+a*4), SP -= n*4
  0nnnnnnn aaaaaaaa aaaaaaaa aaaaaa01   (2) Tail call validate(F+a*4), SP -= n*4
  0xxxxxxx xxxxxxxx xxxxxxxx xxxxxx1x   (3) Reserved
  10nnnnnn nnnnnnnn iiiiiiii iiiiiii0   (4) Syscall #0-8191, with #imm15
  10nnnnnn nnnnnnnn iiiiiiii iiiiiii1   (4) Tail syscall #0-8191, with #imm15
  110nnnnn aaaaaaaa aaaaaaaa aaaaaaaa   (5) Addrop #0-31 on validate(a)
  111nnnnn aaaaaaaa aaaaaaaa aaaaaaaa   (6) Addrop #0-31 on validate(F+a)

  (F = 0x80000000, flash segment virtual address)
*/
void SvmProgram::svcIndirectOperation(uint8_t imm8)
{
    // we already know the MSB of imm8 is clear by the fact that we're being called here.

    reg_t instructionBase = regs[REG_PC] - 2;
    uintptr_t blockMask = ~(uintptr_t)(FlashLayer::BLOCK_SIZE - 1);
    uint32_t *blockBase = reinterpret_cast<uint32_t*>(instructionBase & blockMask);
    uint32_t literal = blockBase[imm8];

    if ((literal & Svm::CallMask) == Svm::CallTest) {
        LOG(("indirect call\n"));
    }
    else if ((literal & Svm::TailCallMask) == Svm::TailCallTest) {
        LOG(("indirect tail call\n"));
    }
    else if ((literal & Svm::IndirectSyscallMask) == Svm::IndirectSyscallTest) {
        LOG(("indirect syscall\n"));
    }
    else if ((literal & Svm::TailSyscallMask) == Svm::TailSyscallTest) {
        LOG(("indirect tail syscall\n"));
    }
    else if ((literal & Svm::AddropMask) == Svm::AddropTest) {
        LOG(("Addrop #0-31 on validate(a)\n"));
    }
    else if ((literal & Svm::AddropFlashMask) == Svm::AddropFlashTest) {
        LOG(("Addrop #0-31 on validate(F+a)\n"));
    }
    else {
        ASSERT(0 && "unhandled svc Indirect Operation");
    }
}

bool SvmProgram::loadElfFile(unsigned addr, unsigned len)
{
    FlashRegion r;
    if (!FlashLayer::getRegion(addr, FlashLayer::BLOCK_SIZE, &r)) {
        LOG(("couldn't get flash region for elf file\n"));
        return false;
    }
    uint8_t *block = static_cast<uint8_t*>(r.data());

    // verify magicality
    if ((block[Elf::EI_MAG0] != Elf::Magic0) ||
        (block[Elf::EI_MAG1] != Elf::Magic1) ||
        (block[Elf::EI_MAG2] != Elf::Magic2) ||
        (block[Elf::EI_MAG3] != Elf::Magic3))
    {
        LOG(("incorrect elf magic number"));
        return false;
    }


    Elf::FileHeader header;
    memcpy(&header, block, sizeof header);

    // ensure the file is the correct Elf version
    if (header.e_ident[Elf::EI_VERSION] != Elf::EV_CURRENT) {
        LOG(("incorrect elf file version\n"));
        return false;
    }

    // ensure the file is the correct data format
    union {
        int32_t l;
        char c[sizeof (int32_t)];
    } u;
    u.l = 1;
    if ((u.c[sizeof(int32_t) - 1] + 1) != header.e_ident[Elf::EI_DATA]) {
        LOG(("incorrect elf data format\n"));
        return false;
    }

    if (header.e_machine != Elf::EM_ARM) {
        LOG(("elf: incorrect machine format\n"));
        return false;
    }

    progInfo.entry = header.e_entry;
    /*
        We're looking for 3 segments, identified by the following profiles:
        - text segment - executable & read-only
        - data segment - read/write & non-zero size on disk
        - bss segment - read/write, zero size on disk, and non-zero size in memory
    */
    unsigned offset = header.e_phoff;
    for (unsigned i = 0; i < header.e_phnum; ++i, offset += header.e_phentsize) {
        Elf::ProgramHeader pHeader;
        memcpy(&pHeader, block + offset, header.e_phentsize);
        if (pHeader.p_type != Elf::PT_LOAD)
            continue;

        switch (pHeader.p_flags) {
        case (Elf::PF_Exec | Elf::PF_Read):
            LOG(("rodata/text segment found\n"));
            progInfo.textRodata.start = pHeader.p_offset;
            progInfo.textRodata.size = pHeader.p_filesz;
            progInfo.textRodata.vaddr = pHeader.p_vaddr;
            progInfo.textRodata.paddr = pHeader.p_paddr;
            break;
        case (Elf::PF_Read | Elf::PF_Write):
            if (pHeader.p_memsz > 0 && pHeader.p_filesz == 0) {
                LOG(("bss segment found\n"));
                progInfo.bss.start = pHeader.p_offset;
                progInfo.bss.size = pHeader.p_memsz;
            }
            else if (pHeader.p_filesz > 0) {
                LOG(("found rwdata segment\n"));
                progInfo.rwdata.start = pHeader.p_offset;
                progInfo.rwdata.size = pHeader.p_memsz;
            }
            break;
        }
    }

    FlashLayer::releaseRegion(r);
    return true;
}
