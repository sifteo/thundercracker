/*
 * Thundercracker Firmware -- Confidential, not for redistribution.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#include "macros.h"
#include "adpcmdecoder.h"
#include "machine.h"


int ALWAYS_INLINE AdPcmDecoder::decodeNybble(unsigned nybble)
{
    // IMA ADPCM step sizes
    static const uint16_t stepSizeTable[89] = {
        7, 8, 9, 10, 11, 12, 13, 14, 16, 17, 19, 21, 23, 25, 28, 31,
        34, 37, 41, 45, 50, 55, 60, 66, 73, 80, 88, 97, 107, 118, 130,
        143, 157, 173, 190, 209, 230, 253, 279, 307, 337, 371, 408, 449,
        494, 544, 598, 658, 724, 796, 876, 963, 1060, 1166, 1282, 1411,
        1552, 1707, 1878, 2066, 2272, 2499, 2749, 3024, 3327, 3660, 4026,
        4428, 4871, 5358, 5894, 6484, 7132, 7845, 8630, 9493, 10442, 11487,
        12635, 13899, 15289, 16818, 18500, 20350, 22385, 24623, 27086, 29794, 32767
    };

    /*
     * Packed LUT for decoded nybble. Bits [7:0] are a signed coefficient for
     * the current step size, and bits [31:24] are the amount by which we update
     * the index. This split makes it easy to extract either half, as a signed
     * integer, using common ARM instructions. (Arithmetic shift, or 8-bit
     * sign extend).
     *
     * Multiply is cheap, table lookups are cheap!
     */

    static const int codeTable[16] = {
        0xFFFFFF01,
        0xFFFFFF03,
        0xFFFFFF05,
        0xFFFFFF07,
        0x00000209,
        0x0000040B,
        0x0000060D,
        0x0000080F,
        0xFFFFFFFF,
        0xFFFFFFFD,
        0xFFFFFFFB,
        0xFFFFFFF9,
        0x000002F7,
        0x000004F5,
        0x000006F3,
        0x000008F1,
    };

    ASSERT(index >= 0 && index < int(arraysize(stepSizeTable)));
    ASSERT(nybble < arraysize(codeTable));

    int step = stepSizeTable[index];
    int code = codeTable[nybble];

    // Update predictor and clamp using MULS and SSAT.
    int32_t nextPredictor = predictedSample;
    nextPredictor += int(unsigned(int8_t(code)) * unsigned(step)) >> 3;
    nextPredictor = Intrinsic::SSAT(nextPredictor, 16);
    predictedSample = nextPredictor;

    // Update quantizer step size
    int nextIndex = index + (code >> 8);

    // Clamp index. (This actually generates quite reasonable assembly)
    nextIndex = MAX(nextIndex, 0);
    nextIndex = MIN(nextIndex, int(arraysize(stepSizeTable) - 1));
    index = nextIndex;

    return nextPredictor;
}

int AdPcmDecoder::decodeSample(uint8_t **paPtr)
{
    ASSERT(paPtr);

    uint8_t *pa = *paPtr;
    ASSERT(pa);

    if (hasExtraSample) {
        hasExtraSample = false;
        (*paPtr) += 1;
        return extraSample;
    }

    const uint8_t code = *pa;

    int sample = decodeNybble(code & 0xF);
    extraSample = decodeNybble(code >> 4);
    hasExtraSample = true;

    return sample;
}

