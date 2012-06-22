/*
 * Thundercracker Firmware -- Confidential, not for redistribution.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#ifndef ADPCMDECODER_H
#define ADPCMDECODER_H

#include "macros.h"
#include "machine.h"


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
            uint8_t extra;
        };
    };

    void ALWAYS_INLINE init() {
        value = 0;
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
    unsigned extra;

public:
    void ALWAYS_INLINE load(const ADPCMState &state)
    {
        sample = state.sample;
        index = state.index;
        extra = state.extra;
    }

    void ALWAYS_INLINE store(ADPCMState &state)
    {
        state.sample = sample;
        state.index = index;
        state.extra = extra;
    }

    void ALWAYS_INLINE decodeNybble(unsigned nybble)
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
    }

    /// Decodes one sample. Maybe advances 'bytes'
    int ALWAYS_INLINE decodeSample(const uint8_t *&bytes)
    {
        int result;

        if (extra ^= 1) {
            // No extra sample; decode two and store one.
            const uint8_t code = *bytes;
            decodeNybble(code & 0xF);
            result = sample;
            decodeNybble(code >> 4);

        } else {
            // Return the stored 'extra' sample, move to next byte.
            bytes++;
            result = sample;
        }

        return result;
    }
};


#endif // ADPCMDECODER_H
