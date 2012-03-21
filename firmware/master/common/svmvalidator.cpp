/*
 * Thundercracker Firmware -- Confidential, not for redistribution.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */
 
#include "svmvalidator.h"
#include "svm.h"

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
            numValidBytes += 2;
        }
        else {
            uint16_t instrLow = *b++;
            if (!isValid32(instr << 16 | instrLow))
                break;
            numValidBytes += 4;
        }
    }

    /*
     * XXX: MUST also ensure that the last valid instruction is a terminator.
     *      (Branch, long branch, tail call, tail syscall) Otherwise, execution
     *      can run past the end of valid instructions into invalid territory.
     *      We can do this by allowing valid non-terminators to make forward
     *      progress through the block, but only saving a copy of the
     *      numValidBytes result when we hit a terminator instruction.
     */
    
    /*
     * XXX: We also must validate local branch targets! This could be tricky,
     *      since the result of branch target validation affects which
     *      instructions are valid, which affects which branch destinations
     *      are valid. Reverse branches can always be validated in one pass,
     *      but we'll likely need a multi-pass algorithm to account for
     *      forward branches.
     *
     *      Worst case, if there's no way to do this that's fast and
     *      guaranteed to converge, we can always include an explicit
     *      compile-time marker which indicates the end of code and beginning
     *      of data, at a small space penalty per-block.
     */

#ifdef SVM_TRACE
    LOG(("VALIDATOR: complete, 0x%03x bytes valid\n", numValidBytes));
#endif

    return numValidBytes;
}

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
    if ((instr & MovMask) == MovTest) {
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

#ifdef SVM_TRACE
    LOG(("VALIDATOR: invalid 16bit instruction: 0x%x\n", instr));
#endif

    return false;
}

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

#ifdef SVM_TRACE
    LOG(("VALIDATOR: invalid 32-bit instruction: 0x%x\n", instr));
#endif

    return false;
}
