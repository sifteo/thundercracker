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
     */

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
            cubeDeadline[id] = SysTime::ticks() + SysTime::msTicks(350);
            return;

        /*
         * Done loading! This cube is no longer active.
         */
        case S_COMPLETE:
            Atomic::ClearLZ(activeCubes, id);
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

    switch (s) {

        /*
         * Waiting for a flash reset, as acknowledged by resetAckCubes bits.
         * If we time out, re-enter the S_RESET state to try again.
         */
        case S_RESET:
            if (resetAckCubes & bit) {
                // Done with reset! We know the device's FIFO is empty now.
                cubeBufferAvail[id] = FLS_FIFO_USABLE;
                fsmNextConfigurationStep(id);
                ASSERT(cubeTaskState[id] != S_RESET);

            } else if (SysTime::ticks() > cubeDeadline[id]) {
                LOG(("FLASH[%d]: Reset timeout\n", id));
                fsmEnterState(id, S_RESET);

            } else {
                // Keep waiting
                Tasks::trigger(Tasks::AssetLoader);
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
                uint32_t remaining = cubeTaskSubstate[id];
                ASSERT(remaining);
                unsigned slot = Intrinsic::CLZ(remaining);
                unsigned tiles = AssetUtil::totalTilesForPhysicalSlot(id, slot);
                if (!tiles) {
                    // Empty slot? Consider this successful, try the next.
                    remaining ^= Intrinsic::LZ(slot);
                    cubeTaskSubstate[id] = remaining;
                    if (remaining)
                        continue;

                    // Done!
                    Atomic::Or(cacheCoherentCubes, bit);
                    fsmNextConfigurationStep(id);
                    return;
                }

                // XXX: Send query
                ASSERT(0);
            }

        default:
            break;
    }
}

void AssetLoader::fsmNextConfigurationStep(_SYSCubeID id)
{
    /*
     * Begin the next step in trying to apply the chosen AssetConfiguration on one cube.
     *
     * This scans the AssetConfiguration and the current state of the cube, and takes
     * the appropriate combination of immediate action and state transitions.
     */

    ASSERT(id < _SYS_NUM_CUBE_SLOTS);
    _SYSCubeIDVector bit = Intrinsic::LZ(id);

    // Cache not consistent? Check it out first.
    if (0 == (bit & cacheCoherentCubes)) {
        cubeTaskSubstate[id] = 0xFFFFFFFF << (32 - SysLFS::ASSET_SLOTS_PER_CUBE);
        return fsmEnterState(id, S_CRC_COMMAND);
    }

    LOG(("XXX\n"));
}
