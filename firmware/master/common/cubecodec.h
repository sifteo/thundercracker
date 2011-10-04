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
#include <sifteo/macros.h>
#include <sifteo/limits.h>

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
        count -= bitWidth;

        /*
         * This ?: is important if the buffer is totally
         * full. Shifting a uint32_t by 32 would be undefined in C,
         * and compilers will in fact give us very nonzero results.
         */
        bits = count ? (bits >> bitWidth) : 0;

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

    void debug();

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
    bool encodeVRAMAddr(PacketBuffer &buf, uint16_t addr);
    bool encodeVRAMData(PacketBuffer &buf, _SYSVideoBuffer *vb, uint16_t data);

    bool flashReset(PacketBuffer &buf);
    bool flashSend(PacketBuffer &buf, _SYSAssetGroup *group, _SYSAssetGroupCube *ac, bool &done);

    void flashAckBytes(uint8_t count) {
        loadBufferAvail += count;
    }

    void endPacket(PacketBuffer &buf) {
        /*
         * If we didn't emit a full packet, that implies an encoder state reset.
         *
         * If we have any partial codes buffered, they'll be lost forever. So,
         * flush them out by adding a nybble which doesn't mean anything on its own.
         */
        
        if (!buf.isFull()) {
            stateReset();
            txBits.append(0xF, 4);
            txBits.flush(buf);
            txBits.init();

            if (!buf.len) {
                /*
                 * If we have nothing to send, make it an empty 'ping' packet.
                 * But the nRF24L01 can't actually send a zero-byte packet, so
                 * we have to pad it with a no-op. We don't have any explicit
                 * no-op in our protocol, but we can send only the first byte
                 * from a multi-byte code.
                 *
                 * This is the first byte of a 14-bit literal.
                 */
                buf.append(0xFF);
            }
        }
    }
    
 private:
    // Try to keep these ordered to minimize padding...

    BitBuffer txBits;           /// Buffer of transmittable codes
    uint8_t loadBufferAvail;    /// Amount of flash buffer space available
    uint8_t codeS;              /// Codec "S" state (sample #)
    uint8_t codeD;              /// Codec "D" state (coded delta)
    uint8_t codeRuns;           /// Codec run count
    uint16_t codePtr;           /// Codec's VRAM write pointer state (word address)

    void codePtrAdd(uint16_t words) {
        codePtr = (codePtr + words) & PTR_MASK;
    }

    unsigned deltaSample(_SYSVideoBuffer *vb, uint16_t data, uint16_t offset) {
        uint16_t ptr = codePtr - offset;

        ptr &= PTR_MASK;

        if ((vb->lock & Sifteo::Intrinsic::LZ(ptr >> 4)) ||
            (vb->cm1[ptr >> 5] & Sifteo::Intrinsic::LZ(ptr & 31))) {

            // Can't match a locked or modified word
            return (unsigned) -1;
        }
 
        uint16_t sample = vb->vram.words[ptr];

        if ((sample & 0x0101) != (data & 0x0101)) {
            // Different LSBs, can't possibly reach it via a delta
            return (unsigned) -1;
        }

        int16_t dI = ((data & 0xFF) >> 1) | ((data & 0xFF00) >> 2);
        int16_t sI = ((sample & 0xFF) >> 1) | ((sample & 0xFF00) >> 2);

        return dI - sI + RF_VRAM_DIFF_BASE;
    }

    void appendDS(uint8_t d, uint8_t s) {
        if (d == RF_VRAM_DIFF_BASE) {
            // Copy code
            txBits.append(0x4 | s, 4);
        } else {
            // Diff code
            txBits.append(0x8 | s | (d << 4), 8);
        }
    }

    void flashEscape(PacketBuffer &buf) {
        /*
         * Escape to flash mode (two-nybble code 33) plus one extra
         * dummy nybble to force a byte flush.
         */
        txBits.append(0x333, 12);
        txBits.flush(buf);
        txBits.init();
        stateReset();
    }

    void encodeDS(uint8_t d, uint8_t s);
    void flushDSRuns(bool rleSafe);

    static const unsigned PTR_MASK = 511;
};

#endif
