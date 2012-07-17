/*
 * Thundercracker Firmware -- Confidential, not for redistribution.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

/*
 * Syscalls for manipulating CubeSlot state.
 */

#include <sifteo/abi.h>
#include "svmmemory.h"
#include "svmruntime.h"
#include "cubeslots.h"
#include "cube.h"
#include "neighbors.h"
#include "accel.h"

extern "C" {

void _SYS_enableCubes(_SYSCubeIDVector cv)
{
    CubeSlots::enableCubes(CubeSlots::truncateVector(cv));
}

void _SYS_disableCubes(_SYSCubeIDVector cv)
{
    CubeSlots::disableCubes(CubeSlots::truncateVector(cv));
}

void _SYS_setVideoBuffer(_SYSCubeID cid, struct _SYSVideoBuffer *vbuf)
{
    if (!isAligned(vbuf))
        return SvmRuntime::fault(F_SYSCALL_ADDR_ALIGN);
    if (!SvmMemory::mapRAM(vbuf, sizeof *vbuf, true))
        return SvmRuntime::fault(F_SYSCALL_ADDRESS);
    if (!CubeSlots::validID(cid))
        return SvmRuntime::fault(F_SYSCALL_PARAM);

    CubeSlots::instances[cid].setVideoBuffer(vbuf);
}

uint32_t _SYS_getAccel(_SYSCubeID cid)
{
    if (!CubeSlots::validID(cid)) {
        SvmRuntime::fault(F_SYSCALL_PARAM);
        return 0;
    }

    return CubeSlots::instances[cid].getAccelState().value;
}

uint32_t _SYS_getNeighbors(_SYSCubeID cid)
{
    if (!CubeSlots::validID(cid)) {
        SvmRuntime::fault(F_SYSCALL_PARAM);
        return 0xFFFFFFFF;
    }

    return NeighborSlot::instances[cid].getNeighborState().value;
}

uint32_t _SYS_getTilt(_SYSCubeID cid)
{
    if (!CubeSlots::validID(cid)) {
        SvmRuntime::fault(F_SYSCALL_PARAM);
        return 0;
    }

    return AccelState::instances[cid].getTiltState().value;
}

uint32_t _SYS_getShake(_SYSCubeID cid)
{
    if (!CubeSlots::validID(cid)) {
        SvmRuntime::fault(F_SYSCALL_PARAM);
        return 0;
    }

    return AccelState::instances[cid].getShakeState();
}

uint32_t _SYS_isTouching(_SYSCubeID cid)
{
    if (!CubeSlots::validID(cid)) {
        SvmRuntime::fault(F_SYSCALL_PARAM);
        return 0;
    }

    return CubeSlots::instances[cid].isTouching();
}

uint32_t _SYS_getBatteryV(_SYSCubeID cid)
{
    if (!CubeSlots::validID(cid)) {
        SvmRuntime::fault(F_SYSCALL_PARAM);
        return 0;
    }

    // XXX: Temporary for testing. Instead of raw battery voltage, we should
    //      be returning some cooked percentage-like value.
    return CubeSlots::instances[cid].getRawBatteryV();
}

uint64_t _SYS_getCubeHWID(_SYSCubeID cid)
{
    if (!CubeSlots::validID(cid)) {
        SvmRuntime::fault(F_SYSCALL_PARAM);
        return _SYS_INVALID_HWID;
    }

    return CubeSlots::instances[cid].getHWID();
}

}  // extern "C"
