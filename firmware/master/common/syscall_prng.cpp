/*
 * Thundercracker Firmware -- Confidential, not for redistribution.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

/*
 * Syscalls for our pseudorandom number generator.
 */

#include <sifteo/abi.h>
#include "svmmemory.h"
#include "svmruntime.h"
#include "prng.h"

extern "C" {

void _SYS_prng_init(struct _SYSPseudoRandomState *state, uint32_t seed)
{
    if (!SvmMemory::mapRAM(state, sizeof *state))
        return SvmRuntime::fault(F_SYSCALL_ADDRESS);

    PRNG::init(state, seed);
}

uint32_t _SYS_prng_value(struct _SYSPseudoRandomState *state)
{
    if (!SvmMemory::mapRAM(state, sizeof *state)) {
        SvmRuntime::fault(F_SYSCALL_ADDRESS);
        return 0;
    }

    return PRNG::value(state);
}

uint32_t _SYS_prng_valueBounded(struct _SYSPseudoRandomState *state, uint32_t limit)
{
    if (!SvmMemory::mapRAM(state, sizeof *state)) {
        SvmRuntime::fault(F_SYSCALL_ADDRESS);
        return 0;
    }

    return PRNG::valueBounded(state, limit);
}

}  // extern "C"
