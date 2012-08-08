/*
 * Thundercracker Firmware -- Confidential, not for redistribution.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#include <protocol.h>
#include "assetloader.h"
#include "assetutil.h"
#include "assetslot.h"
#include "radio.h"
#include "cubeslots.h"
#include "tasks.h"

_SYSAssetLoader *AssetLoader::userLoader;
const _SYSAssetConfiguration *AssetLoader::userConfig[_SYS_NUM_CUBE_SLOTS];
uint8_t AssetLoader::userConfigSize[_SYS_NUM_CUBE_SLOTS];
uint8_t AssetLoader::cubeTaskState[_SYS_NUM_CUBE_SLOTS];
uint8_t AssetLoader::cubeBufferAvail[_SYS_NUM_CUBE_SLOTS];
SysTime::Ticks AssetLoader::cubeDeadline[_SYS_NUM_CUBE_SLOTS];
AssetLoader::SubState AssetLoader::cubeTaskSubstate[_SYS_NUM_CUBE_SLOTS];
_SYSCubeIDVector AssetLoader::activeCubes;
_SYSCubeIDVector AssetLoader::startedCubes;
_SYSCubeIDVector AssetLoader::cacheCoherentCubes;
_SYSCubeIDVector AssetLoader::resetPendingCubes;
_SYSCubeIDVector AssetLoader::resetAckCubes;
FlashLFSIndexRecord::KeyVector_t AssetLoader::slotsInProgress;


void AssetLoader::init()
{
    /*
     * Reset the state of the AssetLoader before a new userspace Volume starts.
     * This is similar to finish(), but we never wait.
     */

    // This is sufficient to invalidate all other state
    userLoader = NULL;
    activeCubes = 0;
}

void AssetLoader::finish()
{
    // Wait until loads finish and/or cubes disconnect
    while (activeCubes)
        Tasks::idle();

    // Reset user state, disconnecting the AssetLoader
    init();
}

void AssetLoader::cancel(_SYSCubeIDVector cv)
{
    // Undo the effects of 'start'. Make this cube inactive, and don't auto-restart it.
    Atomic::And(startedCubes, ~cv);
    Atomic::And(activeCubes, ~cv);
    updateActiveCubes();
}

void AssetLoader::cubeConnect(_SYSCubeID id)
{
    // Restart loading on this cube, if we aren't done loading yet

    ASSERT(id < _SYS_NUM_CUBE_SLOTS);
    _SYSCubeIDVector bit = Intrinsic::LZ(id);
    ASSERT(0 == (activeCubes & bit));

    if (!activeCubes) {
        /*
         * Load is already done! If we added a new cube now,
         * there wouldn't be a non-racy way for userspace to
         * finish the loading process. We have to stay done.
         */
        return;
    }

    if (startedCubes & bit) {
        // Re-start this cube
        fsmEnterState(id, S_RESET);
        Atomic::Or(startedCubes, bit);
        Tasks::trigger(Tasks::AssetLoader);
    }
}

void AssetLoader::cubeDisconnect(_SYSCubeID id)
{
    // Disconnected cubes immediately become inactive, but we can restart them.
    ASSERT(id < _SYS_NUM_CUBE_SLOTS);
    _SYSCubeIDVector bit = Intrinsic::LZ(id);
    Atomic::And(activeCubes, ~bit);
    Atomic::And(cacheCoherentCubes, ~bit);
    updateActiveCubes();
}

void AssetLoader::start(_SYSAssetLoader *loader, const _SYSAssetConfiguration *cfg,
    unsigned cfgSize, _SYSCubeIDVector cv)
{
    ASSERT(loader);
    ASSERT(userLoader == loader || !userLoader);
    userLoader = loader;

    ASSERT(cfg);
    ASSERT(cfgSize < 0x100);

    // If these cubes were already loading, temporarily cancel them
    cancel(cv);

    // Update per-cube state
    _SYSCubeIDVector iterCV = cv;
    while (iterCV) {
        _SYSCubeID id = Intrinsic::CLZ(iterCV);
        _SYSCubeIDVector bit = Intrinsic::LZ(id);
        iterCV ^= bit;

        userConfig[id] = cfg;
        userConfigSize[id] = cfgSize;

        fsmEnterState(id, S_RESET);

        // Zero out the _SYSAssetLoaderCube.
        _SYSAssetLoaderCube *lc = AssetUtil::mapLoaderCube(loader, id);
        if (lc) {
            memset(lc, 0, sizeof *lc);
        } else {
            // Complain just via logging. We're too far in to stop now,
            // though this will obviously prevent the load from making progress.
            LOG(("ASSET[%d]: Cannot map _SYSAssetLoaderCube. Bad _SYSAssetLoader address.\n", id));
        }
    }

    // Atomically enable these cubes
    Atomic::Or(startedCubes, cv);
    Atomic::Or(activeCubes, cv);
    updateActiveCubes();

    // Poke our task, if necessary
    Tasks::trigger(Tasks::AssetLoader);
}

void AssetLoader::ackReset(_SYSCubeID id)
{
    /*
     * Reset ACKs affect the Task state machine, but we can't safely edit
     * the task state directly because we're in ISR context. So, set a bit
     * in resetAckCubes that our Task will poll for.
     */

    Atomic::SetLZ(resetAckCubes, id);
    Tasks::trigger(Tasks::AssetLoader);
}

void AssetLoader::ackData(_SYSCubeID id, unsigned bytes)
{
    /*
     * FIFO Data acknowledged. We can update cubeBufferAvail directly.
     */

    unsigned buffer = cubeBufferAvail[id];
    buffer += bytes;

    if (buffer > FLS_FIFO_USABLE) {
        LOG(("ASSET[%d]: Acknowledged more data than sent (%d + %d > %d)\n",
            id, buffer - bytes, bytes, FLS_FIFO_USABLE));
        buffer = FLS_FIFO_USABLE;
    }

    cubeBufferAvail[id] = buffer;
}

bool AssetLoader::needFlashPacket(_SYSCubeID id)
{
    /*
     * If we return 'true' we're committed to sending a flash packet. So, we'll only do this
     * if we definitely either need a flash reset, or we have FIFO data to send right away.
     */

    ASSERT(userLoader);
    ASSERT(id < _SYS_NUM_CUBE_SLOTS);
    _SYSCubeIDVector bit = Intrinsic::LZ(id);
    ASSERT(bit & activeCubes);

    if (bit & resetPendingCubes)
        return true;

    if (cubeBufferAvail[id] != 0) {
        _SYSAssetLoaderCube *lc = AssetUtil::mapLoaderCube(userLoader, id);
        if (lc) {
            AssetFIFO fifo(*lc);
            if (fifo.readAvailable())
                return true;
        }
    }

    return false;
}

bool AssetLoader::needFullACK(_SYSCubeID id)
{
    /*
     * We'll take a full ACK if we're waiting on the cube to finish something:
     * Either a reset, or some flash programming.
     */

    ASSERT(userLoader);
    ASSERT(Intrinsic::LZ(id) & activeCubes);
    ASSERT(id < _SYS_NUM_CUBE_SLOTS);

    if (cubeTaskState[id] == S_RESET)
        return true;

    if (cubeBufferAvail[id] == 0)
        return true;

    return false;
}

void AssetLoader::produceFlashPacket(_SYSCubeID id, PacketBuffer &buf)
{
    /*
     * We should only end up here if needFlashPacket() returns true,
     * meaning we either (1) need a reset, or (2) have FIFO data to send.
     */

    ASSERT(userLoader);
    ASSERT(id < _SYS_NUM_CUBE_SLOTS);
    _SYSCubeIDVector bit = Intrinsic::LZ(id);
    ASSERT(bit & activeCubes);

    if (bit & resetPendingCubes) {
        // An empty flash packet is a flash reset
        Atomic::And(resetPendingCubes, ~bit);
        return;
    }

    /*
     * Send data that was generated by our task, and stored in the AssetFIFO.
     */

    _SYSAssetLoaderCube *lc = AssetUtil::mapLoaderCube(userLoader, id);
    if (lc) {
        AssetFIFO fifo(*lc);
        unsigned avail = cubeBufferAvail[id];
        unsigned count = MIN(fifo.readAvailable(), buf.bytesFree());
        count = MIN(count, avail);
        avail -= count;
        ASSERT(count);

        while (count) {
            buf.append(fifo.read());
            count--;
        }

        fifo.commitReads();
        cubeBufferAvail[id] = avail;
    }
}

void AssetLoader::task()
{
    /*
     * Pump the state machine, on each active cube.
     */

    _SYSCubeIDVector cv = activeCubes;
    while (cv) {
        _SYSCubeID id = Intrinsic::CLZ(cv);
        cv ^= Intrinsic::LZ(id);

        fsmTaskState(id, TaskState(cubeTaskState[id]));
    }
}

void AssetLoader::prepareCubeForLoading(_SYSCubeID id)
{
    /*
     * This is called in Task context, while we're waiting for a state machine reset.
     *
     * Here, our job is to do all of the preparation work necessary prior to asset
     * loading, without actually sending any data to the cubes yet. This work will
     * be re-done if the cube reconnects.
     *
     * Work we do here:
     *
     *   1. Erase asset slots, if necessary
     *   2. Reset the cube's progress meter, calculating a fresh total
     *
     * Note that the _SYSAssetLoaderCube was already zero'ed in start().
     */

    unsigned dataSizeWithoutErase[_SYS_ASSET_SLOTS_PER_BANK];
    unsigned dataSizeWithErase[_SYS_ASSET_SLOTS_PER_BANK];
    int tilesFree[_SYS_ASSET_SLOTS_PER_BANK];
    unsigned numSlots = VirtAssetSlots::getNumBoundSlots();
    _SYSCubeIDVector bit = Intrinsic::LZ(id);

    for (unsigned slot = 0; slot < numSlots; ++slot) {
        tilesFree[slot] = VirtAssetSlots::getInstance(slot).tilesFree(bit);
        dataSizeWithoutErase[slot] = 0;
        dataSizeWithErase[slot] = 0;
    }

    /*
     * Simulate how much space would be free in each asset slot if we started
     * loading the Configuration, and simulate how much total data transfer
     * would occur both with and without erasing the slot.
     *
     * As we track the free tiles, we'll eventually know whether the slot needs
     * to be erased or not. This will determine which dataSize totals we'll use
     * for the progress calculations.
     *
     * Note: We find out, during this process, about which groups are loaded
     * already and which aren't. We could store that information for later, but
     * it would be quite large in the worst-case: hundreds of bytes just for
     * a bitmap. The bottom line is that AssetConfigurations are large objects
     * from our point of view, and we can't afford to store anything that scales
     * at the same rate. So we're stuck doing this load test twice.
     */

    const _SYSAssetConfiguration *cfg = userConfig[id];
    const _SYSAssetConfiguration *cfgEnd = cfg + userConfigSize[id];
    for (; cfg != cfgEnd; ++cfg) {

        AssetGroupInfo group;
        if (!group.fromAssetConfiguration(cfg))
            continue;

        unsigned slot = cfg->slot;
        if (slot >= numSlots) {
            LOG(("ASSET: Bad slot number %d in _SYSAssetConfiguration\n", slot));
            continue;
        }

        _SYSCubeIDVector cachedCV;
        if (VirtAssetSlots::locateGroup(group, bit, cachedCV) && cachedCV == bit) {
            // This group is already loaded; only need to resend if we're erasing.

            dataSizeWithErase[slot] += group.dataSize;

        } else {
            // Not already loaded. We need to send it unconditionally, and it counts
            // against our tilesFree total.

            dataSizeWithErase[slot] += group.dataSize;
            dataSizeWithoutErase[slot] += group.dataSize;
            tilesFree[slot] -= roundup<_SYS_ASSET_GROUP_SIZE_UNIT>(group.numTiles);
        }
    }

    /*
     * Now iterate over slots, erasing if necessary, and setting up
     * the progress totals.
     */

    for (unsigned slot = 0; slot < numSlots; ++slot) {
        unsigned dataSize;

        if (tilesFree[slot] < 0) {
            // Underflow! Erase the slot.
            VirtAssetSlots::getInstance(slot).erase(bit);
            dataSize = dataSizeWithErase[slot];
        } else {
            dataSize = dataSizeWithoutErase[slot];
        }

        _SYSAssetLoaderCube *lc = AssetUtil::mapLoaderCube(userLoader, id);
        if (lc) {
            lc->progress = 0;
            lc->total = dataSize;
        }
    }
}


#if 0


void CubeSlots::fetchAssetLoaderData(_SYSAssetLoaderCube *lc)
{
    /*
     * Given a guaranteed-valid pointer to a _SYSAssetLoaderCube,
     * try to top off the FIFO buffer with data from flash memory.
     */

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


void CubeSlot::startAssetLoad(SvmMemory::VirtAddr groupVA, uint16_t baseAddr)
{
    /*
     * Trigger the beginning of an asset group installation for this cube.
     * There must be a SYSAssetLoader currently set.
     */

    // Translate and verify addresses
    SvmMemory::PhysAddr groupPA;
    if (!SvmMemory::mapRAM(groupVA, sizeof(_SYSAssetGroup), groupPA))
        return;
    _SYSAssetGroup *G = reinterpret_cast<_SYSAssetGroup*>(groupPA);
    _SYSAssetLoader *L = CubeSlots::assetLoader;
    if (!L) return;
    _SYSAssetLoaderCube *LC = assetLoaderCube(L);
    if (!LC) return;
    _SYSAssetGroupCube *GC = assetGroupCube(G);
    if (!GC) return;

    // Read (cached) asset group header. Must be valid.
    const _SYSAssetGroupHeader *headerVA =
        reinterpret_cast<const _SYSAssetGroupHeader*>(G->pHdr);
    _SYSAssetGroupHeader header;
    if (!SvmMemory::copyROData(header, headerVA))
        return;

    // Because we're storing this in a 32-bit struct field, squash groupVA
    SvmMemory::squashPhysicalAddr(groupVA);

    // Initialize state
    Atomic::ClearLZ(L->complete, id());
    GC->baseAddr = baseAddr;
    LC->pAssetGroup = groupVA;
    LC->progress = 0;
    LC->dataSize = header.dataSize;
    LC->reserved = 0;
    LC->head = 0;
    LC->tail = 0;

 

    LOG(("FLASH[%d]: Sending asset group %s, at base address 0x%08x\n",
        id(), SvmDebugPipe::formatAddress(G->pHdr).c_str(), baseAddr));

    DEBUG_ONLY({
        // In debug builds, we log the asset download time
        assetLoadTimestamp = SysTime::ticks();
    });

    // Start by resetting the flash decoder.
    requestFlashReset();
    Atomic::SetLZ(CubeSlots::flashAddrPending, id());

    // Only _after_ triggering the reset, start the actual download
    // by marking cubeVec as valid.
    Atomic::SetLZ(L->cubeVec, id());

    // Start filling our asset data FIFOs.
    Tasks::trigger(Tasks::AssetLoader);
}

    void flashAckBytes(uint8_t count) {
        loadBufferAvail += count;
        ASSERT(loadBufferAvail <= FLS_FIFO_USABLE);
    }

//////////////////////////

    /*
     * If we need to send an address command, send that first. If we can
     * still cram in some actual loadstream data, awesome, but this is
     * the only part that must succeed.
     */

    if (flashAddrPending) {
        ASSERT(buf.bytesFree() >= 3);

        // Opcode, lat1, lat2:a21
        ASSERT(baseAddr < 0x8000);
        buf.append(0xe1);
        buf.append(baseAddr << 1);
        buf.append(((baseAddr >> 6) & 0xfe) | ((baseAddr >> 14) & 1));

        Atomic::And(CubeSlots::flashAddrPending, ~cubeBit);
        ASSERT(count >= 3);
        ASSERT(loadBufferAvail >= 3);
        count -= 3;
        loadBufferAvail -= 3;
        if (!count)
            return true;
    }

////////////////////////

                    /* Finished sending the group, and the cube finished writing it. */
                    Atomic::SetLZ(L->complete, id());
                    Event::setCubePending(Event::PID_CUBE_ASSETDONE, id());

                    DEBUG_ONLY({
                        // In debug builds only, we log the asset download time
                        float seconds = (SysTime::ticks() - assetLoadTimestamp) * (1.0f / SysTime::sTicks(1));
                        LOG(("FLASH[%d]: Finished loading in %.3f seconds\n", id(), seconds));
                    });

/////////////////////


/////////////

    // Finalize the SysLFS state for any slots we're loading to
    VirtAssetSlots::finalizeGroup(gSlotsInProgress);
    ASSERT(gSlotsInProgress.empty());

////////////////

// Simulator headers, for simAssetLoaderBypass.
#ifdef SIFTEO_SIMULATOR
#   include "system_mc.h"
#   include "cube_hardware.h"
#   include "lsdec.h"
#endif


#endif

