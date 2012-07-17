/*
 * Thundercracker Firmware -- Confidential, not for redistribution.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

/*
 * Syscalls for atomic operations.
 */

#include <sifteo/abi.h>
#include "svmmemory.h"
#include "svmruntime.h"

extern "C" {

uint32_t _SYS_fetch_and_or_4(uint32_t *p, uint32_t t) {
    if (!isAligned(p)) {
        SvmRuntime::fault(F_SYSCALL_ADDR_ALIGN);
        return 0;
    }
    if (!SvmMemory::mapRAM(p, sizeof *p)) {
        SvmRuntime::fault(F_SYSCALL_ADDRESS);
        return 0;
    }

    return __sync_fetch_and_or(p, t);
}

uint32_t _SYS_fetch_and_xor_4(uint32_t *p, uint32_t t) {
    if (!isAligned(p)) {
        SvmRuntime::fault(F_SYSCALL_ADDR_ALIGN);
        return 0;
    }
    if (!SvmMemory::mapRAM(p, sizeof *p)) {
        SvmRuntime::fault(F_SYSCALL_ADDRESS);
        return 0;
    }

    return __sync_fetch_and_xor(p, t);
}

uint32_t _SYS_fetch_and_and_4(uint32_t *p, uint32_t t) {
    if (!isAligned(p)) {
        SvmRuntime::fault(F_SYSCALL_ADDR_ALIGN);
        return 0;
    }
    if (!SvmMemory::mapRAM(p, sizeof *p)) {
        SvmRuntime::fault(F_SYSCALL_ADDRESS);
        return 0;
    }

    return __sync_fetch_and_and(p, t);
}

}  // extern "C"
