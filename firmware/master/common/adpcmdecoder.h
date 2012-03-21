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
        predictedSample(0)
    {}

    void decode(FlashStream &in, AudioBuffer &out);

private:
    int16_t index;
    int32_t predictedSample;

    static const uint16_t stepSizeTable[];
    static const int8_t indexTable[];

    int16_t decode4to16bits(uint8_t code);
};

#endif // ADPCMDECODER_H
