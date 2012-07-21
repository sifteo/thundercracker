/*
 * Thundercracker Firmware -- Confidential, not for redistribution.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#include <protocol.h>
#include "macros.h"
#include "cubeconnector.h"
#include "neighbor_tx.h"
#include "neighbor_protocol.h"
#include "systime.h"
#include "prng.h"

RadioAddress CubeConnector::pairingAddr = { 0, RF_PAIRING_ADDRESS };
_SYSPseudoRandomState CubeConnector::prng;
RingBuffer<RadioManager::FIFO_DEPTH, uint8_t, uint8_t> CubeConnector::rxState;

uint8_t CubeConnector::neighborKey;
uint8_t CubeConnector::txState;
uint8_t CubeConnector::pairingPacketCounter;
uint8_t CubeConnector::pairingHWID[HWID_LEN];


void CubeConnector::init()
{
    // Seed our PRNG with an unguessable number, unique to this system
    Crc32::reset();
    Crc32::addUniqueness();
    PRNG::init(&prng, Crc32::get());

    // Set a default master ID
    neighborKey = ~0;
    nextNeighborKey();
    ASSERT(neighborKey < Neighbor::NUM_MASTER_ID);

    // State machine init
    txState = PairingFirstContact;
    rxState.init();
}

void CubeConnector::setNeighborKey(unsigned k)
{
    /*
     * Set the random 3-bit key which determines our neighbor ID and pairing channel.
     */

    ASSERT(k < Neighbor::NUM_MASTER_ID);
    neighborKey = k;

    unsigned idByte = Neighbor::FIRST_MASTER_ID + k;
    NeighborTX::start((idByte << 8) | uint8_t(~idByte << 3), ~0);

    static const uint8_t channels[] = RF_PAIRING_CHANNELS;
    pairingAddr.channel = channels[k];
}

void CubeConnector::nextNeighborKey()
{
    /*
     * Choose a random neighbor key, using any value other than the current key.
     * Use entropy from the current time, the system-unique ID, and all previous calls
     * to this same function.
     *
     * We're careful that this value is hard to guess as well as uniformly distributed,
     * so we make the best use of our very limited ID space.
     */

    SysTime::Ticks now = SysTime::ticks();
    PRNG::init(&prng, PRNG::value(&prng) ^ uint32_t(now ^ (now >> 32)));

    unsigned newKey;
    if (neighborKey < Neighbor::NUM_MASTER_ID) {
        // Replacing a valid key. Avoid picking the same one.
        newKey = PRNG::valueBounded(&prng, Neighbor::NUM_MASTER_ID - 2);
        if (newKey >= neighborKey)
            newKey++;
    } else {
        // Not replacing any key. Choose any one.
        newKey = PRNG::valueBounded(&prng, Neighbor::NUM_MASTER_ID - 1);
    }

    setNeighborKey(newKey);
}

void CubeConnector::radioProduce(PacketTransmission &tx)
{
    rxState.enqueue(txState);
    switch (txState) {

        /*
         * Our first chance to talk to a new cube who's in range of our
         * neighbor transmitters. If it detects the beacon we're sending
         * out with our current neighbor key, it will be listening on the
         * corresponding channel.
         *
         * We just send a mininal ping packet, since we (a) don't want to
         * waste the time sending a longer packet that will most likely be
         * ignored, and (b) we don't want to hop until we've confirmed that
         * we can talk to the cube and that it's physically neighbored.
         *
         * We periodically hop to the next neighbor key, not because we need
         * to for verification purposes, but to help us avoid spamming any
         * single radio frequency too badly. We do this based on the number
         * of packets sent since the last key hop, so the hop rate scales
         * with our transmit rate.
         *
         * Right now we just hop every time an 8-bit packet counter overflows.
         * At the fastest, this equates to about one hop per second.
         */
        case PairingFirstContact:
            if (!++pairingPacketCounter) {
                nextNeighborKey();
            }
            tx.dest = &pairingAddr;
            tx.packet.len = 1;
            tx.packet.bytes[0] = 0xff;
            tx.numSoftwareRetries = 0;
            tx.numHardwareRetries = 0;
            break;

        /*
         * After establishing first contact, we gain some trust that we're
         * talking to a physically-neighbored cube by performing a sequence
         * of channel hops and waiting for the cube to follow. This would
         * be easy to spoof, but the chances of randomly "stealing" another
         * base's pairing event becomes extremely low.
         *
         * The ping packet we transmit is identical to first contact, except
         * we do want a small amount of retry. Not too much, or we open the
         * window for false positives. Currently the defaults seem fine.
         */
        case PairingFirstVerify ... PairingFinalVerify:
            tx.dest = &pairingAddr;
            tx.packet.len = 1;
            tx.packet.bytes[0] = 0xff;
            break;

    };
}

void CubeConnector::radioAcknowledge(const PacketBuffer &packet)
{
    RF_ACKType *ack = (RF_ACKType *) packet.bytes;
    unsigned packetRxState = rxState.dequeue();
    switch (packetRxState) {

        /*
         * When we get a response to the first packet, start
         * a sequence of channel hops which will allow us to verify that
         * this cube is neighbored with us and not a different base.
         *
         * Store the HWID, so we can check it during each verify.
         */
        case PairingFirstContact:
            nextNeighborKey();
            if (packet.len >= RF_ACK_LEN_HWID) {
                memcpy(pairingHWID, ack->hwid, sizeof pairingHWID);
                txState = packetRxState + 1;
            } else {
                txState = PairingFirstContact;
            }
            break;

        /*
         * For each verification stage, check the HWID and proceed to the
         * next stage if everything looks okay.
         */
        case PairingFirstVerify ... PairingFinalVerify:
            nextNeighborKey();
            if (packet.len >= RF_ACK_LEN_HWID && !memcmp(pairingHWID, ack->hwid, sizeof pairingHWID)) {
                txState = packetRxState + 1;
            } else {
                txState = PairingFirstContact;
            }
            break;

    }
}

void CubeConnector::radioTimeout()
{
    unsigned packetRxState = rxState.dequeue();
    switch (packetRxState) {

        /*
         * In all other cases, abort what we were doing and go back to
         * the default first-contact state.
         */
        default:
            txState = PairingFirstContact;
            break;
    }
}

void CubeConnector::radioEmptyAcknowledge()
{
    /*
     * Empty ACKs don't really mean anything to us, since a disconnected
     * cube should always be sending us a full ACK packet (which we need
     * in order to verify its identity). So, just dequeue the state and
     * don't act on it.
     */
    rxState.dequeue();
}
