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
 * @defgroup bluetooth Bluetooth
 *
 * @brief Support for communicating with mobile devices via Bluetooth.
 *
 * Some Sifteo Bases include support for Bluetooth 4.0 Low Energy, also known
 * as Bluetooth Smart. This is a standard designed for low-power exchange of
 * small amounts of data between mobile devices.
 *
 * Bluetooth Low Energy is slow, maxing out at about one kilobyte per second.
 * It is designed for exchanging small bits of game state, or synchronizing
 * small amounts of saved game data. It should not be used for transferring
 * large files.
 *
 * This module provides a very high-level interface to the Bluetooth radio.
 * Pairing, connection state, encryption, and non-game-specific operations
 * including filesystem access are implemented by the system. Some apps
 * which synchronize state or unlock features may find this functionality
 * to be sufficient, and very little in-game code may be necessary.
 *
 * For games that want to communicate directly with mobile devices during
 * gameplay, BluetoothPipe can be used to exchange small packets with a
 * peer device.
 *
 * @{
 */


/**
 * @brief Global Bluetooth operations.
 *
 * The system provides a basic level of Bluetooth functionality without any
 * game-specific code. This includes filesystem access, pairing the base with
 * a Sifteo user account, getting information about the running game, and
 * launching a game.
 */

class Bluetooth {
public:

    /**
     * @brief Is Bluetooth support available on this hardware?
     *
     * Returns 'true' if the Base we're running on has firmware and
     * hardware that support Bluetooth. If not, returns 'false'.
     *
     * @warning Other Bluetooth functions must only be called if isAvailable() 
     * returns 'true'! Do not call any other methods on the Bluetooth object
     * if this returns 'false', and do not install any Bluetooth event handlers.
     */

    static bool isAvailable()
    {
        if ((_SYS_getFeatures() & _SYS_FEATURE_BLUETOOTH) == 0) {
            return false;
        }

        return _SYS_bt_isAvailable();
    }

    /**
     * @brief Is a device currently connected via the Sifteo Bluetooth API?
     *
     * Returns 'true' if a device is connected right now, 'false' if not.
     *
     * To get notified when a connection is created or destroyed, you can
     * attach handlers to Events::bluetoothConnect and Events::bluetoothDisconnect.
     *
     * @note If a new connection is created immediately after an old connection
     *       is destroyed, you're still guaranteed to receive one Disconnect
     *       and one Connect event, in that order.
     */

    static bool isConnected()
    {
        return _SYS_bt_isConnected();
    }

    /**
     * @brief Set the current advertised game state
     *
     * When a device has connected to the Sifteo Base's system software
     * but hasn't necessarily connected to a game's BluetoothPipe,
     * it can still retrieve basic information about the running game.
     *
     * This basic information is "advertised" to the mobile device, which
     * can use it to determine game state and possibly to determine whether
     * it wants to connect.
     *
     * This basic information that we advertise includes a game's
     * package name and version. Games may optionally specify an additional
     * game-specific binary blob to include in the advertisement data.
     *
     * The provided advertisement data is atomically copied to a system buffer.
     * The application can reuse the buffer memory immediately.
     *
     * To disable this additional advertisement data, call advertiseState()
     * with an empty or NULL buffer.
     */

    static void advertiseState(const uint8_t *bytes, unsigned length)
    {
        _SYS_bt_advertiseState(bytes, length);
    };

    /**
     * @brief Template wrapper around advertiseState()
     *
     * This is a convenience wrapper around advertiseState(), for passing
     * arbitrary C++ objects. The object is copied, and it does not need
     * to be retained in memory by the caller.
     */

    template <typename T>
    static void advertiseState(const T &object)
    {
        advertiseState(reinterpret_cast<const uint8_t*>(&object), sizeof object);
    }
};


/**
 * @brief A container for one Bluetooth packet's data.
 *
 * Each packet consists of a variable-length payload of up to 19 bytes, and
 * a "type" byte of which the low 7 bits are available for games to use.
 */

struct BluetoothPacket {
    _SYSBluetoothPacket sys;

    // Implicit conversions
    operator _SYSBluetoothPacket* () { return &sys; }
    operator const _SYSBluetoothPacket* () const { return &sys; }

    /// Retrieve the capacity of a packet, in bytes. (19)
    static unsigned capacity() {
        return _SYS_BT_PACKET_BYTES;
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
 * BluetoothQueues are templatized by buffer size. The system supports
 * buffers with between 1 and 255 packets of capacity. You can pick a
 * buffer size based on how many packets you expect may arrive between
 * each opportunity you have to check on the queue. 
 */

template < unsigned tCapacity >
struct BluetoothQueue {
    struct SysType {
        union {
            _SYSBluetoothQueueHeader header;
            uint32_t header32;
        };
        _SYSBluetoothPacket packets[tCapacity + 1];
    } sys;

    // Implicit conversions
    operator _SYSBluetoothQueue* () { return reinterpret_cast<_SYSBluetoothQueue*>(&sys); }
    operator const _SYSBluetoothQueue* () const { return reinterpret_cast<const _SYSBluetoothQueue*>(&sys); }

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
    void read(BluetoothPacket &buffer)
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
    const BluetoothPacket &peek() const
    {
        ASSERT(readAvailable() > 0);
        unsigned head = sys.header.head;
        ASSERT(head <= tCapacity);
        return *reinterpret_cast<const BluetoothPacket *>(&sys.packets[head]);
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
        _SYS_bt_queueReadHint();
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
    void write(const BluetoothPacket &buffer)
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
    BluetoothPacket &reserve()
    {
        ASSERT(writeAvailable() > 0);
        unsigned tail = sys.header.tail;
        ASSERT(tail <= tCapacity);
        return *reinterpret_cast<BluetoothPacket *>(&sys.packets[tail]);
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
        _SYS_bt_queueWriteHint();
    }
};


/**
 * @brief A memory buffer for bidirectional Bluetooth communications.
 *
 * This is a pair of FIFO buffers which can be used for bidirectional
 * communication over Bluetooth. When the pipe is "attached", this
 * Base becomes available to send and receive application-specific
 * packets over Bluetooth.
 *
 * When the pipe is not attached or no pipe has been allocated,
 * the Sifteo Bluetooth API will throw an error if application-specific
 * packets are sent. However the Bluetooth API provides generic functionality
 * which can be used even without a BluetoothPipe.
 *
 * If the BluetoothPipe's receiveQueue is full, the Sifteo Bluetooth API will
 * not be allowed to transmit more packets until our receiveQueue has room.
 * Packets will not be dropped if the buffer fills up.
 *
 * Some applications may desire lower-level access to the data queues
 * we use. Those applications may access 'sendQueue' and 'receiveQueue'
 * directly, and use the methods on those BluetoothQueue objects.
 *
 * Applications that do not need this level of control can use the
 * read() and write() methods on this object directly.
 *
 * Bluetooth packets do not have guaranteed delivery like TCP sockets.
 * Instead, they have low latency but it's possible for data to be lost,
 * more like UDP sockets. To summarize the guarantees made by BluetoothPipe:
 *
 *
 * 1. Does NOT guarantee that a packet is ever delivered.
 *    Packets can always be dropped.
 * 2. Does NOT provide end-to-end flow control. Packets received
 *    while the receive queue is full will be dropped.
 * 3. DOES provide data integrity, through the Bluetooth protocol's standard CRC.
 *    If a packet arrives at all, it's very unlikely to be corrupted.
 * 4. DOES guarantee in-order delivery, if packets are ever delivered.
 * 5. DOES provide local transmit flow control, so it's easy to write at
 *    the radio's max transmit rate.
 */

template < unsigned tSendCapacity = 4, unsigned tReceiveCapacity = 4 >
struct BluetoothPipe {

    /// Queue for packets we're waiting to send.
    BluetoothQueue< tSendCapacity > sendQueue;

    /// Queue for packets that have been received but not yet processed.
    BluetoothQueue< tReceiveCapacity > receiveQueue;

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
        _SYS_bt_setPipe(sendQueue, receiveQueue);
    }

    /**
     * @brief Detach all pipes from the system.
     *
     * After this call, the Sifteo Bluetooth API will throw an error if
     * application-specific packets are sent to us. The BluetoothPipe
     * may be recycled or freed.
     */
    void detach() {
        _SYS_bt_setPipe(0, 0);
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
    bool write(const BluetoothPacket &buffer)
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
    bool read(BluetoothPacket &buffer)
    {
        if (receiveQueue.readAvailable()) {
            receiveQueue.read(buffer);
            return true;
        }
        return false;
    }

    /// How many packets are available to read from this queue right now?
    unsigned readAvailable() const
    {
        return receiveQueue.readAvailable();
    }

    /// How many free buffers are available for writing right now?
    unsigned writeAvailable() const
    {
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

struct BluetoothCounters
{
    _SYSBluetoothCounters current;
    _SYSBluetoothCounters base;

    /**
     * @brief Reset all counters back to zero
     *
     * This captures a baseline value for all counters. This must be
     * called once before using the counters to measure changes.
     */
    void reset() {
        _SYS_bt_counters(&base, sizeof base);
    }

    /**
     * @brief Update the state of all counters
     */
    void capture() {
        _SYS_bt_counters(&current, sizeof current);
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
     * either having no BluetoothPipe attached, or having a BluetoothPipe
     * with a full receive buffer.
     */
    uint32_t userPacketsDropped() {
        return current.rxUserDropped - base.rxUserDropped;
    }
};


/**
 * @} endgroup bluetooth
 */

} // namespace Sifteo
