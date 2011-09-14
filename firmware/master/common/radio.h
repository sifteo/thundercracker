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
 * Abstract base class: Message dispatcher for one peer. Each
 * RadioEndpoint is scheduled to produce packets by the
 * RadioManager. Any timeouts or acknowledgments are delivered to the
 * corresponding RadioEndpoint.
 *
 * These entry points are called in interrupt context.
 */

class RadioEndpoint {
 protected:
    friend class RadioManager;

    // Called in interrupt context
    virtual void produce(PacketTransmission &tx) = 0;
    virtual void acknowledge(const PacketBuffer &packet) = 0;
    virtual void timeout() = 0;
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
    void add(RadioEndpoint *ep);
    void remove(RadioEndpoint *ep);

    /**
     * ISR Delegates, called by Radio's implementation. For every
     * produce()'d packet, we are guaranteed to respond in FIFO
     * order with a corresponding acknowledge() or timeout().
     */

    static void produce(PacketTransmission &tx);
    static void acknowledge(const PacketBuffer &packet);
    static void timeout();

    // Must be a power of two
    static const unsigned MAX_ENDPOINTS = 32;

 private:
    /*
     * All connected endpoints. When an endpoint is disconnected due
     * to a timeout, it is automatically removed (in interrupt
     * context) by setting its slot to NULL without modifying any
     * other slots.
     */
    unsigned numSlots;
    RadioEndpoint *epSlots[MAX_ENDPOINTS];

    /* XXX: How to track when it's safe to reuse a slot? */

    /*
     * Round-robin endpoint scheduling. We currently give every
     * endpoint an opportunity to produce one packet per round. Note
     * that this is updated from the interrupt context. If the main
     * context removes a slot we're using, it can't delete or
     * otherwise make the slot invalid until after we have moved on to
     * the next slot and updated currentSlot.
     */
    unsigned currentSlot;

    /*
     * FIFO buffer of slot numbers that have pending acknowledgments.
     * This lets us match up ACKs with endpoints. Accessed ONLY in interrupt context.
     */
    uint8_t epFifo[MAX_ENDPOINTS];
    unsigned epHead, epTail;
};

};  // namespace Sifteo

#endif
