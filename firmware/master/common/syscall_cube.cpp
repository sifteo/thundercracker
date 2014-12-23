/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * Thundercracker firmware
 *
 * Copyright <c> 2012 Sifteo, Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
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
#include "cubeconnector.h"

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

    CubeSlots::instances[cid].setMotionBuffer(mbuf);
}

void _SYS_motion_integrate(const struct _SYSMotionBuffer *mbuf, unsigned duration, struct _SYSInt3 *result)
{
    if (!isAligned(mbuf))
        return SvmRuntime::fault(F_SYSCALL_ADDR_ALIGN);

    // Even though these are read-only, we don't bother allowing them to be in flash memory.
    // Also, the same note about sizing applies from _SYS_setMotionBuffer().
    if (!SvmMemory::mapRAM(mbuf, sizeof *mbuf))
        return SvmRuntime::fault(F_SYSCALL_ADDRESS);

    if (!isAligned(result))
        return SvmRuntime::fault(F_SYSCALL_ADDR_ALIGN);

    if (!SvmMemory::mapRAM(result, sizeof *result))
        return SvmRuntime::fault(F_SYSCALL_ADDRESS);

    MotionUtil::integrate(mbuf, duration, result);
}

void _SYS_motion_median(const struct _SYSMotionBuffer *mbuf, unsigned duration, struct _SYSMotionMedian *result)
{
    if (!isAligned(mbuf))
        return SvmRuntime::fault(F_SYSCALL_ADDR_ALIGN);

    // Even though these are read-only, we don't bother allowing them to be in flash memory.
    // Also, the same note about sizing applies from _SYS_setMotionBuffer().
    if (!SvmMemory::mapRAM(mbuf, sizeof *mbuf))
        return SvmRuntime::fault(F_SYSCALL_ADDRESS);

    // Note: _SYSMotionMedian does NOT require word alignment.
    if (!SvmMemory::mapRAM(result, sizeof *result))
        return SvmRuntime::fault(F_SYSCALL_ADDRESS);

    MotionUtil::median(mbuf, duration, result);
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

void _SYS_unpair(_SYSCubeID cid)
{
    if (!CubeSlots::validID(cid)) {
        SvmRuntime::fault(F_SYSCALL_PARAM);
        return;
    }

    CubeConnector::unpair(cid);
    Atomic::Or(CubeSlots::sendShutdown, Intrinsic::LZ(cid));
}

}  // extern "C"
