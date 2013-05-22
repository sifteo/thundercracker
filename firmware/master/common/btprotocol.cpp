/*
 * Thundercracker Firmware -- Confidential, not for redistribution.
 * Copyright <c> 2013 Sifteo, Inc. All rights reserved.
 */

#include "btprotocol.h"
#include "macros.h"
#include "pause.h"
#include "event.h"
#include "svmmemory.h"
#include <stdint.h>
#include <string.h>

BTProtocol BTProtocol::instance;


bool BTProtocol::setUserQueues(SvmMemory::VirtAddr send, SvmMemory::VirtAddr receive)
{
    return instance.userSendQueue.attach(send) && instance.userReceiveQueue.attach(receive);
}

bool BTProtocol::setUserState(SvmMemory::VirtAddr data, unsigned length)
{
    FlashBlockRef ref;

    ASSERT(length <= _SYS_BT_PACKET_BYTES);

    // Implement me: Atomically store this userspace state in a local buffer.
    //               This probably means double-buffering internally.

    return true;
}

void BTProtocolCallbacks::onConnect()
{
    BTProtocol::instance.flags.atomicClear(BTProtocol::PairingFlag);
    BTProtocol::instance.flags.atomicMark(BTProtocol::ConnectedFlag);
    Event::setBasePending(Event::PID_BASE_BT_CONNECT);
}

void BTProtocolCallbacks::onDisconnect()
{
    BTProtocol::instance.flags.atomicClear(BTProtocol::PairingFlag);
    BTProtocol::instance.flags.atomicClear(BTProtocol::ConnectedFlag);
    Event::setBasePending(Event::PID_BASE_BT_DISCONNECT);
}

void BTProtocolCallbacks::onDisplayPairingCode(const char *code)
{
    // Store the code in our long-lived buffer
    memcpy(BTProtocol::instance.pairingCode, code, BTProtocol::PAIRING_CODE_LEN);

    // Remember that we're in pairing mode until onConnect/onDisconnect happens
    BTProtocol::instance.flags.atomicMark(BTProtocol::PairingFlag);

    // Pause SVM, and display the pairing UI as long as PairingFlag is set.
    Pause::beginBluetoothPairing();
}

void BTProtocolCallbacks::onReceiveData(uint8_t *buffer, unsigned length)
{
    BTProtocol::instance.counters.rxPackets++;
    BTProtocol::instance.counters.rxBytes += length;

    if (length < 1) {
        // Packet is too short. We need at least our one-byte header.
        return;
    }

    // Is this a system packet?
    uint8_t type = buffer[0];
    if (type & BTProtocol::TYPE_SYS) {
        return BTProtocol::instance.handleSystemPacket(type, buffer + 1, length - 1);
    }

    /* 
     * Is there a user pipe with room for this packet?
     */

    BTQueue &queue = BTProtocol::instance.userReceiveQueue;
    if (queue.hasQueue() && queue.writeAvailable()) {

        _SYSBluetoothPacket *dest = queue.reserve();
        dest->length = length - 1;
        dest->type = type;
        memcpy(dest->bytes, buffer + 1, length - 1);

        // Notify userspace that some data has arrived.
        queue.commit();
        return Event::setBasePending(Event::PID_BASE_BT_READ_AVAILABLE);
    }

    // Dropped a user packet on the floor!
    BTProtocol::instance.counters.rxUserDropped++;
}

unsigned BTProtocolCallbacks::onProduceData(uint8_t *buffer)
{
    unsigned result;

    // First priority goes to system packets. Is there anything that needs sending?
    result = BTProtocol::instance.produceSystemPacket(buffer);
    if (!result) {

        /*
         * No system packets. Anything to send from userspace?
         */

        BTQueue &queue = BTProtocol::instance.userSendQueue;
        if (queue.hasQueue() && queue.readAvailable()) {

            _SYSBluetoothPacket *src = queue.peek();

            // Read length exactly once, and securely truncate it.
            unsigned length = src->length;
            length = MIN(length, sizeof src->bytes);
            result = length + 1;
            buffer[0] = src->type & BTProtocol::TYPE_MASK;
            memcpy(buffer + 1, src->bytes, length);

            // Notify userspace that some buffer space is available
            queue.pop();
            Event::setBasePending(Event::PID_BASE_BT_WRITE_AVAILABLE);
        }
    }

    if (result) {
        // Update transmit counters
        BTProtocol::instance.counters.txPackets++;
        BTProtocol::instance.counters.txBytes += result;
    }

    return result;
}

void BTProtocol::task()
{
    // Implement me!
}

void BTProtocol::handleSystemPacket(unsigned type, const uint8_t *data, unsigned length)
{
    /*
     * Called on ISR context to handle incoming "System" packets. The packet's
     * first byte is broken out as 'type', and the remainder of the packet
     * payload is indicated by 'data' and 'length'.
     *
     * Many system packets can be handled synchronously, by setting state and
     * generating the proper packets later in produceSystemPacket().
     *
     * For true asynchronous operations, like filesystem ops, it will be necessary
     * to punt these back to our Task handler, which will eventually give us
     * some data for produceSystemPacket() to return.
     */

    // Implement me!
}

unsigned BTProtocol::produceSystemPacket(uint8_t *buffer)
{
    /*
     * Called on ISR context to ask for system packets that need sending. If any
     * system packets need to be sent, this returns the packet length and populates 'buffer'
     * with a type code and payload. If not, this function returns 0 to yield bandwidth
     * to userspace.
     */

    // Implement me!
    return 0;
}
