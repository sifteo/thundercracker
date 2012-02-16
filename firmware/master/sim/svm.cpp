#include "svm.h"
#include "elfdefs.h"
#include "flashlayer.h"

#include <string.h>

Svm::Svm()
{
}

SvmProgram::SvmProgram()
{
}

void SvmProgram::run()
{
    if (!loadElfFile(0, 12345))
        return;

    LOG(("loaded. entry: 0x%x text start: 0x%x\n", progInfo.entry, progInfo.textRodata.start));

    validate();

    memset(regs, 0, sizeof(regs));  // zero out general purpose regs
    cpsr = 0;

    // TODO: do all of our instructions support conditional execution?
    FlashRegion r;
    if (!FlashLayer::getRegion(progInfo.textRodata.start + progInfo.entry, FlashLayer::BLOCK_SIZE, &r)) {
        LOG(("failed to get entry block\n"));
        return;
    }

    uint16_t *instr = static_cast<uint16_t*>(r.data());
    unsigned bsize = r.size();
    for (; bsize != 0; bsize -= sizeof(uint32_t), instr += 2) {
        if (instructionSize(instr[0]) == InstrBits32) {
            // swap nibbles
            execute32((instr[0] << 16) | instr[1]);
        }
        else {
            execute16(instr[0]);
            execute16(instr[1]);
        }
    }
}

void SvmProgram::cycle()
{
}

void SvmProgram::validate()
{
    FlashRegion r;
    if (!FlashLayer::getRegion(progInfo.textRodata.start + progInfo.entry, FlashLayer::BLOCK_SIZE, &r)) {
        LOG(("failed to get entry block\n"));
        return;
    }

    uint16_t *instr = static_cast<uint16_t*>(r.data());
    unsigned bsize = r.size();
    for (; bsize != 0; bsize -= sizeof(uint32_t), instr += 2) {
        if (instructionSize(instr[0]) == InstrBits32) {
            // swap nibbles
            if (!isValid32((instr[0] << 16) | instr[1]))
                break;
        }
        else {
            if (!isValid16(instr[0]))
                break;
            if (!isValid16(instr[1]))
                break;
        }
    }
    LOG(("validation complete\n"));
}

SvmProgram::InstructionSize SvmProgram::instructionSize(uint16_t instr) const
{
    // if bits [15:11] are 0b11101, 0b11110 or 0b11111, it's a 32-bit instruction
    // 0xe800 == 0b11101 << 11
    return (instr & 0xf800) >= 0xe800 ? InstrBits32 : InstrBits16;
}

/*
Allowed 16-bit instruction encodings:

  00xxxxxx xxxxxxxx     lsl, lsr, asr, add, sub, mov, cmp
  010000xx xxxxxxxx     and, eor, lsl, lsr, asr, adc, sbc, ror,
                        tst, rsb, cmp, cmn, orr, mul, bic, mvn

  10110010 xxxxxxxx     uxth, sxth, uxtb, sxtb
  10111111 00000000     nop
  11011111 xxxxxxxx     svc

  01001xxx xxxxxxxx     ldr r0-r7, [PC, #imm8]
  1001xxxx xxxxxxxx     ldr/str r0-r7, [SP, #imm8]
  10101xxx xxxxxxxx     add r0-r7, SP, #imm8

Near branches are allowed, assuming the target address is within the code
region of the same page, and the target is 32-bit aligned:

  1011x0x1 xxxxxxxx     cb(n)z
  1101xxxx xxxxxxxx     bcc
  11100xxx xxxxxxxx     b
*/
bool SvmProgram::isValid16(uint16_t instr)
{
    if ((instr & AluMask) == AluTest) {
        LOG(("arithmetic\n"));
        return true;
    }
    if ((instr & DataProcMask) == DataProcTest) {
        LOG(("data processing\n"));
        return true;
    }
    if ((instr & MiscMask) == MiscTest) {
        LOG(("miscellaneous\n"));
        return true;
    }
    if ((instr & SvcMask) == SvcTest) {
        LOG(("svc\n"));
        return true;
    }
    if ((instr & PcRelLdrMask) == PcRelLdrTest) {
        LOG(("pc relative ldr\n"));
        return true;
    }
    if ((instr & SpRelLdrStrMask) == SpRelLdrStrTest) {
        LOG(("sp relative ldr/str\n"));
        return true;
    }
    if ((instr & SpRelAddMask) == SpRelAddTest) {
        LOG(("sp relative add\n"));
        return true;
    }
    if ((instr & UncondBranchMask) == UncondBranchTest) {
        LOG(("unconditional branch\n"));
        // TODO: must validate target
        return true;
    }
    if ((instr & CompareBranchMask) == CompareBranchTest) {
        LOG(("compare and branch\n"));
        // TODO: must validate target
        return true;
    }
    if ((instr & CondBranchMask) == CondBranchTest) {
        LOG(("branchcc\n"));
        // TODO: must validate target
        return true;
    }
    if (instr == Nop) {
        // 10111111 00000000     nop
        LOG(("nop\n"));
        return true;
    }
    LOG(("*********************************** invalid 16bit instruction: 0x%x\n", instr));
    return false;
}

void SvmProgram::execute16(uint16_t instr)
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
        uint8_t isLoad = instr & (1 << 11);
        if (isLoad)
            emulateLDRImm(instr);
        else
            emulateSTRImm(instr);
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
        regs[Rd] = regs[Rm] << imm5;
        // TODO: carry flags
    }
    // TODO: N, Z flags - V unaffected
}

void SvmProgram::emulateLSRImm(uint16_t inst)
{
    if (!conditionPassed(inst))
        return;

    unsigned imm5 = (inst >> 6) & 0x1f;
    unsigned Rm = (inst >> 3) & 0x7;
    unsigned Rd = inst & 0x7;

    if (imm5 == 0)
        imm5 = 32;

    regs[Rd] = regs[Rm] >> imm5;
    // TODO: C, N, Z flags - V is unaffected
}

void SvmProgram::emulateASRImm(uint16_t instr)
{
    if (!conditionPassed(instr))
        return;

    unsigned imm5 = (instr >> 6) & 0x1f;
    unsigned Rm = (instr >> 3) & 0x7;
    unsigned Rd = instr & 0x7;

    if (imm5 == 0)
        imm5 = 32;

    // TODO: verify sign bit is being shifted in
    regs[Rd] = regs[Rm] >> imm5;
    // TODO: C, Z, N flags - V flag unchanged
}

void SvmProgram::emulateADDReg(uint16_t instr)
{
    if (!conditionPassed(instr))
        return;

    unsigned Rm = (instr >> 6) & 0x7;
    unsigned Rn = (instr >> 3) & 0x7;
    unsigned Rd = instr & 0x7;

    regs[Rd] = regs[Rn] + regs[Rm];
    // TODO: set N, Z, C and V flags
}

void SvmProgram::emulateSUBReg(uint16_t instr)
{
    if (!conditionPassed(instr))
        return;

    unsigned Rm = (instr >> 6) & 0x7;
    unsigned Rn = (instr >> 3) & 0x7;
    unsigned Rd = instr & 0x7;

    regs[Rd] = regs[Rn] - regs[Rm];
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

// D A T A   P R O C E S S I N G

void SvmProgram::emulateANDReg(uint16_t instr)
{
    // TODO
}

void SvmProgram::emulateEORReg(uint16_t instr)
{
    // TODO
}

void SvmProgram::emulateLSLReg(uint16_t instr)
{
    // TODO
}

void SvmProgram::emulateLSRReg(uint16_t instr)
{
    // TODO
}

void SvmProgram::emulateASRReg(uint16_t instr)
{
    // TODO
}

void SvmProgram::emulateADCReg(uint16_t instr)
{
    // TODO
}

void SvmProgram::emulateSBCReg(uint16_t instr)
{
    // TODO
}

void SvmProgram::emulateRORReg(uint16_t instr)
{
    // TODO
}

void SvmProgram::emulateTSTReg(uint16_t instr)
{
    // TODO
}

void SvmProgram::emulateRSBImm(uint16_t instr)
{
    // TODO
}

void SvmProgram::emulateCMPReg(uint16_t instr)
{
    // TODO
}

void SvmProgram::emulateCMNReg(uint16_t instr)
{
    // TODO
}

void SvmProgram::emulateORRReg(uint16_t instr)
{
    // TODO
}

void SvmProgram::emulateMUL(uint16_t instr)
{
    // TODO
}

void SvmProgram::emulateBICReg(uint16_t instr)
{
    // TODO
}

void SvmProgram::emulateMVNReg(uint16_t instr)
{
    // TODO
}

// M I S C   I N S T R U C T I O N S

void SvmProgram::emulateSXTH(uint16_t isntr)
{
    // TODO
}

void SvmProgram::emulateSXTB(uint16_t isntr)
{
    // TODO
}

void SvmProgram::emulateUXTH(uint16_t isntr)
{
    // TODO
}

void SvmProgram::emulateUXTB(uint16_t isntr)
{
    // TODO
}

void SvmProgram::emulateB(uint16_t instr)
{
    // encoding T2 only
    unsigned imm11 = instr & 0x3FF;
    BranchWritePC(regs[REG_PC] + imm11);
}

void SvmProgram::emulateCondB(uint16_t instr)
{
    unsigned cond = (instr >> 8) & 0xf;
    unsigned imm8 = instr & 0xff;

    if (conditionPassed(cond)) {
        uint32_t offset = SignExtend<uint32_t, 9>(imm8 << 1);
        BranchWritePC(regs[REG_PC] + offset);
    }
}

void SvmProgram::emulateCBZ_CBNZ(uint16_t instr)
{
    bool nonzero = instr & (1 << 11);
    unsigned i = instr & (1 << 9);
    unsigned imm5 = (instr >> 3) & 0x1f;
    unsigned Rn = instr & 0x7;

    // ZeroExtend(i:imm5:'0')
    unsigned offset = (i << 6) | (imm5 << 1);

    if (nonzero ^ (regs[Rn] == 0)) {
        BranchWritePC(regs[REG_PC] + offset);
    }
}

void SvmProgram::emulateSTRImm(uint16_t instr)
{
    // encoding T2 only
    unsigned Rt = (instr >> 8) & 0x7;
    unsigned imm8 = instr & 0xff;

    // TODO: store to mem
    // "Allowed constant values are multiples of 4 in the range of 0-1020 for
    // encoding T1", so shift it on over.
}

void SvmProgram::emulateLDRImm(uint16_t instr)
{
    // encoding T2 only
    unsigned Rt = (instr >> 8) & 0x7;
    unsigned imm8 = instr & 0xff;

    // TODO: load from mem
    // "Allowed constant values are multiples of 4 in the range of 0-1020 for
    // encoding T1", so shift it on over.
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

void SvmProgram::emulateSVC(uint16_t instr)
{
    unsigned imm8 = instr & 0xFF;
    LOG(("SVC #%d\n", imm8));
    // TODO: find params
    // syscall(imm8);
}

/*
Allowed 32-bit instruction encodings:

  11111000 11001001, 0xxxxxxx xxxxxxxx      str         r0-r7, [r9, #imm12]
  11111000 10x01001, 0xxxxxxx xxxxxxxx      str[bh]     r0-r7, [r9, #imm12]

  1111100x 10x1100x, 0xxxxxxx xxxxxxxx      ldr(s)[bh]  r0-r7, [r8-9, #imm12]
  11111000 1101100x, 0xxxxxxx xxxxxxxx      ldr         r0-r7, [r8-9, #imm12]

  11110x10 x100xxxx, 0xxx0xxx xxxxxxxx      mov[wt]     r0-r7, #imm16
*/
bool SvmProgram::isValid32(uint32_t instr)
{
    if ((instr & StrMask) == StrTest) {
        LOG(("32bit str\n"));
        return true;
    }
    if ((instr & StrBhMask) == StrBhTest) {
        LOG(("32bit str[bh]\n"));
        return true;
    }
    if ((instr & LdrBhMask) == LdrBhTest) {
        LOG(("32bit ldr(s)[bh]\n"));
        return true;
    }
    if ((instr & LdrMask) == LdrTest) {
        LOG(("32bit ldr(s)[bh]\n"));
        return true;
    }
    if ((instr & MovWtMask) == MovWtTest) {
        LOG(("32bit mov[wt]\n"));
        return true;
    }
    LOG(("----------------------- invalid 32-bit instruction: 0x%x\n", instr));
    return false;
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
            break;
        case (Elf::PF_Read | Elf::PF_Write):
            if (pHeader.p_memsz >= 0 && pHeader.p_filesz == 0) {
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
