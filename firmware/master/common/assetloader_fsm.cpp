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
#include "assetslot.h"
#include "assetutil.h"
#include "machine.h"
#include "tasks.h"
#include "flash_syslfs.h"
#include "svmdebugpipe.h"
#include "event.h"


void AssetLoader::fsmEnterState(_SYSCubeID id, TaskState s)
{
    /*
     * Enter a new specified TaskState.
     * Can run either from ISR or Task context!
     */

    STATIC_ASSERT(sizeof cubeTaskSubstate[0] == sizeof cubeTaskSubstate[0].value);

    ASSERT(id < _SYS_NUM_CUBE_SLOTS);
    cubeTaskState[id] = s;
    Tasks::trigger(Tasks::AssetLoader);

    switch (s) {

        /*
         * Send a flash reset token, and wait for it to ACK.
         */
        case S_RESET1:
        case S_RESET2:
            Atomic::ClearLZ(resetAckCubes, id);
            Atomic::SetLZ(resetPendingCubes, id);
            return;

        /*
         * Done loading! This cube is no longer active.
         */
        case S_COMPLETE:
            Atomic::ClearLZ(activeCubes, id);
            updateActiveCubes();
            Event::setCubePending(Event::PID_CUBE_ASSETDONE, id);
            return;

        default:
            return;
    }
}

void AssetLoader::fsmTaskState(_SYSCubeID id, TaskState s)
{
    /*
     * Perform the Task work for a particular cube and state.
     */

    ASSERT(id < _SYS_NUM_CUBE_SLOTS);
    _SYSCubeIDVector bit = Intrinsic::LZ(id);

    DEBUG_LOG(("ASSET[%d]: Task state %d\n", id, s));

    switch (s) {

        /*
         * Just sent a reset token. Before we go to S_RESET_WAIT state,
         * take advantage of this otherwise-wasted time to do some local
         * preparation work.
         */
        case S_RESET1:
        case S_RESET2:
            resetDeadline(id);
            fsmEnterState(id, TaskState(s + 1));
            return prepareCubeForLoading(id);

        /*
         * Waiting for a flash reset, as acknowledged by resetAckCubes bits.
         * If we time out, re-enter the S_RESET1 state to try again.
         *
         * After we do one reset, we'll do a second identical reset. This
         * is necessary to make absolutely sure that the cube has no
         * residual data in its FIFO, and we're fully sync'ed up with it.
         * The first reset forces a round-trip synchronization that lets
         * us ensure we're on the same page as the cube, while the second
         * reset guarantees that, after this sync-up, the FIFO is also
         * fully drained and our cubeBufferAvail[] estimate matches the
         * cube's actual buffer state.
         */
        case S_RESET1_WAIT:
            if (resetAckCubes & bit)
                return fsmEnterState(id, S_RESET2);
            return;

        /*
         * Reset is fully done. Now we either check the cache consistency
         * with a set of CRC queries, or we get ready to start sending
         * asset groups.
         */
        case S_RESET2_WAIT:
            if (resetAckCubes & bit) {
                // Done with reset! We know the device's FIFO is empty now.
                cubeBufferAvail[id] = FLS_FIFO_USABLE;

                // Cache not consistent? Check it out first.
                if (0 == (bit & cacheCoherentCubes)) {
                    cubeTaskSubstate[id].crc.remaining = uint16_t(0xFFFF << (16 - SysLFS::ASSET_SLOTS_PER_CUBE));
                    cubeTaskSubstate[id].crc.retryCount = 0;
                    return fsmEnterState(id, S_CRC_COMMAND);
                }

                // Otherwise, begin allocating the first configuration
                cubeTaskSubstate[id].value = 0;
                fsmEnterState(id, S_CONFIG_INIT);
            }
            return;

        /*
         * Waiting to send the next CRC query. We can usually do this right away,
         * but we may have to wait if the FIFO doesn't have enough space.
         *
         * To send this query, we need to look up how many tiles are in use on
         * the slot, so we know how much memory to ask the cube to CRC for us.
         * During this process we may discover the slot is empty, in which case
         * we'll remove it from our substate mask as if the query were completed
         * successfully.
         */
        case S_CRC_COMMAND:
            while (1) {
                unsigned remaining = cubeTaskSubstate[id].crc.remaining;

                if (!remaining) {
                    // Done! Start allocating the first configuration.
                    Atomic::Or(cacheCoherentCubes, bit);
                    cubeTaskSubstate[id].value = 0;
                    return fsmEnterState(id, S_CONFIG_INIT);
                }

                unsigned slot = Intrinsic::CLZ16(remaining);
                ASSERT(id < _SYS_NUM_CUBE_SLOTS);
                ASSERT(slot < SysLFS::ASSET_SLOTS_PER_CUBE);

                /*
                 * Read in the AssetSlotRecord for this slot. It includes
                 * information about what assets we expect to see, including
                 * the total tile count and the expected CRC.
                 */

                SysLFS::AssetSlotRecord asr;
                PhysAssetSlot pSlot;

                pSlot.setIndex(slot);
                pSlot.getRecordForCube(id, asr);

                unsigned tiles = asr.totalTiles();
                if (!tiles) {
                    // Empty slot? Consider this successful, try the next.
                    remaining ^= Intrinsic::LZ16(slot);
                    cubeTaskSubstate[id].crc.remaining = remaining;
                    continue;
                }

                /*
                 * Prepare to write the query command into our FIFO.
                 *
                 * We need enough FIFO space for the command itself, plus we
                 * borrow some "spare" bytes past the end of the FIFO for storing
                 * the expected query result. This needs to be checked in ISR
                 * context, where we can't read the CRC from flash.
                 */

                _SYSAssetLoaderCube *lc = AssetUtil::mapLoaderCube(userLoader, id);
                if (!lc)
                    return fsmEnterState(id, S_ERROR);

                AssetFIFO fifo(*lc);
                const unsigned bytesNeeded = fifo.ADDRESS_SIZE + fifo.CRC_QUERY_SIZE + _SYS_ASSET_GROUP_CRC_SIZE;
                if (fifo.writeAvailable() < bytesNeeded)
                    return;

                /*
                 * Put the command in our FIFO. Does not commit it yet.
                 */

                fifo.writeAddress(slot * PhysAssetSlot::SLOT_SIZE);
                fifo.writeCRCQuery(nextQueryID(id), tiles);

                /*
                 * Just past the command, store the expected CRC. This
                 * comes right out of the SysLFS AssetSlotRecord.
                 */

                for (unsigned i = 0; i < _SYS_ASSET_GROUP_CRC_SIZE; ++i)
                    fifo.spare(i) = asr.crc[i];

                /*
                 * Now atomically trigger the query. The 'pending' flag
                 * lets our response handler know it should be active,
                 * and committing it to the FIFO allows our radio ISR
                 * to start sending it.
                 */

                Atomic::And(queryErrorCubes, ~bit);
                Atomic::Or(queryPendingCubes, bit);
                fifo.commitWrites();
                resetDeadline(id);
                return fsmEnterState(id, S_CRC_WAIT);
            }

        /*
         * Waiting for a good CRC response. We validate the received
         * CRC in ISR context, and set some atomic flags to indicate the results.
         */
        case S_CRC_WAIT: {
            if (queryPendingCubes & bit)
                return;

            unsigned remaining = cubeTaskSubstate[id].crc.remaining;
            ASSERT(remaining);
            unsigned slot = Intrinsic::CLZ16(remaining);
            ASSERT(slot < SysLFS::ASSET_SLOTS_PER_CUBE);

            if (queryErrorCubes & bit) {
                /*
                 * Oops, a CRC failure. Retry a smallish number of times, in case this was
                 * a transient electrical or radio glitch. If the CRC is indeed bad, 
                 * we have to do a really heavy-weight slot erase, so we need to be sure!
                 *
                 * XXX: I'm still not totally sure why we're seeing these transient CRC
                 *      failures, but it seems like they tend to happen sometimes (but not
                 *      all the time) on cubes that have just powered on. This might be
                 *      an issue with the flash, the accelerometer (for A21), or something
                 *      else not powering up as fast as we try to use it. If that's the case,
                 *      this isn't a terrible solution, if rather inelegant.
                 */

                const unsigned kRetryLimit = 30;

                PhysAssetSlot pSlot;
                pSlot.setIndex(slot);
                ASSERT(!(cacheCoherentCubes & bit));

                if (++cubeTaskSubstate[id].crc.retryCount > kRetryLimit) {
                    // Hard failure

                    LOG(("ASSET[%d]: Cached assets in physical slot %d not valid. Erasing.\n", id, slot));
                    pSlot.erase(id);

                    // Go back to the beginning
                    return fsmEnterState(id, S_RESET1);

                } else {
                    // Retry
                    LOG(("ASSET[%d]: Cached assets in physical slot %d not valid. Retrying CRC...\n", id, slot));
                    return fsmEnterState(id, S_CRC_COMMAND);
                }
            }

            /*
             * Next slot. We don't update the 'remaining' bitmap until now,
             * so that the query response ISR can determine which slot the
             * query pertains to.
             */

            remaining ^= Intrinsic::LZ16(slot);
            cubeTaskSubstate[id].crc.remaining = remaining;
            return fsmEnterState(id, S_CRC_COMMAND);
        }

        /*
         * Begin work on a new AssetConfiguration step, identified by
         * the substate config.index.
         *
         * Here, we actually allocate the group if it isn't already present
         * in SysLFS. If it is already present, we immediately skip to the
         * next group.
         */
        case S_CONFIG_INIT:
            while (1) {
                unsigned index = cubeTaskSubstate[id].config.index;
                unsigned configSize = userConfigSize[id];

                if (index >= configSize) {
                    // No more groups. Done loading this cube!
                    return fsmEnterState(id, S_COMPLETE);
                }

                // Validate and load Configuration info
                const _SYSAssetConfiguration *cfg = userConfig[id] + index;

                AssetGroupInfo group;
                if (!group.fromAssetConfiguration(cfg)) {
                    // Bad Asset Group, skip this config
                    cubeTaskSubstate[id].config.index = index + 1;
                    continue;
                }

                _SYSAssetSlot slot = cfg->slot;
                if (!VirtAssetSlots::isSlotBound(slot)) {
                    // Bad slot, skip this config
                    cubeTaskSubstate[id].config.index = index + 1;
                    continue;
                }
                VirtAssetSlot &vSlot = VirtAssetSlots::getInstance(slot);

                // Now search for the group, allocating it if it wasn't found.
                _SYSCubeIDVector foundCV;
                if (!VirtAssetSlots::locateGroup(group, bit, foundCV, &vSlot)) {
                    LOG(("ASSET[%d]: Unexpected allocation failure for Configuration index %d\n", id, index));
                    cubeTaskSubstate[id].config.index = index + 1;
                    continue;
                }

                // Was it already installed? Skip to the next.
                if (foundCV) {
                    ASSERT(foundCV == bit);
                    cubeTaskSubstate[id].config.index = index + 1;
                    continue;
                }

                // Can we install it instantly, via Asset Loader Bypass?
                #ifdef SIFTEO_SIMULATOR
                    if (loaderBypass(id, group)) {
                        VirtAssetSlots::finalizeSlot(id, vSlot, group);
                        cubeTaskSubstate[id].config.index = index + 1;
                        continue;
                    }
                #endif

                // Okay, we have a group ready to install, and we know the base address!
                return fsmEnterState(id, S_CONFIG_ADDR);
            }

        /*
         * Send an Address command for the current group, if there's space in the FIFO.
         */
        case S_CONFIG_ADDR: {
            _SYSAssetLoaderCube *lc = AssetUtil::mapLoaderCube(userLoader, id);
            if (!lc)
                return fsmEnterState(id, S_ERROR);

            unsigned index = cubeTaskSubstate[id].config.index;
            ASSERT(index < userConfigSize[id]);
            AssetGroupInfo group;
            if (!group.fromAssetConfiguration(userConfig[id] + index))
                return fsmEnterState(id, S_ERROR);

            AssetFIFO fifo(*lc);
            if (fifo.writeAvailable() < fifo.ADDRESS_SIZE)
                return;

            unsigned baseAddr = AssetUtil::loadedBaseAddr(group.va, id);

            DEBUG_ONLY(groupBeginTimestamp[id] = SysTime::ticks();)
            LOG(("ASSET[%d]: Group [%d/%d] loading %s at base address 0x%04x\n",
                id, index+1, userConfigSize[id],
                SvmDebugPipe::formatAddress(group.headerVA).c_str(), baseAddr));

            // Opcode, lat1, lat2:a21
            fifo.writeAddress(baseAddr);
            fifo.commitWrites();

            resetDeadline(id);
            cubeTaskSubstate[id].config.offset = 0;
            return fsmEnterState(id, S_CONFIG_DATA);
        }

        /*
         * Sending AssetGroup data for the current Configuration node, if there's space.
         */
        case S_CONFIG_DATA: {
            _SYSAssetLoaderCube *lc = AssetUtil::mapLoaderCube(userLoader, id);
            if (!lc)
                return fsmEnterState(id, S_ERROR);

            unsigned index = cubeTaskSubstate[id].config.index;
            ASSERT(index < userConfigSize[id]);
            AssetGroupInfo group;
            if (!group.fromAssetConfiguration(userConfig[id] + index))
                return fsmEnterState(id, S_ERROR);

            unsigned offset = cubeTaskSubstate[id].config.offset;
            unsigned bytes = AssetFIFO::fetchFromGroup(*lc, group, offset);
            if (!bytes) {
                // No progress was made. Wait for more FIFO space.
                return;
            }

            // Update current load offset and the progress indicator
            offset += bytes;
            lc->progress += bytes;
            cubeTaskSubstate[id].config.offset = offset;
            resetDeadline(id);

            // Are we done?
            ASSERT(offset <= group.dataSize);
            if (offset < group.dataSize)
                return;

            // Done sending data! Start trying to finish this group.
            return fsmEnterState(id, S_CONFIG_FINISH);
        }

        /*
         * We're done sending AssetGroup data for the current Configuration node.
         * Wait for the FIFO to drain and for the cube to acknowledge all data,
         * then mark this loading operation as complete in SysLFS.
         */
        case S_CONFIG_FINISH: {
            _SYSAssetLoaderCube *lc = AssetUtil::mapLoaderCube(userLoader, id);
            if (!lc)
                return fsmEnterState(id, S_ERROR);

            // Still waiting for FIFO to drain?
            AssetFIFO fifo(*lc);
            if (fifo.readAvailable())
                return;

            // Still waiting on all bytes to be ACK'ed?
            if (cubeBufferAvail[id] != FLS_FIFO_USABLE)
                return;

            // Which slot were we writing to?
            unsigned index = cubeTaskSubstate[id].config.index;
            const _SYSAssetConfiguration *cfg = userConfig[id] + index;
            _SYSAssetSlot slot = cfg->slot;
            if (!VirtAssetSlots::isSlotBound(slot))
                return fsmEnterState(id, S_ERROR);
            VirtAssetSlot &vSlot = VirtAssetSlots::getInstance(slot);

            // And what group did we just finish?
            AssetGroupInfo group;
            if (!group.fromAssetConfiguration(cfg))
                return fsmEnterState(id, S_ERROR);

            // Now we're actually done! Commit this to SysLFS.
            VirtAssetSlots::finalizeSlot(id, vSlot, group);

            // Announce our triumphant advancement
            LOG(("ASSET[%d]: Group [%d/%d] finished in %f seconds\n",
                id, index+1, userConfigSize[id],
                (SysTime::ticks() - groupBeginTimestamp[id]) / double(SysTime::sTicks(1))));

            // Next Configuration node!
            cubeTaskSubstate[id].config.index = index + 1;
            return fsmEnterState(id, S_CONFIG_INIT);
        }

        default:
            return;
    }
}

