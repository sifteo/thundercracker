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
#define SPEEX_MODE  SPEEX_MODEID_WB


AudioEncoder *AudioEncoder::create(std::string name, float quality, bool vbr)
{
    std::transform(name.begin(), name.end(), name.begin(), ::tolower);

    if (name == "pcm")
        return new PCMEncoder();

    if (name == "speex" || name == "")
        return new SpeexEncoder(quality, vbr);

    return 0;
}


SpeexEncoder::SpeexEncoder(float quality, bool vbr) :
    frameSize(0)
{
    encoderState = speex_encoder_init(speex_lib_get_mode(SPEEX_MODE));
    speex_bits_init(&bits);

    spx_int32_t complexity = 10;     // Unbounded CPU for encoding
    speex_encoder_ctl(encoderState, SPEEX_SET_COMPLEXITY, &complexity);

    spx_int32_t vbrEnabled = vbr;
    speex_encoder_ctl(encoderState, SPEEX_SET_VBR, &vbrEnabled);
    if (vbr) {
        // Make sure VBR support is compiled in!
        vbrEnabled = 0;
        speex_encoder_ctl(encoderState, SPEEX_GET_VBR, &vbrEnabled);
        assert(vbrEnabled == 1);
    }

    if (vbr) {
        speex_encoder_ctl(encoderState, SPEEX_SET_VBR_QUALITY, &quality);
    } else {
        spx_int32_t iQuality = quality + 0.6;
        speex_encoder_ctl(encoderState, SPEEX_SET_QUALITY, &iQuality);
    }

    speex_encoder_ctl(encoderState, SPEEX_GET_FRAME_SIZE, &frameSize);
}

SpeexEncoder::~SpeexEncoder()
{
    speex_bits_destroy(&bits);
    if (encoderState) {
        speex_encoder_destroy(encoderState);
    }
}

void SpeexEncoder::encodeFile(const std::string &path,
    std::vector<uint8_t> &out, float &kbps)
{
    FILE *fin = fopen(path.c_str(), "rb");
    if (fin == 0)
        return;

    short inbuf[MAX_FRAME_SIZE];
    char outbuf[MAX_FRAME_BYTES];
    unsigned totalSamples = 0;

    for (;;) {
        int rx = fread(inbuf, sizeof(short), frameSize, fin);
        if (feof(fin) && rx == 0)
            break;

        totalSamples += rx;
        if (rx < frameSize) {
            // Pad with silence
            memset(inbuf + rx, 0, (frameSize - rx) * sizeof(short));
        }

        // encode this frame and write it to our raw encoded file
        speex_bits_reset(&bits);
        speex_encode_int(encoderState, inbuf, &bits);

        int nbBytes = speex_bits_write(&bits, outbuf, sizeof(outbuf));
        assert(nbBytes > 0 && "frame shouldn't be empty?");
        assert(nbBytes < 0xFF && "frame is too large for format :(");

        out.push_back((uint8_t) nbBytes);
        out.insert(out.end(), &outbuf[0], &outbuf[nbBytes]);
    }

    // Calculate final bit rate
    kbps = (8.0f * SAMPLE_RATE * out.size()) / (1000.0f * totalSamples);
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
