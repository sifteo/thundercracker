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
//    for () {
//        cycle();
//    }
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
        // if bits [15:11] are 0b11101, 0b11110 or 0b11111, it's a 32-bit instruction
        if ((instr[0] & 0xf800) >= 0xe800) { // 0xe800 == 0b11101 << 11
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
bool SvmProgram::isValid16(uint16_t halfword)
{
    if ((halfword & AluMask) == AluTest) {
        LOG(("arithmetic\n"));
        return true;
    }
    if ((halfword & DataProcMask) == DataProcTest) {
        LOG(("data processing\n"));
        return true;
    }
    if ((halfword & MiscMask) == MiscTest) {
        LOG(("miscellaneous\n"));
        return true;
    }
    if ((halfword & SvcMask) == SvcTest) {
        LOG(("svc\n"));
        return true;
    }
    if ((halfword & PcRelLdrMask) == PcRelLdrTest) {
        LOG(("pc relative ldr\n"));
        return true;
    }
    if ((halfword & SpRelLdrStrMask) == SpRelLdrStrTest) {
        LOG(("sp relative ldr/str\n"));
        return true;
    }
    if ((halfword & SpRelAddMask) == SpRelAddTest) {
        LOG(("sp relative add\n"));
        return true;
    }
    if ((halfword & UncondBranchMask) == UncondBranchTest) {
        LOG(("unconditional branch\n"));
        // TODO: must validate target
        return true;
    }
    if ((halfword & CompareBranchMask) == CompareBranchTest) {
        LOG(("compare and branch\n"));
        // TODO: must validate target
        return true;
    }
    if ((halfword & CondBranchMask) == CondBranchTest) {
        LOG(("branchcc\n"));
        // TODO: must validate target
        return true;
    }
    if (halfword == Nop) {
        // 10111111 00000000     nop
        LOG(("nop\n"));
        return true;
    }
    LOG(("*********************************** invalid 16bit instruction: 0x%x\n", halfword));
    return false;
}

bool SvmProgram::conditionPassed(uint32_t instr)
{
    // eventually check cond regs to provide conditional execution
    return true;
}

// left shift
void SvmProgram::emulateLSLImm(uint16_t inst)
{
    if (!conditionPassed(inst))
        return;

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

void SvmProgram::emulateSVC(uint16_t instr)
{
    unsigned imm8 = instr & 0xFF;
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
bool SvmProgram::isValid32(uint32_t inst)
{
    if ((inst & StrMask) == StrTest) {
        LOG(("32bit str\n"));
        return true;
    }
    if ((inst & StrBhMask) == StrBhTest) {
        LOG(("32bit str[bh]\n"));
        return true;
    }
    if ((inst & LdrBhMask) == LdrBhTest) {
        LOG(("32bit ldr(s)[bh]\n"));
        return true;
    }
    if ((inst & LdrMask) == LdrTest) {
        LOG(("32bit ldr(s)[bh]\n"));
        return true;
    }
    if ((inst & MovWtMask) == MovWtTest) {
        LOG(("32bit mov[wt]\n"));
        return true;
    }
    LOG(("----------------------- invalid 32-bit instruction: 0x%x\n", inst));
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
