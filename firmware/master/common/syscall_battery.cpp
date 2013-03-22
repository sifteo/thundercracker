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
#include "batterylevel.h"
#ifndef SIFTEO_SIMULATOR
#include "powermanager.h"
#endif

extern "C" {


uint32_t _SYS_cubeBatteryLevel(_SYSCubeID cid)
{
    if (!CubeSlots::validID(cid)) {
        SvmRuntime::fault(F_SYSCALL_PARAM);
        return 0;
    }

    return CubeSlots::instances[cid].getScaledBatteryV();
}

uint32_t _SYS_sysBatteryLevel()
{
    /*
     * XXX: Same as above - we should be tracking battery levels over time
     *      and doing a better job of creating a linear representation.
     */

    return BatteryLevel::scaled();
}


}  // extern "C"
