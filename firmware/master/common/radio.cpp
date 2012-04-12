/*
 * Thundercracker Firmware -- Confidential, not for redistribution.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#include <string.h>
#include "radio.h"
#include "cube.h"
#include "systime.h"

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
        ASSERT(schedNext < _SYS_NUM_CUBE_SLOTS);
        STATIC_ASSERT(_SYS_NUM_CUBE_SLOTS <= 32);
        
        // No more enabled slots? Loop back to zero.
        if (!(_SYSCubeIDVector)(CubeSlots::vecEnabled << schedNext)) {
            schedNext = 0;

            if (!CubeSlots::vecEnabled) {
                /*
                 * Oh, no enabled slots period.
                 *
                 * XXX: Once we have pairing and transmit throttling, this
                 *      shouldn't happen, since we'd be sending pairing packets,
                 *      and at a lower rate. But for now, this is the state we
                 *      find ourselves in before any cube slots are enabled.
                 *
                 *      So, for now, send a minimal junk packet to.. anywhere.
                 */

                static const RadioAddress addr = {};
                tx.dest = &addr;
                tx.packet.len = 1;
                fifoPush(_SYS_NUM_CUBE_SLOTS - 1);

                return;
            }
        }

        _SYSCubeID id = schedNext++;
        CubeSlot &slot = CubeSlot::getInstance(id);

        if (slot.enabled() && slot.radioProduce(tx)) {
            // Remember this slot in our queue.
            fifoPush(id);
            break;
        }
    }

    DEBUG_LOG(("Radio TX: [%dms] ", (int)(SysTime::ticks() / SysTime::msTicks(1))));
    tx.log();
}

void RadioManager::ackWithPacket(const PacketBuffer &packet)
{
    CubeSlot &slot = CubeSlot::getInstance(fifoPop());

    DEBUG_LOG(("Radio ACK: "));
    packet.log();

    if (slot.enabled())
        slot.radioAcknowledge(packet);
}

void RadioManager::ackEmpty()
{
    // The transmit succeeded, but there was no data in the ACK.
    fifoPop();
}

void RadioManager::timeout()
{
    CubeSlot &slot = CubeSlot::getInstance(fifoPop());

    DEBUG_LOG(("Radio TIMEOUT\n"));

    if (slot.enabled())
        slot.radioTimeout();
}
