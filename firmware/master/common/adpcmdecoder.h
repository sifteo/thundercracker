/*
 * Thundercracker Firmware -- Confidential, not for redistribution.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#ifndef ADPCMDECODER_H
#define ADPCMDECODER_H

#include <stdint.h>

class AdPcmDecoder
{
public:
    AdPcmDecoder() :
        index(0),
        predictedSample(0),
        hasExtraSample(false)
    {}

    int decodeSample(uint8_t **pa);

    void reset() {
        index = 0;
        predictedSample = 0;
        hasExtraSample = false;
    }

private:
    int16_t index;
    int16_t predictedSample;
    int16_t extraSample;
    bool hasExtraSample;    // since we decode two samples per byte, save the extra

    int decodeNybble(unsigned nybble);
};

#endif // ADPCMDECODER_H
