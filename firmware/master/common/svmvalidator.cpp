#include "svmvalidator.h"
#include "svm.h"
#include "sifteo.h"

using namespace Svm;

unsigned SvmValidator::validBytes(void *block, unsigned lenInBytes)
{
    unsigned numValidBytes = 0;
    uint16_t *b = static_cast<uint16_t*>(block);

    uint16_t *end = b + lenInBytes;
    while (b < end) {
        uint16_t instr = *b++;
        if (instructionSize(instr) == InstrBits16) {
            if (!isValid16(instr))
                break;
            numValidBytes++;
        }
        else {
            uint16_t instrLow = *b++;
            if (!isValid32(instr << 16 | instrLow))
                break;
            numValidBytes += 2;
        }
    }
    LOG(("validation complete, valid: %d\n", numValidBytes));
    return numValidBytes;
}

bool SvmValidator::addressIsValid(uintptr_t address)
{
    return true;
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
bool SvmValidator::isValid16(uint16_t instr)
{
    if ((instr & AluMask) == AluTest) {
        return true;
    }
    if ((instr & DataProcMask) == DataProcTest) {
        return true;
    }
    if ((instr & MiscMask) == MiscTest) {
        return true;
    }
    if ((instr & SvcMask) == SvcTest) {
        return true;
    }
    if ((instr & PcRelLdrMask) == PcRelLdrTest) {
        return true;
    }
    if ((instr & SpRelLdrStrMask) == SpRelLdrStrTest) {
        return true;
    }
    if ((instr & SpRelAddMask) == SpRelAddTest) {
        return true;
    }
    if ((instr & UncondBranchMask) == UncondBranchTest) {
        // TODO: must validate target
        return true;
    }
    if ((instr & CompareBranchMask) == CompareBranchTest) {
        // TODO: must validate target
        return true;
    }
    if ((instr & CondBranchMask) == CondBranchTest) {
        // TODO: must validate target
        return true;
    }
    if (instr == Nop) {
        // 10111111 00000000     nop
        return true;
    }
    LOG(("*********************************** invalid 16bit instruction: 0x%x\n", instr));
    return false;
}

/*
Allowed 32-bit instruction encodings:

  11111000 11001001, 0xxxxxxx xxxxxxxx      str         r0-r7, [r9, #imm12]
  11111000 10x01001, 0xxxxxxx xxxxxxxx      str[bh]     r0-r7, [r9, #imm12]

  1111100x 10x1100x, 0xxxxxxx xxxxxxxx      ldr(s)[bh]  r0-r7, [r8-9, #imm12]
  11111000 1101100x, 0xxxxxxx xxxxxxxx      ldr         r0-r7, [r8-9, #imm12]

  11110x10 x100xxxx, 0xxx0xxx xxxxxxxx      mov[wt]     r0-r7, #imm16
    
  11111011 10x10xxx, 11110xxx 11110xxx      [su]div     r0-r7, r0-r7, r0-r7
*/
bool SvmValidator::isValid32(uint32_t instr)
{
    if ((instr & StrMask) == StrTest) {
        return true;
    }
    if ((instr & StrBhMask) == StrBhTest) {
        return true;
    }
    if ((instr & LdrBhMask) == LdrBhTest) {
        return true;
    }
    if ((instr & LdrMask) == LdrTest) {
        return true;
    }
    if ((instr & MovWtMask) == MovWtTest) {
        return true;
    }
    if ((instr & DivMask) == DivTest) {
        return true;
    }
    LOG(("----------------------- invalid 32-bit instruction: 0x%x\n", instr));
    return false;
}
