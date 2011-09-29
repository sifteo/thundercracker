/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * This file is part of the internal implementation of the Sifteo SDK.
 * Confidential, not for redistribution.
 *
 * Copyright <c> 2011 Sifteo, Inc. All rights reserved.
 */

#include <string.h>
#include "radio.h"
#include "cube.h"

_SYSCubeID RadioManager::epFifo[RadioManager::FIFO_SIZE];
uint8_t RadioManager::epHead;
uint8_t RadioManager::epTail;
_SYSCubeID RadioManager::schedNext;


void RadioManager::produce(PacketTransmission &tx)
{
    /*
     * Currently we just try all cubes in round-robin order until we
     * find one that will give us a packet.
     *
     * XXX: We don't have a fallback when there are zero enabled slots
     *
     * XXX: This isn't the most efficient use of our time, especially
     *      if all cubes are idling and don't need to transmit packets
     *      at the full rate. We should be able to throttle down the
     *      transmit rate in these cases.
     */

    for (;;) {
        // No more enabled slots? Loop back to zero.
        if (!(_SYSCubeIDVector)(CubeSlot::vecEnabled << schedNext))
            schedNext = 0;

        _SYSCubeID id = schedNext++;
        CubeSlot &slot = CubeSlot::instances[id];

        if (slot.enabled() && slot.radioProduce(tx)) {
            // Remember this slot in our queue.
            fifoPush(id);
            break;
        }
    }
}

void RadioManager::acknowledge(const PacketBuffer &packet)
{
    CubeSlot &slot = CubeSlot::instances[fifoPop()];

    if (slot.enabled())
        slot.radioAcknowledge(packet);
}

void RadioManager::timeout()
{
    CubeSlot &slot = CubeSlot::instances[fifoPop()];

    if (slot.enabled())
        slot.radioTimeout();
}
