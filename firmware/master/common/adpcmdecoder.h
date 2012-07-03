/*
 * Thundercracker Firmware -- Confidential, not for redistribution.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#ifndef ADPCMDECODER_H
#define ADPCMDECODER_H

#include "macros.h"
#include "machine.h"
#include "svmmemory.h"


/**
 * Packed representation of the ADPCM CODEC state
 */
struct ADPCMState
{
    union {
        uint32_t value;
        struct {
            int16_t sample;
            uint8_t index;
            uint8_t reserved;
        };
    };

    // Our compressed ADPCM data includes a set of initial conditions as
    // a header, to help us adapt fast enough for short samples which start
    // with a steep slope.

    static const unsigned HEADER_BYTES = 3;

    void ALWAYS_INLINE readHeader(uint32_t buffer) {
        value = buffer;
        index = MIN(index, 88);
    }
};


/**
 * Unpacked ADPCM CODEC state, and inlined decoder.
 *
 * This structure uses native 32-bit words for speed. These values can
 * live in the caller's registers, without any costly sign-extend/truncate
 * instructions. Keep it in a local variable, and use ADPCMState
 * for longer-term storage!
 */
class ADPCMDecoder
{
    static const uint16_t stepSizeTable[89];
    static const int codeTable[16];

    int sample;
    unsigned index;

public:
    void ALWAYS_INLINE load(const ADPCMState &state)
    {
        unsigned v = state.value;
        sample = int16_t(v);
        index = v >> 16;
        ASSERT(index < arraysize(stepSizeTable));
    }

    void ALWAYS_INLINE store(ADPCMState &state)
    {
        state.value = uint16_t(sample) | (index << 16);
    }

    void ALWAYS_INLINE init(const uint8_t *header)
    {
        sample = int16_t(header[0] | (unsigned(header[1]) << 8));
        index = MIN(header[2], int(arraysize(stepSizeTable) - 1));
    }

    // Call once per nybble to update decoder state, least significant nybble first
    int ALWAYS_INLINE decodeNybble(unsigned nybble)
    {
        ASSERT(index < arraysize(stepSizeTable));
        ASSERT(nybble < arraysize(codeTable));

        int step = stepSizeTable[index];
        int code = codeTable[nybble];

        // Update predictor and clamp using MULS and SSAT.
        sample = Intrinsic::SSAT(sample + 
            ( int(unsigned(int8_t(code)) * unsigned(step)) >> 3 ), 16);

        // Update quantizer step size
        int nextIndex = index + (code >> 8);

        // Clamp index. (This actually generates quite reasonable assembly)
        nextIndex = MAX(nextIndex, 0);
        nextIndex = MIN(nextIndex, int(arraysize(stepSizeTable) - 1));
        index = nextIndex;

        return sample;
    }

    // Decode one byte of ADPCM data to two samples. Increments source and dest.
    void ALWAYS_INLINE decodeByte(SvmMemory::PhysAddr &src, int16_t *&dest)
    {
        unsigned byte = *(src++);
        *(dest++) = decodeNybble(byte & 0xF);
        *(dest++) = decodeNybble(byte >> 4);
    }
};


#endif // ADPCMDECODER_H
