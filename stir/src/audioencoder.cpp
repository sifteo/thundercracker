/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * STIR -- Sifteo Tiled Image Reducer
 * M. Elizabeth Scott <beth@sifteo.com>
 *
 * Copyright <c> 2011 Sifteo, Inc. All rights reserved.
 */

#include "audioencoder.h"

#include <errno.h>
#include <string.h>
#include <stdint.h>
#include <assert.h>
#include <ctype.h>
#include <algorithm>
#include <vector>

using namespace std;

#define SAMPLE_RATE 16000

AudioEncoder *AudioEncoder::create(std::string name, float quality, bool vbr)
{
    std::transform(name.begin(), name.end(), name.begin(), ::tolower);

    if (name == "pcm")
        return new PCMEncoder();

    if (name == "adpcm" || name == "")
        return new ADPCMEncoder();

    return 0;
}


void PCMEncoder::encodeFile(const std::string &path,
    std::vector<uint8_t> &out, float &kbps)
{
    FILE *fin = fopen(path.c_str(), "rb");
    if (fin == 0)
        return;
    
    char inbuf[512];

    for (;;) {
        int rx = fread(inbuf, 1, sizeof inbuf, fin);
        if (feof(fin) && rx == 0)
            break;

        out.insert(out.end(), &inbuf[0], &inbuf[rx]);
    }

    // Constant bit rate (uncompressed)
    kbps = SAMPLE_RATE * (16.0f / 1000.0f);
}

const uint16_t ADPCMEncoder::stepSizeTable[89] = {
    7, 8, 9, 10, 11, 12, 13, 14, 16, 17, 19, 21, 23, 25, 28, 31,
    34, 37, 41, 45, 50, 55, 60, 66, 73, 80, 88, 97, 107, 118, 130,
    143, 157, 173, 190, 209, 230, 253, 279, 307, 337, 371, 408, 449,
    494, 544, 598, 658, 724, 796, 876, 963, 1060, 1166, 1282, 1411,
    1552, 1707, 1878, 2066, 2272, 2499, 2749, 3024, 3327, 3660, 4026,
    4428, 4871, 5358, 5894, 6484, 7132, 7845, 8630, 9493, 10442, 11487,
    12635, 13899, 15289, 16818, 18500, 20350, 22385, 24623, 27086, 29794, 32767
};

const int8_t ADPCMEncoder::indexTable[16] = {
    0xff, 0xff, 0xff, 0xff, 2, 4, 6, 8,
    0xff, 0xff, 0xff, 0xff, 2, 4, 6, 8
};

void ADPCMEncoder::encodeFile(const std::string &path, std::vector<uint8_t> &out, float &kbps)
{
    FILE *fin = fopen(path.c_str(), "rb");
    if (fin == 0)
        return;

    for (;;) {
        int16_t samples[2];
        int rx = fread(samples, sizeof samples[0], 2, fin);
        if (feof(fin) && rx <= 0)
            break;

        // 2 16-bit samples are stored in a single byte
        uint8_t adpcmByte = encodeSample(samples[0]);
        adpcmByte |= (encodeSample(samples[1]) << 4);
        out.push_back(adpcmByte);
    }

    // Constant bit rate (compressed @ 1/4)
    kbps = SAMPLE_RATE * (4.0f / 1000.0f);

    fclose(fin);
}

uint8_t ADPCMEncoder::encodeSample(int16_t sample)
{
    uint8_t code = 0;
    uint16_t step = stepSizeTable[index];

    // compute diff and record sign and absolut value
    int32_t diff = sample - predsample;
    if (diff < 0) {
        code = 8;
        diff = -diff;
    }

    // quantize the diff into ADPCM code
    // inverse quantize the code into a predicted diff
    uint16_t tmpstep = step;
    int32_t diffq = (step >> 3);

    if (diff >= tmpstep) {
        code |= 0x04;
        diff -= tmpstep;
        diffq += step;
    }

    tmpstep >>= 1;

    if (diff >= tmpstep) {
        code |= 0x02;
        diff -= tmpstep;
        diffq += (step >> 1);
    }

    tmpstep >>= 1;

    if (diff >= tmpstep) {
        code |= 0x01;
        diffq += (step >> 2);
    }

    // fixed predictor to get new predicted sample
    if (code & 8)
        predsample -= diffq;
    else
        predsample += diffq;

    // check for overflow
    if (predsample > 32767)
        predsample = 32767;
    else if (predsample < -32768)
        predsample = -32768;

    // find new stepsize index
    index += indexTable[code];
    if (index <0)
        index = 0;
    else if (index > 88)
        index = 88;

    return (code & 0x0f);
}
