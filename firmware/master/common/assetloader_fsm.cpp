/*
 * Thundercracker Firmware -- Confidential, not for redistribution.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#include <protocol.h>
#include "assetloader.h"
#include "machine.h"


void AssetLoader::fsmEnterState(_SYSCubeID id, TaskState s)
{
    cubeTaskState[id] = s;
    switch (s) {

        /*
         * Send a flash reset token, and wait for it to ACK.
         */
        case S_RESET:
            Atomic::ClearLZ(resetAckCubes, id);
            Atomic::SetLZ(resetPendingCubes, id);
            cubeDeadline[id] = SysTime::ticks() + SysTime::msTicks(350);
            break;

        /*
         * Reset is done, loading not yet started.
         */
        case S_IDLE:
            cubeBufferAvail[id] = FLS_FIFO_USABLE;
            break;
    }
}

void AssetLoader::fsmTaskState(_SYSCubeID id, TaskState s)
{
    _SYSCubeIDVector bit = Intrinsic::LZ(id);
    switch (s) {

        /*
         * Waiting for a flash reset, as acknowledged by resetAckCubes bits.
         * If we time out, re-enter the S_RESET state to try again.
         */
        case S_RESET:
            if (resetAckCubes & bit) {
                fsmEnterState(id, S_IDLE);
            } else if (SysTime::ticks() > cubeDeadline[id]) {
                LOG(("FLASH[%d]: Reset timeout\n", id));
                fsmEnterState(id, S_RESET);
            }
            break;

        case S_IDLE:
            break;

    }
}
