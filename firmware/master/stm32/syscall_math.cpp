/*
 * Thundercracker Firmware -- Confidential, not for redistribution.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

/*
 * Syscalls for software floating point support.
 *
 * Note that this file only implements operations which don't directly
 * map to functions in the software FP library.
 */

#include <math.h>
#include <sifteo/abi.h>
#include "svmmemory.h"

extern "C" {


void _SYS_sincosf(uint32_t x, float *sinOut, float *cosOut)
{
    float fX = reinterpret_cast<float&>(x);
    float dummy;

    if (!SvmMemory::mapRAM(sinOut, sizeof *sinOut))
        sinOut = &dummy;
    if (!SvmMemory::mapRAM(cosOut, sizeof *cosOut))
        cosOut = &dummy;

    sincosf(fX, sinOut, cosOut);
}

__attribute__((naked, noreturn))
uint64_t _SYS_mul_i64(uint32_t aL, uint32_t aH, uint32_t bL, uint32_t bH)
{
    /*
     * This is faster than __aeabi_lmul currently, since we know the
     * processor supports the umull instruction. Use umull to do a 32x32=64
     * multiply on the low words, then add (aH*bL)+(aL*bH) to the high word.
     *
     * This is the same instruction sequence that GCC would generate for
     * a plain 64-bit multiply with this same function signature, but the
     * split high and low portions confuse the optimizer enough that it's
     * better to just write out the assembly for this function ourselves.
     */

    asm volatile (
        "muls   r1, r2                  \n\t"
        "mla    r3, r0, r3, r1          \n\t"
        "umull  r0, r1, r2, r0          \n\t"
        "adds   r3, r3, r1              \n\t"
        "mov    r1, r3                  \n\t"
        "bx     lr                      \n\t"
    );
}

__attribute__((naked, noreturn))
int64_t _SYS_srem_i64(uint32_t aL, uint32_t aH, uint32_t bL, uint32_t bH)
{
    /*
     * Signed 64-bit remainder.
     *
     * EABI only provides a combined divide/remainder operation.
     * Call that, and move the remainder portion into r1:r0.
     */

    asm volatile (
        "push   {lr}                    \n\t"
        "bl     __aeabi_ldivmod         \n\t"
        "mov    r0, r2                  \n\t"
        "mov    r1, r3                  \n\t"
        "pop    {pc}                    \n\t"
    );
}

__attribute__((naked, noreturn))
uint64_t _SYS_urem_i64(uint32_t aL, uint32_t aH, uint32_t bL, uint32_t bH)
{
    /*
     * Unsigned 64-bit remainder.
     *
     * EABI only provides a combined divide/remainder operation.
     * Call that, and move the remainder portion into r1:r0.
     */

    asm volatile (
        "push   {lr}                    \n\t"
        "bl     __aeabi_uldivmod        \n\t"
        "mov    r0, r2                  \n\t"
        "mov    r1, r3                  \n\t"
        "pop    {pc}                    \n\t"
    );
}


}  // extern "C"
