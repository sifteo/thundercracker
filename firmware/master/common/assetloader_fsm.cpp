/*
 * Thundercracker Firmware -- Confidential, not for redistribution.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#include <protocol.h>
#include "assetloader.h"
#include "machine.h"
#include "tasks.h"


void AssetLoader::fsmEnterState(_SYSCubeID id, TaskState s)
{
    /*
     * Enter a new specified TaskState.
     */

    cubeTaskState[id] = s;
    switch (s) {

        /*
         * Send a flash reset token, and wait for it to ACK.
         */
        case S_RESET:
            Atomic::ClearLZ(resetAckCubes, id);
            Atomic::SetLZ(resetPendingCubes, id);
            cubeDeadline[id] = SysTime::ticks() + SysTime::msTicks(350);
            Tasks::trigger(Tasks::AssetLoader);
            break;

        /*
         * Done loading! This cube is no longer active.
         */
        case S_COMPLETE:
            Atomic::ClearLZ(activeCubes, id);
            break;
    }
}

void AssetLoader::fsmTaskState(_SYSCubeID id, TaskState s)
{
    /*
     * Perform the Task work for a particular cube and state.
     */

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
            break;

        /*
         * Cube is done. This should never be called, since completed
         * cubes are removed from activeCubes.
         */
        case S_COMPLETE:
            ASSERT(0);
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


}
