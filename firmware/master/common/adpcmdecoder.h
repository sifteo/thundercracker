/*
 * Thundercracker Firmware -- Confidential, not for redistribution.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#ifndef ADPCMDECODER_H
#define ADPCMDECODER_H

#include "audiobuffer.h"
#include "flashlayer.h"

class AdPcmDecoder
{
public:
    AdPcmDecoder() :
        index(0),
        predictedSample(0),
        hasExtraSample(false)
    {}

    int16_t decodeSample(uint8_t **pa);

    void reset() {
        index = 0;
        predictedSample = 0;
        hasExtraSample = false;
    }

private:
    int16_t index;
    int32_t predictedSample;

    // since we decode two samples per byte, save the extra
    bool hasExtraSample;
    int16_t extraSample;

    static const uint16_t stepSizeTable[];
    static const int8_t indexTable[];

    int16_t decode4to16bits(uint8_t code);
};

#endif // ADPCMDECODER_H
