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
     * have received from, a Radio implementation.
     */

    struct PacketBuffer {
	static const unsigned MAX_SIZE = 32;

	void init(uint8_t length = 0) {
	    len = length;
	    timeout = false;
	}

	uint8_t bytes[MAX_SIZE];
	uint8_t len;
	bool timeout;
    };

    /**
     * Multiplexes radio communications to and from multiple cubes.
     */

    class RadioManager {
    public:
	static void pair(Cube &addr);

	static bool produce(RadioAddress &*addr, PacketBuffer &*packet);
	static void consume(const PacketBuffer &packet);

	static const unsigned MAX_CUBES = 6;
    };

    /*
     * XXX: Still thinking about how much of this should be pull
     *      vs. push.  In some ways it makes sense to use a blocking
     *      send() but with an asynchronous receive interrupt, since
     *      that most closely matches what games will want. BUT, the
     *      RadioManager will actually be doing a lot of work behind
     *      the scenes that would end up requiring a lot of
     *      buffering. It would be more efficient if we had a pull
     *      interface here, so that we could load data on-demand from
     *      any existing buffers, like loadstream decompression
     *      buffers or in-memory tile buffers.
     *
     *      SO, it would actually be best if we kept the interface
     *      pull-based, plus as bufferless as possible. I think this
     *      means we have produce/consume interrupt callbacks, which
     *      then in turn call very fine-grained functions which
     *      operate on the radio's SPI buffers.
     *
     * XXX: Also need to look at the STM32's DMA peripheral, to see
     *      what kind of data path we'd want to optimally use e.g.
     *      for VRAM or loadstream data. Then figure out how we can
     *      abstract that without introducing any undue inefficiencies.
     */


    /**
     * Abstraction for low-level radio communications, and
     * hardware-specific details pertaining to
     * communications. Individual platforms will have separate
     * implementations of this class.
     *
     * We assume that the radio consists of hardware transmit and
     * receive queues. Received packets are handled asynchronously
     * and immediately by The radio has a simple interrupt-driven
     * event loop in pump() which will possibly transmit or receive
     * a packet, or will go into low-power sleep if we have nothing
     * better to do.
     *
     * When the radio has room in its transmit buffer or has a
     * received packet ready for us, it will call the RadioManager's
     * produce() or consume() members.
     */

    class Radio {
    public:
	static void open();
	static void pump();
    };
    
};

#endif
