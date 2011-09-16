/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * This file is part of the internal implementation of the Sifteo SDK.
 * Confidential, not for redistribution.
 *
 * Copyright <c> 2011 Sifteo, Inc. All rights reserved.
 */

#ifndef _SIFTEO_CUBECODEC_H
#define _SIFTEO_CUBECODEC_H

#include <sifteo/abi.h>
#include <sifteo/machine.h>
#include "radio.h"
#include "runtime.h"


/**
 * A utility class to buffer bit-streams as we transmit them to a cube.
 *
 * Intended only for little-endian machines with fast bit shifting,
 * like ARM or x86!
 */

class BitBuffer {
 public:
    void init() {
	bits = 0;
	count = 0;
    }

    unsigned flush(PacketBuffer &buf) {
	unsigned byteWidth = MIN(buf.bytesFree(), count >> 3);
	buf.append((uint8_t *) &bits, byteWidth);

	unsigned bitWidth = byteWidth << 3;
	bits >>= bitWidth;
	count -= bitWidth;

	return byteWidth;
    }

    bool hasRoomForFlush(PacketBuffer &buf, unsigned additionalBits=0) {
	// Does the buffer have room for every complete byte in the buffer, plus additionalBits?
	return buf.bytesFree() >= ((count + additionalBits) >> 3);
    }

    void append(uint32_t value, unsigned width) {
	bits |= value << count;
	count += width;
    }

    void appendMasked(uint32_t value, unsigned width) {
	append(value & ((1 << width) - 1), width);
    }

 private:
    uint32_t bits;
    uint8_t count;
};


/**
 * Compression codec, for sending VRAM and Flash data to cubes.
 * This implements the protocol described in protocol.h.
 */

class CubeCodec {
 public:
    void encodeVRAM(PacketBuffer &buf, _SYSVideoBuffer *vb);
    bool flashReset(PacketBuffer &buf);
    bool flashSend(PacketBuffer &buf, _SYSAssetGroup *group, _SYSAssetGroupCube *ac, bool &done);

    void flashAckBytes(uint8_t count) {
	loadBufferAvail += count;
    }
    
 private:
    // Try to keep these ordered to minimize padding...

    BitBuffer txBits;		/// Buffer of transmittable codes
    uint8_t loadBufferAvail;	/// Amount of flash buffer space available
    uint8_t codeS;		/// Codec "S" state (sample #)
    uint8_t codeD;		/// Codec "D" state (coded delta)
    uint8_t codeRuns;		/// Codec run count
    uint16_t codePtr;		/// Codec's VRAM write pointer state (word address)
    uint16_t scanPtr;		/// Current word address in scanning the VRAM buffer

    // Video buffer predicates

    static const unsigned NUM_VRAM_WORDS = 512;

    static bool testCM1(const _SYSVideoBuffer *vb, uint16_t wordAddr) {
	// Is this address marked for change in CM1?
	return vb->cm1[wordAddr >> 5] & (1 << (wordAddr & 31));
    }

    static uint16_t next1(uint16_t word) {
	// Next address, by 1 word
	return (word + 1) & (NUM_VRAM_WORDS - 1);
    }

    static bool testCM16(const _SYSVideoBuffer *vb, uint16_t wordAddr) {
	// Is this address marked for change in CM16?
	return vb->cm16 & (1 << (wordAddr >> 4));
    }

    static uint16_t next16(uint16_t word) {
	// Next 16-word address bucket
	return ((word & ~15) + 16) & (NUM_VRAM_WORDS - 1);
    }

    // Code packing functions, for the VRAM compression algorithm

    void txSkip();
};

#endif
