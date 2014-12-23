/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * Thundercracker firmware
 *
 * Copyright <c> 2013 Sifteo, Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include "btprotocol.h"
#include "macros.h"
#include "pause.h"
#include "event.h"
#include "svmmemory.h"
#include "svmloader.h"
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
    BTProtocol::instance.sysData.setLength(0);
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
    if (instance.flags.test(ReportVolumeFlag)) {
        instance.flags.atomicClear(ReportVolumeFlag);
        instance.captureCurrentVolume();
        return;
    }

    // more handlers here...
}

void BTProtocol::captureCurrentVolume()
{
    uint32_t len = Elf::Program::copyMeta(SvmLoader::getRunningVolume(),
                                          _SYS_METADATA_PACKAGE_STR,
                                          1,
                                          sizeof sysData.payload,
                                          sysData.payload);
    if (len) {
        sysData.type = CurrentApp;
        sysData.setLength(len);
        BTProtocolHardware::requestProduceData();
    }
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

    if (sysData.bytesToWrite()) {
        return sysData.writeTo(buffer);
    }

    // nothing to send
    return 0;
}
