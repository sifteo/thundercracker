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

void _SYS_solicitCubes(_SYSCubeID min, _SYSCubeID max)
{
    CubeSlots::solicitCubes(min, max);
}

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

void _SYS_loadAssets(_SYSCubeID cid, _SYSAssetGroup *group)
{
    if (SvmMemory::mapRAM(group, sizeof *group) && CubeSlots::validID(cid))
        CubeSlots::instances[cid].loadAssets(group);
}

struct _SYSAccelState _SYS_getAccel(_SYSCubeID cid)
{
    struct _SYSAccelState r = { 0, 0, 0 };
    if (CubeSlots::validID(cid))
        CubeSlots::instances[cid].getAccelState(&r);
    return r;
}

void _SYS_getNeighbors(_SYSCubeID cid, struct _SYSNeighborState *state)
{
    if (SvmMemory::mapRAM(state, sizeof *state) && CubeSlots::validID(cid)) {
        NeighborSlot::instances[cid].getNeighborState(state);
    }
}

struct _SYSTiltState _SYS_getTilt(_SYSCubeID cid)
{
    struct _SYSTiltState r = { 0 };
    if (CubeSlots::validID(cid))
        AccelState::instances[cid].getTiltState(&r);
    return r;
}

uint32_t _SYS_getShake(_SYSCubeID cid)
{
    _SYSShakeState r = NOT_SHAKING;
    if (CubeSlots::validID(cid))
        AccelState::instances[cid].getShakeState(&r);
    return r;
}

uint32_t _SYS_isTouching(_SYSCubeID cid)
{
    if (CubeSlots::validID(cid)) {
        return CubeSlots::instances[cid].isTouching();
    }
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
