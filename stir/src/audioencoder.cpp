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

AudioEncoder *AudioEncoder::create(std::string name)
{
    std::transform(name.begin(), name.end(), name.begin(), ::tolower);

    if (name == "pcm")
        return new PCMEncoder();

    if (name == "adpcm" || name == "")
        return new ADPCMEncoder();

    return 0;
}


void PCMEncoder::encodeFile(const std::string &path,
    std::vector<uint8_t> &out)
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
}

uint32_t PCMEncoder::encodeBuffer(void *buf, uint32_t bufsize)
{
    // buffer is already PCM!
    return bufsize;
}

void ADPCMEncoder::encodeFile(const std::string &path, std::vector<uint8_t> &out)
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

    fclose(fin);
}

uint32_t ADPCMEncoder::encodeBuffer(void *buf, uint32_t bufsize)
{
    int16_t *rbuf = (int16_t *)buf;
    uint8_t *wbuf = (uint8_t *)buf;
    uint32_t r = 0;
    uint32_t w = 0;

    // In-place compression of samples
    while (r < bufsize / sizeof(int16_t)) {
        int16_t samples[2] = {rbuf[r++], 0};
        if (r < bufsize / sizeof(int16_t)) samples[1] = rbuf[r++];

        wbuf[w++] = encodeSample(samples[0]) | (encodeSample(samples[1]) << 4);
    }

    return w;
}

unsigned ADPCMEncoder::encodeSample(int sample)
{
    /*
     * Important: This isn't *quite* standard IMA ADPCM. The rounding
     * rules are a little different, in order to support a fast implementation
     * on ARM with multiply and shift.
     */
    
    static const uint16_t stepSizeTable[89] = {
        7, 8, 9, 10, 11, 12, 13, 14, 16, 17, 19, 21, 23, 25, 28, 31,
        34, 37, 41, 45, 50, 55, 60, 66, 73, 80, 88, 97, 107, 118, 130,
        143, 157, 173, 190, 209, 230, 253, 279, 307, 337, 371, 408, 449,
        494, 544, 598, 658, 724, 796, 876, 963, 1060, 1166, 1282, 1411,
        1552, 1707, 1878, 2066, 2272, 2499, 2749, 3024, 3327, 3660, 4026,
        4428, 4871, 5358, 5894, 6484, 7132, 7845, 8630, 9493, 10442, 11487,
        12635, 13899, 15289, 16818, 18500, 20350, 22385, 24623, 27086, 29794, 32767
    };

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

    int step = stepSizeTable[index];

    // Difference between new sample and old prediction
    int diff = sample - predsample;

    // Find the best nybble for this diff
    unsigned bestCode = 0;
    int bestDiff = 0x100000;
    for (unsigned code = 0; code < 16; ++code) {
        int thisDiff = int(unsigned(int8_t(codeTable[code])) * unsigned(step)) >> 3;
        int thisError = std::max(thisDiff - diff, diff - thisDiff);
        int bestError = std::max(bestDiff - diff, diff - bestDiff);
        if (thisError <= bestError) {
            bestDiff = thisDiff;
            bestCode = code;
        }
    }

    // Update prediction
    predsample = std::min(32767, std::max(-32768, predsample + bestDiff));

    // Update quantizer step size
    int nextIndex = index + (codeTable[bestCode] >> 8);
    nextIndex = std::max(nextIndex, 0);
    nextIndex = std::min(nextIndex, 88);
    index = nextIndex;

    return bestCode;
}
