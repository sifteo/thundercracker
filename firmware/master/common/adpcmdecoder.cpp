/*
 * Thundercracker Firmware -- Confidential, not for redistribution.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#include "adpcmdecoder.h"

const uint16_t AdPcmDecoder::stepSizeTable[89] = {
    7, 8, 9, 10, 11, 12, 13, 14, 16, 17, 19, 21, 23, 25, 28, 31,
    34, 37, 41, 45, 50, 55, 60, 66, 73, 80, 88, 97, 107, 118, 130,
    143, 157, 173, 190, 209, 230, 253, 279, 307, 337, 371, 408, 449,
    494, 544, 598, 658, 724, 796, 876, 963, 1060, 1166, 1282, 1411,
    1552, 1707, 1878, 2066, 2272, 2499, 2749, 3024, 3327, 3660, 4026,
    4428, 4871, 5358, 5894, 6484, 7132, 7845, 8630, 9493, 10442, 11487,
    12635, 13899, 15289, 16818, 18500, 20350, 22385, 24623, 27086, 29794, 32767
};

const int8_t AdPcmDecoder::indexTable[16] = {
    0xff, 0xff, 0xff, 0xff, 2, 4, 6, 8,
    0xff, 0xff, 0xff, 0xff, 2, 4, 6, 8
};

void AdPcmDecoder::decode(FlashStream &in, AudioBuffer &out)
{
    FlashStreamBuffer<256> inStream;
    inStream.reset();

    while (out.writeAvailable() >= 4) {

        const uint8_t *pcode = inStream.read(in, 1);
        if (!pcode)
            return;

        const uint8_t code = *pcode;

        int16_t sample = decode4to16bits(code & 0xf);
        out.write(reinterpret_cast<uint8_t*>(&sample), 2);

        sample = decode4to16bits(code >> 4);
        out.write(reinterpret_cast<uint8_t*>(&sample), 2);
    }
}

int16_t AdPcmDecoder::decode4to16bits(uint8_t code)
{
    uint16_t step = stepSizeTable[index];

    // inverse code into diff
    int32_t diffq = step >> 3;
    if (code & 4)
        diffq += step;

    if (code & 2)
        diffq += step >> 1;

    if (code & 1)
        diffq += step >> 2;

    // add diff to predicted sample
    if (code & 8)
        predictedSample -= diffq;
    else
        predictedSample += diffq;

    // check for overflow
    if (predictedSample > 32767)
        predictedSample = 32767;
    else if (predictedSample < -32768)
        predictedSample = -32768;

    // find new quantizer step size
    index += indexTable[code];
    if (index < 0)
        index = 0;
    else if (index > 88)
        index = 88;

    return (int16_t)predictedSample;
}
