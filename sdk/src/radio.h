/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * This file is part of the internal implementation of the Sifteo SDK.
 * Confidential, not for redistribution.
 *
 * Copyright <c> 2011 Sifteo, Inc. All rights reserved.
 */

#ifndef _SIFTEO_RADIO_H
#define _SIFTEO_RADIO_H

#include <stdint.h>

namespace Sifteo {


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
};

/**
 * When transmitting a packet, we provide both a RadioAddress and
 * a PacketBuffer.  We use a pointer to the RadioAddress here, on
 * the assumption that the full (larger) RadioAddress structure
 * will be maintained as part of a Cube object.
 */

struct PacketTransmission {
    PacketBuffer packet;
    RadioAddress *dest;
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
 *
 * We maintain a set of per-cube transmit buffers, with distinct
 * buffering schemes for VRAM, Flash, and other data.
 */
 
class RadioManager {
 public:
    //static void pair(Cube &addr);
    static const unsigned MAX_CUBES = 6;

    /**
     * ISR Delegates, called by Radio's implementation. For every
     * produce()'d packet, we are guaranteed to respond in FIFO
     * order with a corresponding acknowledge() or timeout().
     */

    static void produce(PacketTransmission &tx);
    static void acknowledge(const PacketBuffer &packet);
    static void timeout();
};

};  // namespace Sifteo

#endif
