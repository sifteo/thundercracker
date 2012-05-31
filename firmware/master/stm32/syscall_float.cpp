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


}  // extern "C"
