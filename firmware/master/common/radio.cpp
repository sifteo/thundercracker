/*
 * Thundercracker Firmware -- Confidential, not for redistribution.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#include <string.h>
#include "radio.h"
#include "cube.h"
#include "cubeconnector.h"

#ifdef RADIO_UART_TRACE
#   define RADIO_UART_STR(_x)   UART(_x)
#   define RADIO_UART_HEX(_i)   UART_HEX(_i)
#else
#   define RADIO_UART_STR(_x)
#   define RADIO_UART_HEX(_i)
#endif

RadioManager::fifo_t RadioManager::fifo;
bool RadioManager::enabled;
uint8_t RadioManager::nextPID;
uint32_t RadioManager::schedule[RadioManager::PID_COUNT];
uint32_t RadioManager::nextSchedule[RadioManager::PID_COUNT];


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
     * To avoid this, our scheduling algorithm has been designed
     * to specifically select only producers which would not cause
     * such a collision.
     *
     * We do this by maintaining four separate schedule queues, one
     * for each of the four possible PID values. The queue we put
     * a producer into is dictated by the PID that producer had
     * last time we gave it a transmit opportunity. Because we
     * have these four queues, we actually have no need to store
     * this last PID explicitly.
     *
     * At every transmit opportunity, we can ask any producer for
     * a packet as long as it's in a schedule *other* than the one
     * corresponding to the last PID we transmitted.
     *
     * We'd be stuck if the only pending producers were located on
     * the one queue corresponding to this last PID. If this does
     * happen, we have no choice but to transmit a dummy packet
     * or skip the producer for the current scheduling cycle. However,
     * we take a preventative measure by always choosing a producer
     * from the per-PID queue which the greatest number of items in it.
     * This way, we reduce our chances of using up a producer we
     * would really need to use later on in the cycle.
     *
     * This is clearly not a globally optimal algorithm, but it should
     * yield an optimal-enough solution in all cases, and it needs
     * to be efficient enough to run on every radio ISR :)
     */

    const uint32_t activeMask = CubeSlots::sysConnected | Intrinsic::LZ(CONNECTOR_ID);
    const SysTime::Ticks now = SysTime::ticks();

    for (;;) {

        /*
         * Look for the most populous queue other than the
         * one corresponding to our nextPID, ignoring empty
         * queues.
         *
         * While we're in here, we clear inactive producers from the schedule.
         */

        const unsigned NOT_FOUND = -1;
        unsigned thisPID = nextPID;
        unsigned foundPID = 0;
        unsigned foundCount = 0;

        for (unsigned i = 0; i < PID_COUNT; ++i) {
            if (i != thisPID) {
                uint32_t masked = activeMask & schedule[i];
                schedule[i] = masked;
                unsigned count = Intrinsic::POPCOUNT(masked);
                if (count > foundCount) {
                    foundPID = i;
                    foundCount = count;
                }
            }
        }

        /*
         * If we didn't find anything, it could be because
         * all of our queues are empty and we just need to
         * roll over nextSchedule to schedule. Or it could
         * be that all of our remaining producers would cause
         * a collision!
         *
         * In the latter case, we need to insert a dummy packet
         * in order to skip the offending PID.
         */

        if (foundCount == 0) {
            if (schedule[thisPID] == 0) {

                /*
                 * Roll-over items from 'nextSchedule' to current schedule.
                 * This is how we carry forward our notion of the last PID
                 * that each producer transmitted on.
                 *
                 * Producers for cubes which are no longer connected will
                 * automatically drop from this list when we process the schedule.
                 * Adds, however, need to be considered explicitly. We do that
                 * here. Any items which should be in the schedule but aren't
                 * are added to an arbitrary PID's queue.
                 */

                uint32_t added = activeMask;
                for (unsigned i = 0; i < PID_COUNT; ++i) {
                    uint32_t s = nextSchedule[i];
                    nextSchedule[i] = 0;
                    schedule[i] = s;
                    added &= ~s;
                }
                schedule[0] |= added;
                continue;
            }

            /*
             * Send a dummy packet to an address that is never valid,
             * just to force the radio to bump its PID.
             */

            static const RadioAddress dummy = { 0, { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF } };

            tx.dest = &dummy;
            tx.packet.bytes[0] = 0xFF;
            tx.packet.len = 1;
            tx.numSoftwareRetries = 0;
            tx.numHardwareRetries = 0;

            nextPID = (thisPID + 1) & PID_MASK;
            fifo.enqueue(DUMMY_ID);
            return;
        }

        /*
         * Now extract the next producer from our chosen priority queue,
         * and re-add it in its new location, if applicable.
         */

        ASSERT(schedule[foundPID]);
        unsigned producer = Intrinsic::CLZ(schedule[foundPID]);
        ASSERT(producer < NUM_PRODUCERS);
        uint32_t producerBit = Intrinsic::LZ(producer);

        // Always remove from current schedule
        schedule[foundPID] ^= producerBit;

        // Does this producer even want to transmit right now?
        if (dispatchProduce(producer, tx, now)) {
            nextSchedule[thisPID] |= producerBit;
            nextPID = (thisPID + 1) & PID_MASK;
            fifo.enqueue(producer);
            return;
        }

        // If not, put it back with its existing PID but in the new schedule
        nextSchedule[foundPID] |= producerBit;
    }
}

void RadioManager::ackWithPacket(const PacketBuffer &packet)
{
    dispatchAcknowledge(fifo.dequeue(), packet);
}

void RadioManager::ackEmpty()
{
    dispatchEmptyAcknowledge(fifo.dequeue());
}

void RadioManager::timeout()
{
    dispatchTimeout(fifo.dequeue());
}

ALWAYS_INLINE bool RadioManager::dispatchProduce(unsigned id, PacketTransmission &tx, SysTime::Ticks now)
{
    RADIO_UART_STR("\r\ntx ");
    RADIO_UART_HEX(id);

    ASSERT(id < NUM_PRODUCERS);

    if (id == CONNECTOR_ID) {
        CubeConnector::radioProduce(tx);
        return true;
    }

    CubeSlot &slot = CubeSlot::getInstance(id);
    return slot.isSysConnected() && slot.radioProduce(tx, now);
}

ALWAYS_INLINE void RadioManager::dispatchAcknowledge(unsigned id, const PacketBuffer &packet)
{
    RADIO_UART_STR("\r\nack ");
    RADIO_UART_HEX(id);

    if (id == CONNECTOR_ID)
        return CubeConnector::radioAcknowledge(packet);

    if (id >= NUM_PRODUCERS) {
        ASSERT(id == DUMMY_ID);
        return;
    }

    CubeSlot &slot = CubeSlot::getInstance(id);
    if (slot.isSysConnected())
        slot.radioAcknowledge(packet);
}

ALWAYS_INLINE void RadioManager::dispatchEmptyAcknowledge(unsigned id)
{
    RADIO_UART_STR("\r\nack0 ");
    RADIO_UART_HEX(id);

    // Cubes don't care, only the connector.
    if (id == CONNECTOR_ID)
        return CubeConnector::radioEmptyAcknowledge();
}

ALWAYS_INLINE void RadioManager::dispatchTimeout(unsigned id)
{
    RADIO_UART_STR("\r\nTIMEOUT ");
    RADIO_UART_HEX(id);

    if (id == CONNECTOR_ID)
        return CubeConnector::radioTimeout();

    if (id >= NUM_PRODUCERS) {
        ASSERT(id == DUMMY_ID);
        return;
    }

    CubeSlot &slot = CubeSlot::getInstance(id);
    if (slot.isSysConnected())
        slot.radioTimeout();
}
