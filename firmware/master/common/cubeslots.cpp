/*
 * Thundercracker Firmware -- Confidential, not for redistribution.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#include "cubeslots.h"
#include "cube.h"
#include "neighbors.h"
#include "machine.h"


CubeSlot CubeSlots::instances[_SYS_NUM_CUBE_SLOTS];

_SYSCubeIDVector CubeSlots::vecEnabled = 0;
_SYSCubeIDVector CubeSlots::vecConnected = 0;
_SYSCubeIDVector CubeSlots::flashResetWait = 0;
_SYSCubeIDVector CubeSlots::flashResetSent = 0;
_SYSCubeIDVector CubeSlots::flashACKValid = 0;
_SYSCubeIDVector CubeSlots::frameACKValid = 0;
_SYSCubeIDVector CubeSlots::neighborACKValid = 0;
_SYSCubeIDVector CubeSlots::expectStaleACK = 0;
_SYSCubeIDVector CubeSlots::flashAddrPending = 0;
_SYSCubeIDVector CubeSlots::hwidValid = 0;

_SYSCubeID CubeSlots::minCubes = 0;
_SYSCubeID CubeSlots::maxCubes = _SYS_NUM_CUBE_SLOTS;

_SYSAssetLoader *CubeSlots::assetLoader = 0;


void CubeSlots::solicitCubes(_SYSCubeID min, _SYSCubeID max)
{
	minCubes = min;
	maxCubes = max;
}

void CubeSlots::enableCubes(_SYSCubeIDVector cv)
{
    NeighborSlot::resetSlots(cv);
    Atomic::Or(CubeSlots::vecEnabled, cv);

    // Expect that the cube's radio may have one old ACK packet buffered. Ignore this packet.
    Atomic::Or(CubeSlots::expectStaleACK, cv);
}

void CubeSlots::disableCubes(_SYSCubeIDVector cv)
{
    Atomic::And(CubeSlots::vecEnabled, ~cv);

    Atomic::And(CubeSlots::flashResetWait, ~cv);
    Atomic::And(CubeSlots::flashResetSent, ~cv);
    Atomic::And(CubeSlots::flashACKValid, ~cv);
    Atomic::And(CubeSlots::neighborACKValid, ~cv);
    Atomic::And(CubeSlots::hwidValid, ~cv);

    NeighborSlot::resetSlots(cv);
    NeighborSlot::resetPairs(cv);

    resetCubeState(cv);

    // TODO: if any of the cubes in cv are currently part of a
    // neighbor-pair with any cubes that are still active, those
    // active cubes neeed to remove their now-defunct neighbors
}

void CubeSlots::connectCubes(_SYSCubeIDVector cv)
{
    Atomic::Or(CubeSlots::vecConnected, cv);
}

void CubeSlots::disconnectCubes(_SYSCubeIDVector cv)
{
    Atomic::And(CubeSlots::vecConnected, ~cv);
}

void CubeSlots::resetCubeState(_SYSCubeIDVector cv)
{
    /*
     * Calls resetState() on all cubes in 'cv'
     */

    while (cv) {
        _SYSCubeID id = Intrinsic::CLZ(cv);
        cv ^= Intrinsic::LZ(id);
        instances[id].resetState();
    }
}

void CubeSlots::paintCubes(_SYSCubeIDVector cv)
{
    /*
     * If a previous repaint is still in progress, wait for it to
     * finish. Then trigger a repaint on all cubes that need one.
     *
     * Since we always send VRAM data to the radio in order of
     * increasing address, having the repaint trigger (vram.flags) at
     * the end of memory guarantees that the remainder of VRAM will
     * have already been sent by the time the cube gets the trigger.
     *
     * Why does this operate on a cube vector? Because we want to
     * trigger all cubes at close to the same time. So, we first wait
     * for all cubes to finish their last paint, then we trigger all
     * cubes.
     */

    _SYSCubeIDVector waitVec = cv;
    while (waitVec) {
        _SYSCubeID id = Intrinsic::CLZ(waitVec);
        CubeSlots::instances[id].waitForPaint();
        waitVec ^= Intrinsic::LZ(id);
    }

    SysTime::Ticks timestamp = SysTime::ticks();

    _SYSCubeIDVector paintVec = cv;
    while (paintVec) {
        _SYSCubeID id = Intrinsic::CLZ(paintVec);
        CubeSlots::instances[id].triggerPaint(timestamp);
        paintVec ^= Intrinsic::LZ(id);
    }
}

void CubeSlots::finishCubes(_SYSCubeIDVector cv)
{
    while (cv) {
        _SYSCubeID id = Intrinsic::CLZ(cv);
        CubeSlots::instances[id].waitForFinish();
        cv ^= Intrinsic::LZ(id);
    }
}

void CubeSlots::assetLoaderTask(void *)
{
    /*
     * Pump data from flash memory into the current _SYSAssetLoader as needed.
     * This lets us install assets from ISR context without accessing flash
     * directly. The _SYSAssetLoader includes a tiny FIFO buffer for each cube.
     */

    _SYSAssetLoader *L = assetLoader;
    _SYSAssetLoaderCube *cubeArray = reinterpret_cast<_SYSAssetLoaderCube*>(L + 1);
    if (!L) return;

    _SYSCubeIDVector cubeVec = L->cubeVec & ~L->complete;
    while (cubeVec) {
        _SYSCubeID id = Intrinsic::CLZ(cubeVec);
        cubeVec ^= Intrinsic::LZ(id);
        _SYSAssetLoaderCube *cube = cubeArray + id;
        if (SvmMemory::mapRAM(cube, sizeof *cube))
            fetchAssetLoaderData(cube);
    }
}

void CubeSlots::fetchAssetLoaderData(_SYSAssetLoaderCube *lc)
{
    /*
     * Given a guaranteed-valid pointer to a _SYSAssetLoaderCube,
     * try to top off the FIFO buffer with data from flash memory.
     */

    // Sample the FIFO state exactly once, and validate it.
    int head = lc->head;
    int tail = lc->tail;
    if (head >= _SYS_ASSETLOAD_BUF_SIZE || tail >= _SYS_ASSETLOAD_BUF_SIZE)
        return;
    unsigned fifoCount = umod(tail - head, _SYS_ASSETLOAD_BUF_SIZE);
    unsigned fifoAvail = _SYS_ASSETLOAD_BUF_SIZE - 1 - fifoCount;

    /*
     * Sample the progress state exactly once, and figure out how much
     * data can be copied into the FIFO right now.
     */
    uint32_t progress = lc->progress;
    uint32_t dataSize = lc->dataSize;
    if (progress > dataSize)
        return;
    unsigned count = MIN(fifoAvail, dataSize - progress);

    // Nothing to do?
    if (count == 0)
        return;

    // Follow the pAssetGroup pointer
    SvmMemory::VirtAddr groupVA = lc->pAssetGroup;
    SvmMemory::PhysAddr groupPA;
    if (!SvmMemory::mapRAM(groupVA, sizeof(_SYSAssetGroup), groupPA))
        return;
    _SYSAssetGroup *G = reinterpret_cast<_SYSAssetGroup*>(groupPA);

    /*
     * Calculate the VA we're reading from in flash. We can do this without
     * mapping the _SYSAssetGroupHeader at all, which avoids a bit of
     * cache pollution.
     */

    SvmMemory::VirtAddr srcVA = G->pHdr + sizeof(_SYSAssetGroupHeader) + progress;

    // Write to the FIFO.

    FlashBlockRef ref;
    progress += count;
    while (count--) {
        SvmMemory::copyROData(ref, &lc->buf[tail], srcVA, 1);
        if (++tail == _SYS_ASSETLOAD_BUF_SIZE)
            tail = 0;
        srcVA++;
    }

    /*
     * Order matters when writing back state: The CubeCodec detects
     * completion by noticing that progress==dataSize and the FIFO is empty.
     * To avoid it detecting a false positive, we must update 'progress'
     * after writing 'tail'.
     */
    
    lc->tail = tail;
    Atomic::Barrier();
    lc->progress = progress;
    ASSERT(progress <= dataSize);
}
