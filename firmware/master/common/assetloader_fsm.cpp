/*
 * Thundercracker Firmware -- Confidential, not for redistribution.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#include <protocol.h>
#include "assetloader.h"
#include "machine.h"


void AssetLoader::fsmEnterState(_SYSCubeID id, TaskState s)
{
    LOG(("FLASH[%d]: Enter State %d\n", id, s));

    cubeTaskState[id] = s;
    switch (s) {

        /*
         * Send a flash reset token, and wait for it to ACK.
         */
        case S_RESET:
            Atomic::ClearLZ(resetAckCubes, id);
            Atomic::SetLZ(resetPendingCubes, id);
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
    LOG(("FLASH[%d]: Poll state %d\n", id, s));
    switch (s) {

        case S_RESET:
            break;

        case S_IDLE:
            break;

    }
}

