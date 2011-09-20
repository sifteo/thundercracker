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
#include <protocol.h>
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
    void stateReset() {	
	codePtr = 0;
	codeS = -1;
    }

    void encodeVRAM(PacketBuffer &buf, _SYSVideoBuffer *vb);
    bool encodeVRAM(PacketBuffer &buf, uint16_t addr, uint16_t data,
		    _SYSVideoBuffer *vb);

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

    void codePtrAdd(uint16_t words) {
	codePtr = (codePtr + words) & PTR_MASK;
    }

    uint16_t wordToIndex(uint16_t data) {
	return ((data & 0xFF) >> 1) | ((data & 0xFF00) >> 2);
    }

    unsigned deltaSample(_SYSVideoBuffer *vb, uint16_t index, uint16_t offset) {
	uint16_t ptr = codePtr - offset;

	ptr &= PTR_MASK;

	if ((vb->lock & (0x80000000 >> (ptr >> 4))) ||
	    (vb->cm1[ptr >> 5] & (0x80000000 >> (ptr & 31)))) {

	    // Can't match a locked or modified word
	    return (unsigned) -1;
	}

	return index - wordToIndex(vb->words[ptr]) + RF_VRAM_DIFF_BASE;
    }

    void encodeDS(uint8_t d, uint8_t s);
    void flushDSRuns();

    static const unsigned PTR_MASK = 511;
};

#endif
