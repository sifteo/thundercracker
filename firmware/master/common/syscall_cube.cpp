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
#include "neighborslot.h"
#include "accel.h"

extern "C" {

uint32_t _SYS_getConnectedCubes()
{
    return CubeSlots::userConnected;
}

void _SYS_setCubeRange(uint32_t minimum, uint32_t maximum)
{
    if (minimum > _SYS_NUM_CUBE_SLOTS ||
        maximum > _SYS_NUM_CUBE_SLOTS ||
        minimum > maximum)
        return SvmRuntime::fault(F_SYSCALL_PARAM);
    
    CubeSlots::setCubeRange(minimum, maximum);
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

void _SYS_setMotionBuffer(_SYSCubeID cid, _SYSMotionBuffer *mbuf)
{
    if (!isAligned(mbuf))
        return SvmRuntime::fault(F_SYSCALL_ADDR_ALIGN);

    /*
     * Note: Because the buffer's actual size is dynamic, we enforce
     *       that even a maximally-large motion buffer is mappable
     *       at this address before we'll accept the pointer.
     */
    if (!SvmMemory::mapRAM(mbuf, sizeof *mbuf, true))
        return SvmRuntime::fault(F_SYSCALL_ADDRESS);

    if (!CubeSlots::validID(cid))
        return SvmRuntime::fault(F_SYSCALL_PARAM);

    // XXX: Do stuff
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

uint64_t _SYS_getCubeHWID(_SYSCubeID cid)
{
    if (!CubeSlots::validID(cid)) {
        SvmRuntime::fault(F_SYSCALL_PARAM);
        return _SYS_INVALID_HWID;
    }

    return CubeSlots::instances[cid].getHWID();
}

}  // extern "C"
