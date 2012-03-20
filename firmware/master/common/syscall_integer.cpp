/*
 * Thundercracker Firmware -- Confidential, not for redistribution.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

/*
 * Syscalls for integer math operations.
 */

#include <math.h>
#include <sifteo/abi.h>
#include "svmmemory.h"
#include "prng.h"

extern "C" {

uint32_t _SYS_fetch_and_or_4(uint32_t *p, uint32_t t) {
    if (SvmMemory::mapRAM(p, sizeof *p))
        return __sync_fetch_and_or(p, t);
    return 0;
}

uint32_t _SYS_fetch_and_xor_4(uint32_t *p, uint32_t t) {
    if (SvmMemory::mapRAM(p, sizeof *p))
        return __sync_fetch_and_xor(p, t);
    return 0;
}

uint32_t _SYS_fetch_and_nand_4(uint32_t *p, uint32_t t) {
    if (SvmMemory::mapRAM(p, sizeof *p))
        return __sync_fetch_and_nand(p, t); // this issues warnings on GCC > 4.4, just ignoring for now
    return 0;
}

uint32_t _SYS_fetch_and_and_4(uint32_t *p, uint32_t t) {
    if (SvmMemory::mapRAM(p, sizeof *p))
        return __sync_fetch_and_and(p, t);
    return 0;
}

uint64_t _SYS_shl_i64(uint32_t aL, uint32_t aH, uint32_t b) {
    uint64_t a = aL | (uint64_t)aH << 32;
    return a << b;
}

uint64_t _SYS_srl_i64(uint32_t aL, uint32_t aH, uint32_t b) {
    uint64_t a = aL | (uint64_t)aH << 32;
    return a >> b;
}

int64_t _SYS_sra_i64(uint32_t aL, uint32_t aH, uint32_t b) {
    int64_t a = aL | (uint64_t)aH << 32;
    return a >> b;
}

uint64_t _SYS_mul_i64(uint32_t aL, uint32_t aH, uint32_t bL, uint32_t bH) {
    uint64_t a = aL | (uint64_t)aH << 32;
    uint64_t b = bL | (uint64_t)bH << 32;
    return a * b;
}

int64_t _SYS_sdiv_i64(uint32_t aL, uint32_t aH, uint32_t bL, uint32_t bH) {
    int64_t a = aL | (uint64_t)aH << 32;
    int64_t b = bL | (uint64_t)bH << 32;
    return a / b;
}

uint64_t _SYS_udiv_i64(uint32_t aL, uint32_t aH, uint32_t bL, uint32_t bH) {
    uint64_t a = aL | (uint64_t)aH << 32;
    uint64_t b = bL | (uint64_t)bH << 32;
    return a / b;
}

int64_t _SYS_srem_i64(uint32_t aL, uint32_t aH, uint32_t bL, uint32_t bH) {
    int64_t a = aL | (uint64_t)aH << 32;
    int64_t b = bL | (uint64_t)bH << 32;
    return a % b;
}

uint64_t _SYS_urem_i64(uint32_t aL, uint32_t aH, uint32_t bL, uint32_t bH) {
    uint64_t a = aL | (uint64_t)aH << 32;
    uint64_t b = bL | (uint64_t)bH << 32;
    return a % b;
}

void _SYS_prng_init(struct _SYSPseudoRandomState *state, uint32_t seed)
{
    if (SvmMemory::mapRAM(state, sizeof *state))
        PRNG::init(state, seed);
}

uint32_t _SYS_prng_value(struct _SYSPseudoRandomState *state)
{
    if (SvmMemory::mapRAM(state, sizeof *state))
        return PRNG::value(state);
    return 0;
}

uint32_t _SYS_prng_valueBounded(struct _SYSPseudoRandomState *state, uint32_t limit)
{
    if (SvmMemory::mapRAM(state, sizeof *state))
        return PRNG::valueBounded(state, limit);
    return 0;
}

}  // extern "C"
