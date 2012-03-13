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
    struct _SYSAccelState r = { 0 };
    if (CubeSlots::validID(cid))
        CubeSlots::instances[cid].getAccelState(&r);
    return r;
}

void _SYS_getNeighbors(_SYSCubeID cid, struct _SYSNeighborState *state) {
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

_SYSShakeState _SYS_getShake(_SYSCubeID cid)
{
    _SYSShakeState r = NOT_SHAKING;
    if (CubeSlots::validID(cid))
        AccelState::instances[cid].getShakeState(&r);
    return r;
}

void _SYS_getRawNeighbors(_SYSCubeID cid, uint8_t buf[4])
{
    // XXX: Temporary for testing/demoing
    if (SvmMemory::mapRAM(buf, sizeof buf) && CubeSlots::validID(cid))
        memcpy(buf, CubeSlots::instances[cid].getRawNeighbors(), 4);
}

uint8_t _SYS_isTouching(_SYSCubeID cid)
{
    if (CubeSlots::validID(cid)) {
        return CubeSlots::instances[cid].isTouching();
    }
    return 0;
}

uint16_t _SYS_getRawBatteryV(_SYSCubeID cid)
{
    // XXX: Temporary for testing. Master firmware should give cooked battery percentage.
    if (CubeSlots::validID(cid))
        return CubeSlots::instances[cid].getRawBatteryV();
    return 0;
}

void _SYS_getCubeHWID(_SYSCubeID cid, _SYSCubeHWID *hwid)
{
    // XXX: Maybe temporary?

    // XXX: Right now this is only guaranteed to be known after asset downloading, since
    //      there is no code yet to explicitly request it (via a flash reset)

    if (SvmMemory::mapRAM(hwid, sizeof hwid) && CubeSlots::validID(cid))
        *hwid = CubeSlots::instances[cid].getHWID();
}

}  // extern "C"
