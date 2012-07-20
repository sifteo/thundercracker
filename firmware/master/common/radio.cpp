/*
 * Thundercracker Firmware -- Confidential, not for redistribution.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#include <string.h>
#include "radio.h"
#include "cube.h"
#include "cubeconnector.h"

RadioManager::fifo_t RadioManager::fifo;
uint8_t RadioManager::nextPID;
uint8_t RadioManager::lastPID[RadioManager::NUM_PRODUCERS];
BitVector<RadioManager::NUM_PRODUCERS> RadioManager::schedule;


void RadioManager::produce(PacketTransmission &tx)
{
    /*
     * Produce the next radio packet to transmit. This module's
     * responsibility is to multiplex our available radio bandwidth
     * between all enabled CubeSlots and the CubeConnector.
     *
     * To ensure fair allocation of bandwidth, each of the possible
     * packet producers are given transmit opportunities in a
     * round-robin fashion. Each of these producers may give up their
     * transmit slot by returning false from radioProduce(), with the
     * exception of CubeConnector.
     *
     * To further complicate matters, we need to sequence our
     * transmissions to avoid exposing an nRF protocol limitation:
     * Each packet includes a 2-bit sequence number, used to detect
     * and discard duplicate packets when we retransmit after a
     * dropped ACK. The protocol was designed to assume each PTX
     * is transmitting to a single receiver at a time. But when
     * you multiplex one PTX among several receivers as we're doing,
     * there is still only a single global 2-bit counter for the
     * PTX. In the worst case, if we have exactly four cubes in
     * our round-robin rotation, we'll hit a duplicate packet ID
     * every time and every packet will be dropped!
     *
     * To avoid this, we explicitly keep track of the ID that was
     * used for the last transmission by a particular producer
     * and we'll reorder our round-robin rotation to avoid ID
     * collisions.
     */

    // Producers we're temporarily skipping due to an ID collision
    BitVector<RadioManager::NUM_PRODUCERS> deferred;
    deferred.clear();

    for (;;) {
        unsigned id;

        if (!schedule.clearFirst(id)) {
            // No more producers in the schedule.

            if (deferred.empty()) {
                // Start the next round-robin cycle

                schedule.words[0] = CubeSlots::vecEnabled | Intrinsic::LZ(CONNECTOR_ID);

            } else {
                // We still have deferred producers that haven't transmitted.
                // This means every single producer in the schedule was deferred!
                // This can easily happen if we have exactly one producer left, and
                // it happened to land on an unlucky PID. Try transmitting anyway.
                // If the packet is dropped, oh well.
                //
                // (Note that currently this state also occurs immediately after
                // initialization, because all of our PIDs are cleared to zero.
                // So, we have some test coverage for this code.)

                schedule = deferred;
                nextPID = (nextPID + 1) & PID_MASK;
                deferred.clear();
            }

            ASSERT(!schedule.empty());
            continue;
        }

        // We have a candidate producer, but would it cause an ID collision?
        ASSERT(id < NUM_PRODUCERS);
        if (nextPID == lastPID[id]) {
            deferred.mark(id);
            continue;
        }

        // Does this producer even want to transmit right now?
        if (dispatchProduce(id, tx)) {
            // Yes, we're transmitting a packet! Update state and exit.
            lastPID[id] = nextPID;
            nextPID = (nextPID + 1) & PID_MASK;
            schedule.words[0] |= deferred.words[0];
            fifo.enqueue(id);
            return;
        }
    }
}

void RadioManager::ackWithPacket(const PacketBuffer &packet)
{
    dispatchAcknowledge(fifo.dequeue(), packet);
}

void RadioManager::ackEmpty()
{
    // The transmit succeeded, but there was no data in the ACK.
    fifo.dequeue();
}

void RadioManager::timeout()
{
    dispatchTimeout(fifo.dequeue());
}

bool RadioManager::dispatchProduce(unsigned id, PacketTransmission &tx)
{
    if (id == CONNECTOR_ID) {
        CubeConnector::radioProduce(tx);
        return true;
    }

    CubeSlot &slot = CubeSlot::getInstance(id);
    return slot.enabled() && slot.radioProduce(tx);
}

void RadioManager::dispatchAcknowledge(unsigned id, const PacketBuffer &packet)
{
    if (id == CONNECTOR_ID)
        return CubeConnector::radioAcknowledge(packet);

    CubeSlot &slot = CubeSlot::getInstance(id);
    if (slot.enabled())
        slot.radioAcknowledge(packet);
}

void RadioManager::dispatchTimeout(unsigned id)
{
    if (id == CONNECTOR_ID)
        return CubeConnector::radioTimeout();

    CubeSlot &slot = CubeSlot::getInstance(id);
    if (slot.enabled())
        slot.radioTimeout();
}
