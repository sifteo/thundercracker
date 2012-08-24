/*
 * Thundercracker Firmware -- Confidential, not for redistribution.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

/*
 * The validator algorithm isn't described in detail here, since there's a whole separate
 * document for that. See vm/doc/svm-validator.txt for way more info on what this validator
 * does and how it's designed.
 */

#include "svmvalidator.h"
#include "svm.h"
#include "macros.h"
#include "svmcpu.h"

using namespace Svm;
namespace SvmValidator {

#ifdef SVMVALIDATOR_TRACE
#   define VALIDATOR_LOG(_x)    LOG(_x)
#else
#   define VALIDATOR_LOG(_x)
#endif


// Rotate by 16 bits
static ALWAYS_INLINE uint32_t rot16(uint32_t word) {
    return (word << 16) | (word >> 16);
}

/*
 * This data type represents the concept of a bundle's "maximum successor".
 * I.e. -1 if the bundle is a terminator (always valid), or an arbitrary
 * large number if the bundle is totally invalid.
 *
 * The value used for 'INVALID' here doesn't matter at all, so long as it's
 * at least as large as the number of bundles per block. We pick something that's small
 * enough to fit in an ARM instruction immediate.
 */

typedef int MaxSuccessor;
static const MaxSuccessor TERMINATOR = -1;
static const MaxSuccessor INVALID = Svm::BUNDLES_PER_BLOCK;

/*
 * Determinte the MaxSuccessor for one 32-bit instruction.
 *
 * All 32-bit instructions are easy. All current 32-bit instructions for us
 * have no effect on control flow. If they're valid at all, they always
 * have exactly one successor.
 *
 * Note that our halfwords in 'word' are in little endian order, whereas
 * the masks are written in "data sheet order", where the individual words
 * are stored in little-endian format but the two words are ordered so
 * they read left-to-right. To avoid any runtime swapping, we swap the masks
 * instead of swapping the word.
 *
 * These tests should be in roughly descending order of instruction frequency.
 */
static ALWAYS_INLINE MaxSuccessor decode32(uint32_t word, int bundleIndex)
{
    return ( 
        (word & rot16(LdrMask))   == rot16(LdrTest)   ||
        (word & rot16(StrMask))   == rot16(StrTest)   ||
        (word & rot16(LdrBhMask)) == rot16(LdrBhTest) ||
        (word & rot16(StrBhMask)) == rot16(StrBhTest) ||
        (word & rot16(MovWtMask)) == rot16(MovWtTest) ||
        (word & rot16(ClzMask))   == rot16(ClzTest)   ||
        (word & rot16(DivMask))   == rot16(DivTest)
    ) ? (bundleIndex + 1) : INVALID;
}

/*
 * Convert a branch target, as a "program counter" reg_t, to a MaxSuccessor.
 * This checks the branch's bundle alignment and ensures it isn't out of bounds.
 */
static ALWAYS_INLINE MaxSuccessor checkBranch(reg_t target)
{
    STATIC_ASSERT(Svm::BUNDLE_SIZE == 4);

    // Target must be bundle-aligned, regardless of the source's alignment
    if (target & 3)
        return INVALID;

    // Must be unsigned comparison
    if (target >= Svm::BLOCK_SIZE)
        return INVALID;

    return target >> 2;
}

/*
 * Decode an SVC. Returns a MaxSuccessor describing the next bundle-aligned
 * successor, if any, and a boolean indicating whether this instruction
 * will chain flow control to the next instruction.
 *
 * Most SVCs do not affect flow control- but some of them, for our purposes
 * at least, act as terminators. Any SVC which transfers flow control is a
 * terminator for the static validator, since it embodies a guarantee that
 * the actual branch target will be validated at runtime later.
 *
 * This is even the case for 'call' instructions, since the return address
 * must be validated at runtime anyway due to having been stored in untrusted RAM.
 * (This has the pleasant side-effect of allowing the compiler to optimize
 * for cases when a called function never returns.)
 *
 * Any syscall which is guaranteed to never return is also a terminator.
 */
static ALWAYS_INLINE bool decodeSVC(uint32_t word, int bundleIndex,
    const uint32_t *block, MaxSuccessor &succ)
{
    unsigned imm8 = word & 0xFF;

    if (imm8 & 0x80) {
        // Direct SVCs

        // No bundle-aligned successors
        succ = TERMINATOR;

        // Some direct SVCs don't pass control to the next instruction
        return !(
            (imm8 == 0x80           ||  // _SYS_abort()
            (imm8 & 0xF0) == 0xF0)      // Call rN / Tail-call rN
        );
    }

    if (imm8 == 0) {
        // Return
        succ = TERMINATOR;
        return false;
    }

    /*
     * Indirect SVC. Load the argument, and validate it.
     * Run some more mask tests to see if this is a terminator.
     */

    STATIC_ASSERT(Svm::BUNDLE_SIZE == 4);
    if (imm8 >= Svm::BLOCK_SIZE / sizeof(uint32_t)) {
        // Address out of range!
        succ = INVALID;
        return false;
    }

    uint32_t literal = block[imm8];

    // Indirect SVC terminator instructions.
    // These tests should be in roughly descending order of instruction frequency.
    if (
        (literal & SvcBranchMask)   == SvcBranchTest    ||
        (literal & CallMask)        == CallTest         ||
        (literal & TailCallMask)    == TailCallTest     ||
        (literal & TailSyscallMask) == TailSyscallTest  ||
        (literal & SvcExitMask)     == SvcExitTest      )
    {
        succ = TERMINATOR;
        return false;
    }

    // Transfer control to the next instruction
    succ = TERMINATOR;
    return true;
}

/*
 * Decode a 16-bit instruction. Returns a MaxSuccessor describing the next
 * bundle-aligned successor, if any, and a boolean indicating whether
 * this instruction will chain flow control to the next instruction.
 *
 * The upper 16 bits of 'word' are ignored.
 *
 * These tests should be in roughly descending order of instruction frequency.
 */
static ALWAYS_INLINE bool decode16(uint32_t word, int bundleIndex,
    unsigned bundleOffset, const uint32_t *block, MaxSuccessor &succ)
{
    // Easy cases: No branching or terminators, no arguments to validate.
    if (
        (word & AluMask)         == AluTest          ||
        (word & DataProcMask)    == DataProcTest     ||
        (word & MiscMask)        == MiscTest         ||
        (word & MovMask)         == MovTest          ||
        (word & PcRelLdrMask)    == PcRelLdrTest     ||
        (word & SpRelLdrStrMask) == SpRelLdrStrTest  ||
        (word & SpRelAddMask)    == SpRelAddTest     ||
        (word & 0xFFFF)          == Nop              )
    {
        // Transfer control to the next instruction
        succ = TERMINATOR;
        return true;
    }

    if ((word & SvcMask) == SvcTest)
        return decodeSVC(word, bundleIndex, block, succ);

    /*
     * All other instructions are branches.
     *
     * For branch target calculations, fabricate a "program counter"
     * as it would appear while executing this instruction. Note that
     * reg_t is unsigned, so underflows will wrap around and appear
     * to be past the end of the block.
     */
    reg_t pc = ((bundleIndex << 2) | bundleOffset) + 2;

    // Unconditional branch: exactly one successor
    if ((word & UncondBranchMask) == UncondBranchTest) {
        succ = checkBranch(branchTargetB(word, pc));
        return false;
    }

    // Normal conditional jumps
    if ((word & CondBranchMask) == CondBranchTest) {
        succ = checkBranch(passedBranchTargetCondB(word, pc));
        return true;
    }

    // CBZ/CBNZ conditional jumps
    if ((word & CompareBranchMask) == CompareBranchTest) {
        succ = checkBranch(passedBranchTargetCBZ_CBNZ(word, pc));
        return true;
    }

    // Invalid instruction!
    succ = INVALID;
    return false;
}

/*
 * Determine the MaxSuccessor for one 32-bit bundle.
 *
 * The provided words are candidate 32-bit instruction bundles, in little
 * endian byte order. This means the first 16-bit instruction is in the
 * low word, and the second 16-bit instruction is in the high word.
 * Or, if this is a 32-bit instruction, the words are in opposite order
 * from the masks in svm.h.
 */
static ALWAYS_INLINE MaxSuccessor decodeBundle(const uint32_t *block, int index)
{
    uint32_t word = block[index];

    // Is this a 32-bit instruction?
    if (Svm::instructionSize(word) == Svm::InstrBits32)
        return decode32(word, index);

    /*
     * Otherwise, this can be up to two 16-bit instructions. Note that
     * the first instruction (in the low half-word) may be a terminator,
     * in which case the high half-word does not matter (It may be used
     * by the compiler for inline data or any other purpose.)
     *
     * So, this is split into two steps, each of which is handled by
     * an inlined (and slightly specialized) copy of decode16().
     */

    MaxSuccessor s1;
    if (!decode16(word, index, 0, block, s1)) {
        // The next instruction won't be reached
        return s1;
    }

    MaxSuccessor s2;
    if (decode16(word >> 16, index, 2, block, s2)) {
        // Fall through to the next bundle maybe
        s2 = MAX(index + 1, s2);
    }

    return MAX(s1, s2);
}

/**
 * Top-level public entry point for the validator:
 *
 * Given a whole block of code, determine how many bundles, if any, from
 * the start of the block are usable as valid branch targets. Any bundle
 * which falls within this set is guaranteed to be valid itself and only
 * transfer control to other valid bundles.
 *
 * Everything above gets inlined into this function.
 */
unsigned findValidBundles(const uint32_t *block)
{
    MaxSuccessor upperBound = Svm::BUNDLES_PER_BLOCK;
    MaxSuccessor sMax = -1;

    VALIDATOR_LOG(("-------------------- Validating Block --------------------\n"));

    for (int index = upperBound - 1; index >= 0; --index) {
        MaxSuccessor sBundle = decodeBundle(block, index);

        VALIDATOR_LOG(("[%2d] %04x-%04x %d\n",
            index, block[index] & 0xffff, block[index] >> 16, sBundle));

        if (sBundle >= upperBound) {
            /*
             * Definitely invalid! The last valid bundle must come before this.
             * Reset sMax, since we know we'll need to throw out the results from
             * this iteration so far.
             */

            sMax = -1;
            upperBound = index;
        } else {
            sMax = MAX(sMax, sBundle);
        }

        ASSERT(sMax < upperBound);
    }

    VALIDATOR_LOG(("----- Result: %d\n", upperBound));

    return upperBound;
}


}  // end namespace SvmValidator
