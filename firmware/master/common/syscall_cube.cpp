/*
 * Thundercracker Firmware -- Confidential, not for redistribution.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

/*
 * Syscalls for manipulating CubeSlot state.
 */

#include <sifteo/abi.h>
#include "svmmemory.h"
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
    if (SvmMemory::mapRAM(vbuf, sizeof *vbuf) && CubeSlots::validID(cid))
        CubeSlots::instances[cid].setVideoBuffer(vbuf);
}

uint32_t _SYS_getAccel(_SYSCubeID cid)
{
    if (CubeSlots::validID(cid))
        return CubeSlots::instances[cid].getAccelState().value;
    return 0;
}

uint32_t _SYS_getNeighbors(_SYSCubeID cid)
{
    if (CubeSlots::validID(cid))
        return NeighborSlot::instances[cid].getNeighborState().value;
    return 0xFFFFFFFF;
}

uint32_t _SYS_getTilt(_SYSCubeID cid)
{
    if (CubeSlots::validID(cid))
        return AccelState::instances[cid].getTiltState().value;
    return 0;
}

uint32_t _SYS_getShake(_SYSCubeID cid)
{
    if (CubeSlots::validID(cid))
        return AccelState::instances[cid].getShakeState();
    return 0;
}

uint32_t _SYS_isTouching(_SYSCubeID cid)
{
    if (CubeSlots::validID(cid))
        return CubeSlots::instances[cid].isTouching();
    return 0;
}

uint32_t _SYS_getBatteryV(_SYSCubeID cid)
{
    // XXX: Temporary for testing. Instead of raw battery voltage, we should
    //      be returning some cooked percentage-like value.
    if (CubeSlots::validID(cid))
        return CubeSlots::instances[cid].getRawBatteryV();
    return 0;
}

uint64_t _SYS_getCubeHWID(_SYSCubeID cid)
{
    if (CubeSlots::validID(cid))
        return CubeSlots::instances[cid].getHWID();
    return _SYS_INVALID_HWID;
}

}  // extern "C"
