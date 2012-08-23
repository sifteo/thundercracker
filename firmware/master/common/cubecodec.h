/*
 * Thundercracker Firmware -- Confidential, not for redistribution.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#ifndef _SIFTEO_CUBECODEC_H
#define _SIFTEO_CUBECODEC_H

#include <sifteo/abi.h>
#include <protocol.h>
#include "machine.h"
#include "macros.h"
#include "radio.h"
#include "vram.h"

#ifdef CODEC_DEBUG
#define CODEC_DEBUG_LOG(x)   DEBUG_LOG(x)
#else
#define CODEC_DEBUG_LOG(x)
#endif


/**
 * A utility class to buffer bit-streams as we transmit them to a cube.
 *
 * Intended only for little-endian machines with fast bit shifting,
 * like ARM or x86!
 */

class BitBuffer {
 public:
    ALWAYS_INLINE void init() {
        bits = 0;
        count = 0;
    }

    ALWAYS_INLINE unsigned flush(PacketBuffer &buf) {
        ASSERT(count <= 32);

        unsigned byteWidth = MIN(buf.bytesFree(), (unsigned)(count >> 3));
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

    ALWAYS_INLINE bool hasRoomForFlush(PacketBuffer &buf, unsigned additionalBits=0) {
        // Does the buffer have room for every complete byte in the buffer, plus additionalBits?
        return buf.bytesFree() >= ((count + additionalBits) >> 3);
    }

    ALWAYS_INLINE void append(uint32_t value, unsigned width) {
        CODEC_DEBUG_LOG(("CODEC: \tbits: %08x/%d <- %08x/%d\n", bits, count, value, width));

        // Overflow-safe asserts
        ASSERT(count <= 32);
        ASSERT(width <= 32);

        bits |= value << count;
        count += width;
        ASSERT(count <= 32);
    }

    ALWAYS_INLINE void appendMasked(uint32_t value, unsigned width) {
        append(value & ((1 << width) - 1), width);
    }

    ALWAYS_INLINE bool hasPartialByte() const {
        return count > 0 && count < 8;
    }

 private:
    uint32_t bits;
    uint8_t count;
};

/**
 * Compression codec, for sending compressed VRAM data to cubes.
 * This implements the protocol described in protocol.h.
 */

class CubeCodec {
 public:
    ALWAYS_INLINE void stateReset() { 
        codePtr = 0;
        codeS = -1;
        txBits.init();
    }

    // Returns 'true' if finished.
    bool encodeVRAM(PacketBuffer &buf, _SYSVideoBuffer *vb);

    bool encodeVRAMAddr(PacketBuffer &buf, uint16_t addr);
    bool encodeVRAMData(PacketBuffer &buf, uint16_t data);
    bool encodeVRAMData(PacketBuffer &buf, _SYSVideoBuffer *vb, uint16_t data);

    void encodeShutdown(PacketBuffer &buf);
    void encodeStipple(PacketBuffer &buf, _SYSVideoBuffer *vbuf);

    bool encodePoke(PacketBuffer &buf, uint16_t addr, uint16_t data) {
        return encodeVRAMAddr(buf, addr) && encodeVRAMData(buf, data);
    }

    void endPacket(PacketBuffer &buf);

    // Escape codes (Ends the packet)
    void escTimeSync(PacketBuffer &buf, uint16_t rawTimer);
    bool escFlash(PacketBuffer &buf);
    bool escRequestAck(PacketBuffer &buf);
    bool escRadioNap(PacketBuffer &buf, uint16_t duration);
    bool escChannelHop(PacketBuffer &buf, uint8_t channel);

 private:
    // Try to keep these ordered to minimize padding...

    BitBuffer txBits;           /// Buffer of transmittable codes
    uint8_t codeS;              /// Codec "S" state (sample #)
    uint8_t codeD;              /// Codec "D" state (coded delta)
    uint8_t codeRuns;           /// Codec run count
    uint16_t codePtr;           /// Codec's VRAM write pointer state (word address)

    // Temporary state, used only during encode.
    // These are static, so they don't use space per-cube and they're fast to access.

    static uint16_t exemptionBegin;    /// Lock exemption range, first address
    static uint16_t exemptionEnd;      /// Lock exemption range, last address

    ALWAYS_INLINE void codePtrAdd(uint16_t words) {
        ASSERT(codePtr < _SYS_VRAM_WORDS);
        codePtr = (codePtr + words) & _SYS_VRAM_WORD_MASK;
    }

    unsigned deltaSample(_SYSVideoBuffer *vb, uint16_t data, uint16_t offset);

    ALWAYS_INLINE void appendDS(uint8_t d, uint8_t s) {
        if (d == RF_VRAM_DIFF_BASE) {
            // Copy code
            txBits.append(0x4 | s, 4);
        } else {
            // Diff code
            txBits.append(0x8 | s | (d << 4), 8);
        }
    }

    void encodeDS(uint8_t d, uint8_t s);
    void flushDSRuns(bool rleSafe);
};

#endif
