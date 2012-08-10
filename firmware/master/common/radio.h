/*
 * Thundercracker Firmware -- Confidential, not for redistribution.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#ifndef _SIFTEO_RADIO_H
#define _SIFTEO_RADIO_H

#include <string.h>
#include <sifteo/abi.h>
#include "macros.h"
#include "bits.h"
#include "ringbuffer.h"

class RadioManager;


/**
 * All the identifying information necessary to direct a message
 * at a particular radio peer.
 */

struct RadioAddress {
    uint8_t channel;
    uint8_t id[5];

    uint64_t pack() const {
        uint64_t addr = 0;
        for (int i = 4; i >= 0; i--)
            addr = (addr << 8) | id[i];
        return addr | ((uint64_t)channel << 56);
    }
};
 
 
/**
 * Container for data that we eventually want to send to, or that we
 * have received from, a Radio implementation. This does not include
 * the memory allocation for the data buffer, since that is all managed
 * in a platform-specific way by the Radio class.
 */

struct PacketBuffer {
    static const unsigned MAX_LEN = 32;
    uint8_t *bytes;
    unsigned len;

    PacketBuffer() {}

    ALWAYS_INLINE PacketBuffer(uint8_t *_bytes, unsigned _len=0)
        : bytes(_bytes), len(_len) {}

    ALWAYS_INLINE bool isFull() {
        ASSERT(len <= MAX_LEN);
        return len == MAX_LEN;
    }

    ALWAYS_INLINE unsigned bytesFree() {
        ASSERT(len <= MAX_LEN);
        return MAX_LEN - len;
    }

    ALWAYS_INLINE void append(uint8_t b) {
        ASSERT(len < MAX_LEN);
        bytes[len++] = b;
    }

    ALWAYS_INLINE void append(uint8_t *src, unsigned count) {
        // Overflow-safe assertions
        ASSERT(len <= MAX_LEN);
        ASSERT(count <= MAX_LEN);
        ASSERT((len + count) <= MAX_LEN);

        memcpy(bytes + len, src, count);
        len += count;
    }
};


/**
 * When transmitting a packet, we provide both a RadioAddress and
 * a PacketBuffer.  We use a pointer to the RadioAddress here, on
 * the assumption that the full (larger) RadioAddress structure
 * will be maintained as part of a Cube object.
 */

struct PacketTransmission {
    PacketBuffer packet;
    const RadioAddress *dest;

    bool noAck;
    uint8_t numHardwareRetries;
    uint8_t numSoftwareRetries;

    static const unsigned MAX_HARDWARE_RETRIES = 15;

    static const unsigned DEFAULT_HARDWARE_RETRIES = MAX_HARDWARE_RETRIES;
    static const unsigned DEFAULT_SOFTWARE_RETRIES = 64;

    ALWAYS_INLINE void init() {
        noAck = 0;
        numHardwareRetries = DEFAULT_HARDWARE_RETRIES;
        numSoftwareRetries = DEFAULT_SOFTWARE_RETRIES;
    }

    ALWAYS_INLINE PacketTransmission() {
        init();
    }

    ALWAYS_INLINE PacketTransmission(const RadioAddress *_dest, uint8_t *_bytes, unsigned _len=0)
        : packet(PacketBuffer(_bytes, _len)), dest(_dest) {
        init();
    }
};


/**
 * Abstraction for low-level radio communications, and
 * hardware-specific details pertaining to the radio. Individual
 * platforms will have separate implementations of this class.
 *
 * In order to allow efficient use of memory and CPU, we let
 * the Radio object manage control flow and buffering.
 *
 * We assume it will be performing DMA on a per-packet basis, and
 * that it's more efficient overall to construct the packet in a
 * system-memory buffer and DMA it to the radio engine, than to
 * emit individual bytes via PIO.
 *
 * The overall flow control model is one in which application code
 * runs in one or more main threads, and hardware events are
 * handled in ISRs in a platform-specific way.
 *
 * We abstract this as a set of radio callbacks which are invoked
 * asynchronously, and possibly in an interrupt context. See
 * RadioManager for these callbacks.
 */

class Radio {
 public:
    static void init();

    // Called at Tasks::HEARTBEAT_HZ, in task context
    static void heartbeat();

    /*
     * Values for the L01's tx power register.
     */
    enum TxPower {
        dBmMinus18              = 0,
        dBmMinus12              = 1 << 1,
        dBmMinus6               = 2 << 1,
        dBm0                    = 3 << 1
    };

    /*
     * Setter/getter for transmit power.
     * We don't strictly need a getter, but it is used for testing purposes
     * to verify that we can read/write registers properly.
     */
    static void setTxPower(TxPower pwr);
    static TxPower txPower();
};


/**
 * Multiplexes radio communications to and from multiple cubes.
 * This class is non-platform-specific, and it is where Radio
 * delegates the work of actually creating and consuming raw
 * packets.
 */
 
class RadioManager {
 public:

    static ALWAYS_INLINE bool isRadioEnabled() {
        return enabled;
    }

    static ALWAYS_INLINE void enableRadio() {
        enabled = true;
    }

    static ALWAYS_INLINE void disableRadio() {
        enabled = false;
    }

    /**
     * ISR Delegates, called by Radio's implementation. For every
     * produce()'d packet, we are guaranteed to respond in FIFO
     * order with a corresponding acknowledge() or timeout().
     */

    static void produce(PacketTransmission &tx);
    static void ackWithPacket(const PacketBuffer &packet);
    static void ackEmpty();
    static void timeout();

    /*
     * FIFO buffer of slot numbers that have pending acknowledgments.
     * This lets us match up ACKs with endpoints. Accessed ONLY in
     * interrupt context.
     *
     * The size must be deep enough to cover the worst-case
     * queueing depth of the Radio implementation. On real hardware
     * this will be quite small. This is also independent of the
     * number of cubes in use. Must be a power of two.
     */
    static const unsigned FIFO_DEPTH = 8;

 private:
    typedef RingBuffer<FIFO_DEPTH, uint8_t, uint8_t> fifo_t;
    static fifo_t fifo;
    static bool enabled;

    // ID for the CubeConnector. Must not collide with any CubeSlot ID.
    static const unsigned CONNECTOR_ID = _SYS_NUM_CUBE_SLOTS;

    // Total number of producers in the system, including CONNECTOR_ID
    static const unsigned NUM_PRODUCERS = CONNECTOR_ID + 1;

    // Dummy ID, not counted in NUM_PRODUCERS
    static const unsigned DUMMY_ID = NUM_PRODUCERS;

    // Tracking packet IDs, for explicitly avoiding ID collisions
    static const unsigned PID_COUNT = 4;
    static const unsigned PID_MASK = PID_COUNT - 1;
    static uint8_t nextPID;

    // Priority queues for each PID value
    static uint32_t schedule[PID_COUNT];
    static uint32_t nextSchedule[PID_COUNT];
    
    // Dispatch to a paritcular producer, by ID
    static bool dispatchProduce(unsigned id, PacketTransmission &tx);
    static void dispatchAcknowledge(unsigned id, const PacketBuffer &packet);
    static void dispatchEmptyAcknowledge(unsigned id);
    static void dispatchTimeout(unsigned id);
};

#endif
