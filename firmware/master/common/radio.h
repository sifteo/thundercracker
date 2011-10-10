/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * This file is part of the internal implementation of the Sifteo SDK.
 * Confidential, not for redistribution.
 *
 * Copyright <c> 2011 Sifteo, Inc. All rights reserved.
 */

#ifndef _SIFTEO_RADIO_H
#define _SIFTEO_RADIO_H

#include <string.h>
#include <sifteo/abi.h>
#include "runtime.h"

class RadioManager;


/**
 * All the identifying information necessary to direct a message
 * at a particular radio peer.
 */

struct RadioAddress {
    uint8_t channel;
    uint8_t id[5];
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

    PacketBuffer(uint8_t *_bytes, unsigned _len=0)
        : bytes(_bytes), len(_len) {}

    bool isFull() {
        ASSERT(len <= MAX_LEN);
        return len == MAX_LEN;
    }

    unsigned bytesFree() {
        ASSERT(len <= MAX_LEN);
        return MAX_LEN - len;
    }

    void append(uint8_t b) {
        ASSERT(len < MAX_LEN);
        bytes[len++] = b;
    }

    void append(uint8_t *src, unsigned count) {
        // Overflow-safe assertions
        ASSERT(len <= MAX_LEN);
        ASSERT(count <= MAX_LEN);
        ASSERT((len + count) <= MAX_LEN);

        memcpy(bytes + len, src, count);
        len += count;
    }

    void appendUser(uint8_t *src, unsigned count) {
        if (Runtime::checkUserPointer(src, count))
            append(src, count);
    }

    void log() const {
        #if defined(SIFTEO_SIMULATOR) && defined(DEBUG)
        LOG(("[%2d] ", len));
        for  (unsigned i = 0; i < len; i++)
            LOG(("%02x", bytes[i]));
        LOG(("\n"));
        #endif
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

    PacketTransmission() {}

    PacketTransmission(const RadioAddress *_dest, uint8_t *_bytes, unsigned _len=0)
        : packet(PacketBuffer(_bytes, _len)), dest(_dest) {}

    void log() const {
        #if defined(SIFTEO_SIMULATOR) && defined(DEBUG)
        LOG(("%2d/%02x%02x%02x%02x%02x ",
             dest->channel,
             dest->id[0], dest->id[1], dest->id[2], dest->id[3], dest->id[4]));
        packet.log();
        #endif
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
 *
 * The overall Radio device only has a few operations it does
 * synchronously, accessible via members of this class
 * itself. Initialization, of course. And when we have nothing
 * better to do, we can halt() to enter a low-power state until an
 * interrupt arrives. This is similar to the x86 HLT instruction.
 *
 * XXX: This class should also include power management features,
 *      especially transmit power control and transmit frequency
 *      control. For now, we're assuming that every transmit
 *      opportunity will be taken.
 */

class Radio {
 public:
    static void open();
    static void halt();
};

/**
 * Multiplexes radio communications to and from multiple cubes.
 * This class is non-platform-specific, and it is where Radio
 * delegates the work of actually creating and consuming raw
 * packets.
 */
 
class RadioManager {
 public:
    /**
     * ISR Delegates, called by Radio's implementation. For every
     * produce()'d packet, we are guaranteed to respond in FIFO
     * order with a corresponding acknowledge() or timeout().
     */

    static void produce(PacketTransmission &tx);
    static void acknowledge(const PacketBuffer &packet);
    static void timeout();

 private:
    /*
     * FIFO buffer of slot numbers that have pending acknowledgments.
     * This lets us match up ACKs with endpoints. Accessed ONLY in
     * interrupt context.
     *
     * The FIFO_SIZE must be deep enough to cover the worst-case
     * queueing depth of the Radio implementation. On real hardware
     * this will be quite small. This is also independent of the
     * number of cubes in use.
     *
     * Must be a power of two.
     */

    static const unsigned FIFO_SIZE = 8;

    static _SYSCubeID epFifo[FIFO_SIZE];
    static uint8_t epHead;              // Oldest ACK slot ID
    static uint8_t epTail;              // Location for next packet's slot ID
    static _SYSCubeID schedNext;        // Next cube in the schedule rotation

    static void fifoPush(_SYSCubeID id) {
        ASSERT(epTail < FIFO_SIZE);
        epFifo[epTail] = id;
        epTail = (epTail + 1) & (FIFO_SIZE - 1);
        ASSERT(epTail != epHead);
    }

    static _SYSCubeID fifoPop() {
        ASSERT(epTail != epHead);
        ASSERT(epHead < FIFO_SIZE);
        _SYSCubeID id = epFifo[epHead];
        epHead = (epHead + 1) & (FIFO_SIZE - 1);
        return id;
    }
};

#endif
