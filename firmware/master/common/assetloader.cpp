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
uint8_t AssetLoader::cubeLastQuery[_SYS_NUM_CUBE_SLOTS];
SysTime::Ticks AssetLoader::cubeDeadline[_SYS_NUM_CUBE_SLOTS];
AssetLoader::SubState AssetLoader::cubeTaskSubstate[_SYS_NUM_CUBE_SLOTS];
_SYSCubeIDVector AssetLoader::activeCubes;
_SYSCubeIDVector AssetLoader::startedCubes;
_SYSCubeIDVector AssetLoader::cacheCoherentCubes;
_SYSCubeIDVector AssetLoader::resetPendingCubes;
_SYSCubeIDVector AssetLoader::resetAckCubes;
_SYSCubeIDVector AssetLoader::queryPendingCubes;
_SYSCubeIDVector AssetLoader::queryErrorCubes;
DEBUG_ONLY(SysTime::Ticks AssetLoader::groupBeginTimestamp[_SYS_NUM_CUBE_SLOTS];)


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

    // Forget about any queries sent prior to the connect
    cubeLastQuery[id] = 0;

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
        resetDeadline(id);
        fsmEnterState(id, S_RESET1);
        Atomic::Or(activeCubes, bit);
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
    Atomic::And(queryErrorCubes, ~bit);
    Atomic::And(queryPendingCubes, ~bit);
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

        resetDeadline(id);
        fsmEnterState(id, S_RESET1);

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
    Atomic::Or(activeCubes, cv & CubeSlots::userConnected);
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
     * Only called when 'bytes' is nonzero.
     */

    ASSERT(bytes);

    unsigned buffer = cubeBufferAvail[id];
    buffer += bytes;

    if (buffer > FLS_FIFO_USABLE) {
        LOG(("ASSET[%d]: Acknowledged more data than sent (%d + %d > %d)\n",
            id, buffer - bytes, bytes, FLS_FIFO_USABLE));
        ASSERT(0);
        buffer = FLS_FIFO_USABLE;
    }

    cubeBufferAvail[id] = buffer;
    Tasks::trigger(Tasks::AssetLoader);
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

    unsigned state = cubeTaskState[id];
    if (state >= S_BEGIN_RESET_STATES && state <= S_END_RESET_STATES)
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

    _SYSCubeIDVector cv = activeCubes & CubeSlots::userConnected;
    while (cv) {
        _SYSCubeID id = Intrinsic::CLZ(cv);
        cv ^= Intrinsic::LZ(id);

        fsmTaskState(id, TaskState(cubeTaskState[id]));
    }
}

void AssetLoader::heartbeat()
{
    /*
     * Check on our watchdog timer for any active cubes. If the watchdog
     * has timed out, we complain loudly and restart that cube.
     */

    if (!userLoader)
        return;

    _SYSCubeIDVector cv = activeCubes & CubeSlots::userConnected;
    if (!cv)
        return;

    SysTime::Ticks now = SysTime::ticks();

    while (cv) {
        _SYSCubeID id = Intrinsic::CLZ(cv);
        cv ^= Intrinsic::LZ(id);

        if (now > cubeDeadline[id]) {
            LOG(("ASSET[%d]: Deadline passed, restarting load\n", id));

            resetDeadline(id);
            fsmEnterState(id, S_RESET1);
        }
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
    int groupsFree[_SYS_ASSET_SLOTS_PER_BANK];
    unsigned numSlots = VirtAssetSlots::getNumBoundSlots();
    _SYSCubeIDVector bit = Intrinsic::LZ(id);

    for (unsigned slot = 0; slot < numSlots; ++slot) {
        SysLFS::AssetSlotRecord asr;
        VirtAssetSlots::getInstance(slot).getRecordForCube(id, asr);

        tilesFree[slot] = _SYS_TILES_PER_ASSETSLOT - asr.totalTiles();
        groupsFree[slot] = _SYS_ASSET_GROUPS_PER_SLOT - asr.totalGroups();
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
            LOG(("ASSET[%d]: Bad slot number %d in _SYSAssetConfiguration\n", id, slot));
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
            groupsFree[slot]--;
        }
    }

    /*
     * XXX: the above can be slow on hardware in some cases.
     *      VirtAssetSlots::locateGroup() appears to be the bottleneck.
     *
     *      Example: when 15 configurations are found to contain a group,
     *      I have observed this loop taking as long as 730ms (!).
     *      With enough cubes connected, the entire task() invocation
     *      exceeds the Tasks watchdog period,
     *      and an F_NOT_RESPONDING fault is generated.
     *
     *      Reset the watchdog for now, until we can do some
     *      more fine grained profiling to improve this.
     */
    Tasks::resetWatchdog();

    /*
     * Now iterate over slots, erasing if necessary. Calculate the 
     * total amount of data to send.
     */

    unsigned totalData = 0;

    for (unsigned slot = 0; slot < numSlots; ++slot) {

        if (tilesFree[slot] < 0 || groupsFree[slot] < 0) {
            // Underflow! Erase the slot.
            LOG(("ASSET[%d]: Automatically erasing slot %d\n", id, slot));
            VirtAssetSlots::getInstance(slot).erase(bit);
            totalData += dataSizeWithErase[slot];
        } else {
            totalData += dataSizeWithoutErase[slot];
        }
    }

    /*
     * Use our calculated total to initialize the userspace-visible
     * progress estimate.
     */

    _SYSAssetLoaderCube *lc = AssetUtil::mapLoaderCube(userLoader, id);
    if (lc) {
        lc->progress = 0;
        lc->total = totalData;
    }
}

void AssetLoader::queryResponse(_SYSCubeID id, const PacketBuffer &packet)
{
    /*
     * This is invoked by CubeSlot in ISR context when a new query
     * packet arrives. We only keep track of one asynchronous query
     * per cube, so we'll check if it's a response to that query. If not,
     * this packet is ignored.
     *
     * When the queryPendingCubes bit is set for a cube, it means we
     * compare the query against the expected data, which is stowed
     * in the 'spare' area of our AsetFIFO buffer
     *
     * If the query does not match the expected result, we set the
     * queryErrorCubes bit for this cube. Either way, we clear the
     * 'pending' bit and wake up our task.
     */

    _SYSCubeIDVector bit = Intrinsic::LZ(id);

    if (packet.len != _SYS_ASSET_GROUP_CRC_SIZE + 1) {
        LOG(("ASSET[%d]: Query response had unexpected size\n", id));
        return;
    }

    if (!(queryPendingCubes & bit)) {
        LOG(("ASSET[%d]: Query response received unexpectedly\n", id));
        return;
    }

    const uint8_t *crc = &packet.bytes[1];
    if (packet.bytes[0] != cubeLastQuery[id]) {
        LOG(("ASSET[%d]: Query response had unexpected ID\n", id));
        return;
    }

    _SYSAssetLoaderCube *lc = AssetUtil::mapLoaderCube(userLoader, id);
    if (!lc) {
        LOG(("ASSET[%d]: Cannot process query result without _SYSAssetLoaderCube\n", id));
        return;
    }

    AssetFIFO fifo(*lc);
    bool match = true;

    for (unsigned i = 0; i < _SYS_ASSET_GROUP_CRC_SIZE; ++i) {
        if (crc[i] != fifo.spare(i)) {
            match = false;
            break;
        }
    }

    #ifdef UART_CRC_QUERY_DEBUG
        UART("CRC query finished. Cube/slot/match/00: ");
            UART_HEX( (id << 24) | (Intrinsic::CLZ16(cubeTaskSubstate[id].crc.remaining) << 16) | (match << 8) );
        UART("\r\nExpect: ");
            UART_HEX( (fifo.spare(0) << 24)  | (fifo.spare(1) << 16)  | (fifo.spare(2) << 8)  | (fifo.spare(3) << 0) );
            UART_HEX( (fifo.spare(4) << 24)  | (fifo.spare(5) << 16)  | (fifo.spare(6) << 8)  | (fifo.spare(7) << 0) );
            UART_HEX( (fifo.spare(8) << 24)  | (fifo.spare(9) << 16)  | (fifo.spare(10) << 8) | (fifo.spare(11) << 0) );
            UART_HEX( (fifo.spare(12) << 24) | (fifo.spare(13) << 16) | (fifo.spare(14) << 8) | (fifo.spare(15) << 0) );
        UART("\r\nActual: ");
            UART_HEX( (crc[0] << 24)  | (crc[1] << 16)  | (crc[2] << 8)  | (crc[3] << 0) );
            UART_HEX( (crc[4] << 24)  | (crc[5] << 16)  | (crc[6] << 8)  | (crc[7] << 0) );
            UART_HEX( (crc[8] << 24)  | (crc[9] << 16)  | (crc[10] << 8) | (crc[11] << 0) );
            UART_HEX( (crc[12] << 24) | (crc[13] << 16) | (crc[14] << 8) | (crc[15] << 0) );
        UART("\r\n");
    #endif

    if (!match) {
        /*
         * Log the mismatch
         */
        LOG(("ASSET[%d]: Bad CRC for physical slot %d:\nASSET[%d]:    Expected: ",
             id, Intrinsic::CLZ16(cubeTaskSubstate[id].crc.remaining), id));
        for (unsigned i = 0; i < _SYS_ASSET_GROUP_CRC_SIZE; ++i) {
            LOG(("%02x", fifo.spare(i)));
        }
        LOG(("\nASSET[%d]:    Actual:   ", id));
        for (unsigned i = 0; i < _SYS_ASSET_GROUP_CRC_SIZE; ++i) {
            LOG(("%02x", crc[i]));
        }
        LOG(("\n"));

        // Flag an error beore we clear the pending bit
        Atomic::Or(queryErrorCubes, bit);
    }

    // All done. Wake up the FSM.
    Atomic::And(queryPendingCubes, ~bit);
    Tasks::trigger(Tasks::AssetLoader);
}
