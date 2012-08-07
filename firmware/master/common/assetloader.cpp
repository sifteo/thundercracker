/*
 * Thundercracker Firmware -- Confidential, not for redistribution.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */


#include "assetloader.h"
#include "radio.h"

_SYSAssetLoader *AssetLoader::userLoader;
_SYSAssetConfiguration *AssetLoader::userConfig[_SYS_NUM_CUBE_SLOTS];
uint8_t AssetLoader::userConfigSize[_SYS_NUM_CUBE_SLOTS];
_SYSCubeIDVector AssetLoader::activeCubes;
_SYSCubeIDVector AssetLoader::startedCubes;

#ifdef SIFTEO_SIMULATOR
bool AssetLoader::simBypass;
#endif


void AssetLoader::start(_SYSAssetLoader *userLoader, const _SYSAssetConfiguration *cfg,
    unsigned cfgSize, _SYSCubeIDVector cv)
{
}

void AssetLoader::cancel(_SYSCubeIDVector cv)
{
}

void AssetLoader::finish()
{
}

void AssetLoader::init()
{
}

void AssetLoader::cubeConnect(_SYSCubeID id)
{
}

void AssetLoader::cubeDisconnect(_SYSCubeID id)
{
}

bool AssetLoader::needFlashPacket(_SYSCubeID id)
{
    return false;
}

bool AssetLoader::needFullACK(_SYSCubeID id)
{
    return false;
}

void AssetLoader::produceFlashPacket(_SYSCubeID id, PacketBuffer &buf)
{
}

void AssetLoader::ackReset(_SYSCubeID id)
{
}

void AssetLoader::ackData(_SYSCubeID id, unsigned bytes)
{
}

void AssetLoader::task()
{
}







#if 0

class AssetLoader
{
public:

    static void start(_SYSAssetLoader *userLoader, const _SYSAssetConfiguration *cfg,
        unsigned cfgSize, _SYSCubeIDVector cv);
    static void cancel(_SYSCubeIDVector cv);
    static void finish();

    /// Per-process initialization
    static void init();

    /// Tasks::AssetLoader callback
    static void task();

    /// Return the current _SYSAssetLoader, if any is attached, or NULL if we're unattached.
    static _SYSAssetLoader *getUserLoader() const {
        return userLoader;
    }

private:
    AssetLoader();  // Do not implement

    static _SYSAssetLoader *userLoader;
    static _SYSAssetConfiguration *userConfig[_SYS_NUM_CUBE_SLOTS];
    static uint8_t userConfigSize[_SYS_NUM_CUBE_SLOTS];
    static _SYSCubeIDVector activeCubes;
};

void CubeSlots::assetLoaderTask()
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

    #ifdef SIFTEO_SIMULATOR
    if (CubeSlots::simAssetLoaderBypass) {
        /*
         * Asset loader bypass mode: Instead of actually sending this
         * loadstream over the radio, instantaneously decompress it into
         * the cube's flash memory.
         */

        // Use our reference implementation of the Loadstream decoder
        Cube::Hardware *simCube = SystemMC::getCubeForSlot(this);
        if (simCube) {
            FlashStorage::CubeRecord *storage = simCube->flash.getStorage();
            LoadstreamDecoder lsdec(storage->ext, sizeof storage->ext);

            lsdec.setAddress(baseAddr << 7);
            lsdec.handleSVM(G->pHdr + sizeof header, header.dataSize);

            LOG(("FLASH[%d]: Installed asset group %s at base address "
                "0x%08x (loader bypassed)\n",
                id(), SvmDebugPipe::formatAddress(G->pHdr).c_str(), baseAddr));

            // Mark this as done already.
            LC->progress = header.dataSize;
            Atomic::SetLZ(L->complete, id());

            return;
        }
    }
    #endif

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

bool CubeCodec::flashSend(PacketBuffer &buf, _SYSAssetLoaderCube *lc, _SYSCubeID cube, bool &done)
{
    /*
     * Since we're dealing with asset loader state in untrusted memory,
     * this code has to be carefully written to read each user value exactly
     * once, and check it before use.
     *
     * We only send if we have asset data buffered, obviously, but also
     * if the cube has enough buffer space to accept it, and if
     * there's enough room in the packet for both the escape code
     * and at least one byte of flash data.
     *
     * This function MUST NOT access flash memory or the cache, since it's
     * running in interrupt context. All of our state must come from the
     * _SYSAssetLoaderCube object in RAM.
     *
     * Returns 'true' if and only if we sent an escape which ends the packet.
     * Sets 'done' to 'true' if and only if the assset group is fully written.
     */

    ASSERT(lc);

    // Sample the FIFO state exactly once, and validate it.
    int head = lc->head;
    int tail = lc->tail;
    if (head >= _SYS_ASSETLOAD_BUF_SIZE || tail >= _SYS_ASSETLOAD_BUF_SIZE)
        return false;
    unsigned fifoCount = umod(tail - head, _SYS_ASSETLOAD_BUF_SIZE);

    /*
     * We have a state bit in CubeSlots to keep track of whether we
     * need to send an addressing command to the decoder, so that it
     * knows this asset group's load address. Addressing commands are
     * part of the loadstream that we send over the radio, but they're
     * created dynamically rather than coming straight from flash.
     */
    _SYSCubeIDVector cubeBit = Intrinsic::LZ(cube);
    bool flashAddrPending = (CubeSlots::flashAddrPending & cubeBit) != 0;

    /*
     * If and only if we may need to send an address, prepare that address.
     * It's in user RAM, so this mapping may fail.
     */
    uint16_t baseAddr = 0;
    if (flashAddrPending) {
        SvmMemory::PhysAddr groupCubePA;
        SvmMemory::VirtAddr groupCubeVA =
            lc->pAssetGroup + sizeof(_SYSAssetGroup) + sizeof(_SYSAssetGroupCube) * cube;
        
        if (!SvmMemory::mapRAM(groupCubeVA, sizeof(_SYSAssetGroupCube), groupCubePA))
            return false;

        baseAddr = reinterpret_cast<_SYSAssetGroupCube*>(groupCubePA)->baseAddr;
    }

    /*
     * How much space do we need? The minimum unit of asset data is
     * just a byte, but an addressing command is three bytes. If we don't
     * have enough, we need to be able to determine this before sending a
     * flash escape. Once we send the escape, we're committed.
     */
    const unsigned escapeSizeInBits = 12;
    unsigned dataSizeInBytes = flashAddrPending ? 3 : 1;
    unsigned dataSizeInBits = dataSizeInBytes << 3;
    if (!txBits.hasRoomForFlush(buf, escapeSizeInBits + dataSizeInBits))
        return false;

    /*
     * The cube also must have enough buffer space to receive our minimum data.
     * If the cube appears to have too little buffer space left, this may mean
     * the cube is actually out of buffer (flow control) or it may just be that
     * we dropped ACK packets. If we get into this state, try to restart the
     * pipeline by requesting an explicit ACK from the cube.
     */
    if (loadBufferAvail < dataSizeInBytes) {
        explicitAckRequest(buf);
        return true;
    }

    /*
     * Read 'progress' from untrusted memory only once. See if we're done.
     * Note that this tracks how many bytes have been enqueued into the FIFO,
     * not how many bytes were sent over the radio.
     *
     * We require that 'progress' is only updated after enqueueing data into
     * the FIFO and updating the FIFO!
     */

    uint32_t progress = lc->progress;
    uint32_t dataSize = lc->dataSize;
    if (progress >= dataSize && fifoCount == 0) {
        if (loadBufferAvail == FLS_FIFO_USABLE)
            done = true;

        // Poll for dropped ACKs here too
        explicitAckRequest(buf);
        return true;
    }

    /*
     * If we don't need to send the address, make sure we at least have one
     * byte of data in the FIFO to send. Must happen after the done-ness
     * check above.
     */

    if (!flashAddrPending && fifoCount == 0) {
        Tasks::trigger(Tasks::AssetLoader);
        return false;
    }

    /*
     * The escape command indicates that the entire remainder of 'buf' is
     * data for the flash codec. We can figure out how much data to send
     * now. We're limited by the size of the packet buffer, the size of
     * the data left to send, and the amount of space in the cube's FIFO.
     */

    flashEscape(buf);
    uint32_t count = MIN(buf.bytesFree(), loadBufferAvail);
    ASSERT(count >= dataSizeInBytes);

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

    /*
     * Stream flash data, as it becomes available from the FIFO
     */

    // May have reached this point after sending an addr, but without any FIFO data
    if (!fifoCount)
        return true;

    count = MIN(count, fifoCount);
    ASSERT(count > 0);
    ASSERT(loadBufferAvail >= count);
    loadBufferAvail -= count;

    while (count--) {
        buf.append(lc->buf[head++]);
        if (head == _SYS_ASSETLOAD_BUF_SIZE)
            head = 0;
    }

    // Update FIFO state
    lc->head = head;

    // If we're done, remember that. If not, make sure we fetch more data.
    if (progress >= dataSize && head == tail) {
        if (loadBufferAvail == FLS_FIFO_USABLE)
            done = true;
    } else {
        Tasks::trigger(Tasks::AssetLoader);
    }

    return true;
}


    if (CubeSlots::flashResetWait & cv) {
        /*
         * We need to reset the flash decoder before we can send any data.
         *
         * We can only do this if a reset is needed, hasn't already
         * been sent. Send the reset token, and synchronously reset
         * any flash-related IRQ state.
         *
         * Note the flash reset's dual purpose, of both resetting the
         * cube's flash state machine and triggering the cube to send
         * us an ACK packet with a valid flash byte count. So, we
         * actually end up sending two resets if we haven't yet seen a
         * valid flash ACK from this cube.
         */

        if (CubeSlots::flashResetSent & cv) {
            // Already sent the reset. Has it timed out?

            if (SysTime::ticks() > flashDeadline) {
                DEBUG_LOG(("FLASH[%d]: Reset timeout\n", id()));
                Atomic::ClearLZ(CubeSlots::flashResetSent, id());
            }

        } else if (codec.flashReset(tx.packet)) {
            // Okay, we sent a reset. Remember to wait for the ACK.

            DEBUG_LOG(("FLASH[%d]: Sending reset token\n", id()));
            Atomic::SetLZ(CubeSlots::flashResetSent, id());
            flashDeadline = SysTime::ticks() + SysTime::msTicks(RTT_DEADLINE_MS);
        }

    } else {
        // Not waiting on a reset. See if we need to send asset data.
        // Since we can't read external flash pages in our ISR, we're
        // restricted to accessing user RAM only. So, we send data from
        // a small user-ram buffer, and use a Task to refill that buffer.

        _SYSAssetLoader *L = CubeSlots::assetLoader;
        if (isAssetLoading(L)) {
            _SYSAssetLoaderCube *LC = assetLoaderCube(L);
            if (LC) {
                bool done = false;
                bool escape = codec.flashSend(tx.packet, LC, id(), done);

                if (done) {
                    /* Finished sending the group, and the cube finished writing it. */
                    Atomic::SetLZ(L->complete, id());
                    Event::setCubePending(Event::PID_CUBE_ASSETDONE, id());

                    DEBUG_ONLY({
                        // In debug builds only, we log the asset download time
                        float seconds = (SysTime::ticks() - assetLoadTimestamp) * (1.0f / SysTime::sTicks(1));
                        LOG(("FLASH[%d]: Finished loading in %.3f seconds\n", id(), seconds));
                    });
                }

            }
        }
    }

static FlashLFSIndexRecord::KeyVector_t gSlotsInProgress;

    /*
     * Finish first, if a different load is in progress.
     *
     * We must be able to support separate 'start' calls on the same loader
     * for different cubes. (Not all callers will know to combine all cubes
     * into a single CubeIDVector)
     */
    _SYSAssetLoader *prevLoader = CubeSlots::assetLoader;
    if (prevLoader && prevLoader != loader) {
        _SYS_asset_loadFinish(prevLoader);
        ASSERT(CubeSlots::assetLoader == 0);
        ASSERT(gSlotsInProgress.empty());
    }

    MappedAssetGroup map;
    if (!map.init(group))
        return false;

    if (!VirtAssetSlots::isSlotBound(slot)) {
        SvmRuntime::fault(F_BAD_ASSETSLOT);
        return false;
    }
    const VirtAssetSlot &vSlot = VirtAssetSlots::getInstance(slot);

    cv = CubeSlots::truncateVector(cv);

    /*
     * In one step, scan the SysLFS to the indicated group.
     *
     * If the group is cached, it's written to 'cachedCV'. If not,
     * space is allocated for it. In either case, this updates the
     * address of this group on each of the indicated cubes.
     *
     * For any cubes where this group needs to be loaded, we'll mark
     * the relevant AssetSlots as 'in progress'. A set of these
     * in-progress keys are written to gSlotsInProgress, so that
     * we can finalize them after the loading has finished.
     *
     * If this fails to allocate space, we return unsuccessfully.
     * Affected groups may be left in the indeterminate state.
     */

    _SYSCubeIDVector cachedCV;
    if (!VirtAssetSlots::locateGroup(map, cv, cachedCV, &vSlot, &gSlotsInProgress))
        return false;

    cv &= ~cachedCV;
    loader->complete |= cachedCV;

    /*
     * Begin the asset loading itself
     */

    CubeSlots::assetLoader = loader;

    while (cv) {
        _SYSCubeID id = Intrinsic::CLZ(cv);
        cv ^= Intrinsic::LZ(id);

        CubeSlot &cube = CubeSlots::instances[id];
        _SYSAssetGroupCube *gc = cube.assetGroupCube(map.group);
        if (gc) {
            cube.startAssetLoad(reinterpret_cast<SvmMemory::VirtAddr>(map.group), gc->baseAddr);
        }
    }

/////////////

    /*
     * Block until the load operation is finished, if it isn't already.
     * We can't rely on userspace to do this before we mark the in-progress
     * slots as no-longer in progress.
     */

    while ((loader->complete & loader->cubeVec) != loader->cubeVec)
        Tasks::idle();

    // No more current load operation
    CubeSlots::assetLoader = NULL;

    // Finalize the SysLFS state for any slots we're loading to
    VirtAssetSlots::finalizeGroup(gSlotsInProgress);
    ASSERT(gSlotsInProgress.empty());


// Simulator headers, for simAssetLoaderBypass.
#ifdef SIFTEO_SIMULATOR
#   include "system_mc.h"
#   include "cube_hardware.h"
#   include "lsdec.h"
#endif


#endif
