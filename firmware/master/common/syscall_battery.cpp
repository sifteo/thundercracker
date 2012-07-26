/*
 * Thundercracker Firmware -- Confidential, not for redistribution.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

/*
 * Syscalls for battery state tracking.
 */

#include <sifteo/abi.h>
#include "svmmemory.h"
#include "svmruntime.h"
#include "cubeslots.h"
#include "cube.h"

extern "C" {


uint32_t _SYS_cubeBatteryLevel(_SYSCubeID cid)
{
    if (!CubeSlots::validID(cid)) {
        SvmRuntime::fault(F_SYSCALL_PARAM);
        return 0;
    }

    unsigned raw = CubeSlots::instances[cid].getRawBatteryV();

    /*
     * XXX: We should be doing our best to convert the raw reading to a linear
     *      "fuel gauge" representation. We don't have the best hardware for this,
     *      but we can make an attempt to track battery replacements and guess
     *      battery chemistry.
     */

    return raw * _SYS_BATTERY_MAX / 255;
}

uint32_t _SYS_sysBatteryLevel()
{
    /*
     * XXX: Implement me!
     */
    return 0xcccc;
}


}  // extern "C"
