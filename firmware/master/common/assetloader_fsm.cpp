/*
 * Thundercracker Firmware -- Confidential, not for redistribution.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#include <protocol.h>
#include "assetloader.h"
#include "assetslot.h"
#include "assetutil.h"
#include "machine.h"
#include "tasks.h"
#include "flash_syslfs.h"


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
        case S_RESET:
            Atomic::ClearLZ(resetAckCubes, id);
            Atomic::SetLZ(resetPendingCubes, id);
            return;

        /*
         * Done loading! This cube is no longer active.
         */
        case S_COMPLETE:
            Atomic::ClearLZ(activeCubes, id);
            updateActiveCubes();
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

    LOG(("ASSET[%d]: Loader task state %d\n", id, s));

    ASSERT(id < _SYS_NUM_CUBE_SLOTS);
    _SYSCubeIDVector bit = Intrinsic::LZ(id);

    switch (s) {

        /*
         * Just sent a reset token. Before we go to S_RESET_WAIT state,
         * take advantage of this otherwise-wasted time to do some local
         * preparation work.
         */
        case S_RESET:
            resetDeadline(id);
            fsmEnterState(id, S_RESET_WAIT);
            return prepareCubeForLoading(id);

        /*
         * Waiting for a flash reset, as acknowledged by resetAckCubes bits.
         * If we time out, re-enter the S_RESET state to try again.
         */
        case S_RESET_WAIT:
            if (resetAckCubes & bit) {
                // Done with reset! We know the device's FIFO is empty now.
                cubeBufferAvail[id] = FLS_FIFO_USABLE;

                // Cache not consistent? Check it out first.
                if (0 == (bit & cacheCoherentCubes)) {
                    cubeTaskSubstate[id].crc.remaining = 0xFFFFFFFF << (32 - SysLFS::ASSET_SLOTS_PER_CUBE);
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
                uint32_t remaining = cubeTaskSubstate[id].crc.remaining;
                ASSERT(remaining);
                unsigned slot = Intrinsic::CLZ(remaining);
                unsigned tiles = AssetUtil::totalTilesForPhysicalSlot(id, slot);
                if (!tiles) {
                    // Empty slot? Consider this successful, try the next.
                    remaining ^= Intrinsic::LZ(slot);
                    cubeTaskSubstate[id].crc.remaining = remaining;
                    if (remaining)
                        continue;

                    // Done! Start allocating the first configuration.
                    Atomic::Or(cacheCoherentCubes, bit);
                    cubeTaskSubstate[id].value = 0;
                    return fsmEnterState(id, S_CONFIG_INIT);
                }

                // XXX: Send query
                ASSERT(0);
            }

        /*
         * After all loading has finished, finalize any SysLFS state associated
         * with this cube. This marks any in-progress groups as complete.
         */
        case S_FINALIZE:
            // XXX
            return fsmEnterState(id, S_COMPLETE);

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
                    return fsmEnterState(id, S_FINALIZE);
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
                if (!VirtAssetSlots::locateGroup(group, bit, foundCV, &vSlot, &slotsInProgress)) {
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
            if (fifo.writeAvailable() >= 3) {
                unsigned baseAddr = AssetUtil::loadedBaseAddr(group.va, id);

                // Opcode, lat1, lat2:a21
                ASSERT(baseAddr < 0x8000);
                fifo.write(0xe1);
                fifo.write(baseAddr << 1);
                fifo.write(((baseAddr >> 6) & 0xfe) | ((baseAddr >> 14) & 1));
                fifo.commitWrites();

                resetDeadline(id);
                cubeTaskSubstate[id].config.offset = 0;
                return fsmEnterState(id, S_CONFIG_DATA);
            }
            return;
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

            // Next Configuration node!
            cubeTaskSubstate[id].config.index = index + 1;
            return fsmEnterState(id, S_CONFIG_INIT);
        }

        default:
            return;
    }
}

