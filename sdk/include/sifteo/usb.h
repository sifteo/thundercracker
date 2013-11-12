/*
 * This file is part of the public interface to the Sifteo SDK.
 * Copyright <c> 2013 Sifteo, Inc. All rights reserved.
 */

#pragma once
#ifdef NOT_USERSPACE
#   error This is a userspace-only header, not allowed by the current build.
#endif

#include <sifteo/abi.h>

namespace Sifteo {

/**
 * @defgroup usb USB
 *
 * @brief Support for communicating with a host device via USB.
 *
 * @{
 */


/**
 * @brief Global USB operations.
 *
 * The system provides a basic level of USB functionality without any
 * game-specific code. This includes filesystem access, pairing the base with
 * a Sifteo user account, getting information about the running game, and
 * launching a game.
 */

class Usb {
public:

    /**
     * @brief Is a device currently connected to a host via USB?
     *
     * Returns 'true' if a device is connected right now, 'false' if not.
     *
     * To get notified when a connection is created or destroyed, you can
     * attach handlers to Events::usbConnect and Events::usbDisconnect.
     */

    static bool isConnected()
    {
        return _SYS_usb_isConnected();
    }
};


/**
 * @brief A container for one Bluetooth packet's data.
 *
 * Each packet consists of a variable-length payload of up to 19 bytes, and
 * a "type" byte of which the low 7 bits are available for games to use.
 */

struct UsbPacket {
    _SYSUsbPacket sys;

    // Implicit conversions
    operator _SYSUsbPacket* () { return &sys; }
    operator const _SYSUsbPacket* () const { return &sys; }

    /// Retrieve the capacity of a packet, in bytes. (19)
    static unsigned capacity() {
        return _SYS_USB_PACKET_BYTES;
    }

    /// Set the packet's length and type to zero.
    void clear() {
        resize(0);
        setType(0);
    }

    /// Retrieve the size of this packet, in bytes.
    unsigned size() const {
        return sys.length;
    }

    /// Change the size of this packet, in bytes.
    void resize(unsigned bytes) {
        ASSERT(bytes <= capacity());
        sys.length = bytes;
    }

    /// Is this packet's payload empty?
    bool empty() const {
        return size() == 0;
    }

    /// Return a pointer to this packet's payload bytes.
    uint8_t *bytes() {
        return sys.bytes;
    }
    const uint8_t *bytes() const {
        return sys.bytes;
    }

    /// Return the packet's 7-bit type code
    unsigned type() const {
        return sys.type;
    }

    /// Set a packet's 7-bit type.
    void setType(unsigned type) {
        ASSERT(type <= 127);
        sys.type = type;
    }
};


/**
 * @brief A memory buffer which holds a queue of Bluetooth packets.
 *
 * This is a FIFO buffer which holds packets that the game has created
 * but is waiting to transmit, or packets which have been received by the
 * system but not yet processed by the game.
 *
 * This is a single-producer single-consumer lock-free queue. Typically
 * your application is either a producer or a consumer, communicating
 * via this queue with system software that runs in the background.
 *
 * UsbQueues are templatized by buffer size. The system supports
 * buffers with between 1 and 255 packets of capacity. You can pick a
 * buffer size based on how many packets you expect may arrive between
 * each opportunity you have to check on the queue. 
 */

template < unsigned tCapacity >
struct UsbQueue {
    struct SysType {
        union {
            _SYSIoQueueHeader header;
            uint32_t header32;
        };
        _SYSUsbPacket packets[tCapacity + 1];
    } sys;

    // Implicit conversions
    operator _SYSUsbQueue* () { return reinterpret_cast<_SYSUsbQueue*>(&sys); }
    operator const _SYSUsbQueue* () const { return reinterpret_cast<const _SYSUsbQueue*>(&sys); }

    /// Initializes this queue's header, and marks it as empty.
    void clear()
    {
        unsigned lastIndex = tCapacity;

        STATIC_ASSERT(sizeof sys.header == sizeof sys.header32);
        STATIC_ASSERT(lastIndex >= 1);
        STATIC_ASSERT(lastIndex <= 255);

        sys.header32 = lastIndex << 16;

        ASSERT(sys.header.head == 0);
        ASSERT(sys.header.tail == 0);
        ASSERT(sys.header.last == lastIndex);
    }

    /**
     * @brief How many packets are sitting in the queue right now?
     *
     * Note that the system and your application may be updating the queue
     * concurrently. This is a single-producer single-consumer lock-free FIFO.
     * If you're writing to the queue, you can be sure that the number of
     * packets in the queue won't increase without your knowledge. Likewise,
     * if you're the one reading from the queue, you can be sure that the number
     * of packets won't decrease without your knowledge.
     */
    unsigned count() const
    {
        unsigned size = tCapacity + 1;
        unsigned head = sys.header.head;
        unsigned tail = sys.header.tail;
        ASSERT(head < size);
        ASSERT(tail < size);
        return (tail - head) % size;
    }

    /// How many packets are available to read from this queue right now?
    unsigned readAvailable() const
    {
        return count();
    }

    /// How many free buffers are available for writing right now?
    unsigned writeAvailable() const
    {
        return tCapacity - count();
    }

    /**
     * @brief Read the oldest queued packet into a provided buffer.
     *
     * Must only be called if readAvailable() returns a nonzero value.
     * If no packets are available, try again later or use an Event to
     * get notified when Bluetooth data arrives.
     *
     * This function copies the data. To read data without copying,
     * use peek() and pop().
     */
    void read(UsbPacket &buffer)
    {
        buffer = peek();
        pop();
    }

    /**
     * @brief Access the oldest queued packet without copying it.
     *
     * Must only be called if readAvailable() returns a nonzero value.
     * If no packets are available, try again later or use an Event to
     * get notified when Bluetooth data arrives.
     *
     * This returns a reference to the oldest queued packet in the
     * buffer. This can be used to inspect a packet before read()'ing it,
     * or as a zero-copy alternative to read().
     */
    const UsbPacket &peek() const
    {
        ASSERT(readAvailable() > 0);
        unsigned head = sys.header.head;
        ASSERT(head <= tCapacity);
        return *reinterpret_cast<const UsbPacket *>(&sys.packets[head]);
    }

    /**
     * @brief Dequeue the oldest packet.
     *
     * This can be called after peek() to remove the packet from our queue.
     * Must only be called if readAvailable() returns a nonzero value.
     */
    void pop()
    {
        ASSERT(readAvailable() > 0);
        unsigned head = sys.header.head + 1;
        if (head > tCapacity)
            head = 0;
        sys.header.head = head;

        /*
         * Notify the system that we have more space for reading. This may
         * communicate additional flow control tokens to our peer if this
         * queue is the current read pipe.
         *
         * This is only absolutely necessary if the queue transitioned
         * from being totally full to being less-than-totally full, but
         * we'd also like to give the system a chance to send flow control
         * tokens speculatively if it wants.
         */
//        _SYS_bt_queueReadHint();
    }

    /**
     * @brief Copy a new packet into the queue, from a provided buffer.
     *
     * Must only be called if writeAvailable() returns a nonzero value.
     * If there's no space in the buffer, try again later or use an Event
     * to get notified when more space is available.
     *
     * This function copies the data. To write data without copying,
     * use reserve() and commit().
     */
    void write(const UsbPacket &buffer)
    {
        reserve() = buffer;
        commit();
    }

    /**
     * @brief Access a buffer slot where a new packet can be written.
     *
     * Must only be calld if writeAvailable() returns a nonzero value.
     * If there's no space in the buffer, try again later or use an Event
     * to get notified when more space is available.
     *
     * This "reserves" space for a new packet, and returns a reference
     * to that space. This doesn't actually affect the state of the queue
     * until commit(), at which point the written packet becomes visible
     * to consumers.
     */
    UsbPacket &reserve()
    {
        ASSERT(writeAvailable() > 0);
        unsigned tail = sys.header.tail;
        ASSERT(tail <= tCapacity);
        return *reinterpret_cast<UsbPacket *>(&sys.packets[tail]);
    }

    /**
     * @brief Finish writing a packet that was started with reserve()
     *
     * When this function finishes, the new packet will be visible to
     * consumers on this queue.
     */
    void commit()
    {
        ASSERT(writeAvailable() > 0);
        unsigned tail = sys.header.tail + 1;
        if (tail > tCapacity)
            tail = 0;
        sys.header.tail = tail;

        /*
         * Poke the system to look at our queue, in case it's ready to
         * transmit immediately. Has no effect if this queue isn't
         * attached as the current TX pipe.
         *
         * This wakeup is only necessary when the queue transitions
         * from empty to full, but we have no way of knowing for sure when
         * this happens. If we see that the queue is non-empty when we
         * commit(), there's no way to know that the system hasn't dequeued
         * the packet during this function's execution.
         */
        _SYS_usb_queueWriteHint();
    }
};


/**
 * @brief A memory buffer for bidirectional USB communications.
 *
 * This is a pair of FIFO buffers which can be used for bidirectional
 * communication over USB. When the pipe is "attached", this
 * Base becomes available to send and receive application-specific
 * packets over USB.
 *
 * When the pipe is not attached or no pipe has been allocated,
 * the Sifteo USB API will throw an error if application-specific
 * packets are sent. However the USB API provides generic functionality
 * which can be used even without a UsbPipe.
 *
 * If the UsbPipe's receiveQueue is full, the Sifteo USB API will
 * not be allowed to transmit more packets until our receiveQueue has room.
 * Packets will not be dropped if the buffer fills up.
 *
 * Some applications may desire lower-level access to the data queues
 * we use. Those applications may access 'sendQueue' and 'receiveQueue'
 * directly, and use the methods on those UsbQueue objects.
 *
 * Applications that do not need this level of control can use the
 * read() and write() methods on this object directly.
 */

template < unsigned tSendCapacity = 4, unsigned tReceiveCapacity = 4 >
struct UsbPipe {

    /// Queue for packets we're waiting to send.
    UsbQueue< tSendCapacity > sendQueue;

    /// Queue for packets that have been received but not yet processed.
    UsbQueue< tReceiveCapacity > receiveQueue;

    /**
     * @brief Attach this pipe to the system.
     *
     * When a pipe is "attached", the system will write incoming packets
     * to our receiveQueue and it will be looking for transmittable packets
     * in our sendQueue.
     *
     * This buffer must stay allocated as long as it's attached to the system.
     * If you plan on recycling or freeing this memory, make sure to detach()
     * it first.
     *
     * If there was already a different pipe attached, this one replaces it.
     * The previous pipe is automatically detached.
     *
     * This method clears the send and receive queues. Both queues will be empty
     * when they're first attached to the system.
     */
    void attach() {
        sendQueue.clear();
        receiveQueue.clear();
        _SYS_usb_setPipe(sendQueue, receiveQueue);
    }

    /**
     * @brief Detach all pipes from the system.
     *
     * After this call, the Sifteo USB API will throw an error if
     * application-specific packets are sent to us. The UsbPipe
     * may be recycled or freed.
     */
    void detach() {
        _SYS_usb_setPipe(0, 0);
    }

    /**
     * @brief Write one packet to the send queue, if space is available.
     *
     * This copies the provided packet into our send queue and returns
     * 'true' if space is available. If the queue is full, returns 'false'.
     *
     * For more detailed control over the queue, see the methods on
     * our 'sendQueue' member.
     */
    bool write(const UsbPacket &buffer)
    {
        if (sendQueue.writeAvailable()) {
            sendQueue.write(buffer);
            return true;
        }
        return false;
    }

    /**
     * @brief If a packet is available in the queue, read it.
     *
     * If a packet is available, copies it to 'buffer' and returns 'true'.
     * If no packets are waiting, returns 'false'.
     */
    bool read(UsbPacket &buffer)
    {
        if (receiveQueue.readAvailable()) {
            receiveQueue.read(buffer);
            return true;
        }
        return false;
    }

    /// How many packets are available to read from this queue right now?
    unsigned readAvailable() const {
        return receiveQueue.readAvailable();
    }

    /// How many free buffers are available for writing right now?
    unsigned writeAvailable() const {
        return sendQueue.writeAvailable();
    }
};


/**
 * @brief Diagnostic counters for the Bluetooth subsystem
 *
 * This object is a container for counter values that are tracked
 * by the Bluetooth subsystem.
 *
 * The default constructor leaves BluetoothCounters uninitialized.
 * Call reset() once before the event you want to measure, and call
 * capture() to grab the latest counter values. Other accessors in this
 * structure will compare the latest counters with the reference values
 * read by reset().
 */

struct UsbCounters
{
    _SYSUsbCounters current;
    _SYSUsbCounters base;

    /**
     * @brief Reset all counters back to zero
     *
     * This captures a baseline value for all counters. This must be
     * called once before using the counters to measure changes.
     */
    void reset() {
        _SYS_usb_counters(&base, sizeof base);
    }

    /**
     * @brief Update the state of all counters
     */
    void capture() {
        _SYS_usb_counters(&current, sizeof current);
    }

    /// Total received packets
    uint32_t receivedPackets() {
        return current.rxPackets - base.rxPackets;
    }

    /// Total sent packets
    uint32_t sentPackets() {
        return current.txPackets - base.txPackets;
    }

    /// Total received bytes
    uint32_t receivedBytes() {
        return current.rxBytes - base.rxBytes;
    }

    /// Total sent bytes
    uint32_t sentBytes() {
        return current.txBytes - base.txBytes;
    }

    /**
     * @brief Total user-defined packets dropped
     *
     * This measures the number of received packets that have been dropped due to
     * either having no UsbPipe attached, or having a UsbPipe
     * with a full receive buffer.
     */
    uint32_t userPacketsDropped() {
        return current.rxUserDropped - base.rxUserDropped;
    }
};


/**
 * @} endgroup usb
 */

} // namespace Sifteo
