/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * STIR -- Sifteo Tiled Image Reducer
 * Micah Elizabeth Scott <micah@misc.name>
 *
 * Copyright <c> 2011 Sifteo, Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#ifndef _ENCODER_H
#define _ENCODER_H

#include <stdio.h>
#include <vector>
#include <string>
#include "sifteo/abi.h"


class AudioEncoder {
public:
    virtual ~AudioEncoder() {}

    virtual void encode(const std::vector<uint8_t> &in, std::vector<uint8_t> &out) = 0;

    virtual const char *getTypeSymbol() = 0;
    virtual const char *getName() = 0;
    virtual const _SYSAudioType getType() = 0;
    
    static AudioEncoder *create(std::string name);
};


class PCMEncoder : public AudioEncoder {
public:
    virtual void encode(const std::vector<uint8_t> &in, std::vector<uint8_t> &out);

    virtual const char *getTypeSymbol() {
        return "_SYS_PCM";
    }

    virtual const _SYSAudioType getType() {
        return _SYS_PCM;
    }

    virtual const char *getName() {
        return "Uncompressed PCM";
    }
};


class ADPCMEncoder : public AudioEncoder {
public:
    static const unsigned HEADER_SIZE = 3;
    static const unsigned INDEX_MAX = 88;

    virtual void encode(const std::vector<uint8_t> &in, std::vector<uint8_t> &out);

    virtual const char *getTypeSymbol() {
        return "_SYS_ADPCM";
    }

    virtual const _SYSAudioType getType() {
        return _SYS_ADPCM;
    }

    virtual const char *getName() {
        return "ADPCM";
    }

private:
    struct State {
        unsigned index;
        int sample;
    };

    static void optimizeIC(State &state, const std::vector<uint8_t> &in);
    static uint64_t encodeWithIC(State state, const std::vector<uint8_t> &in,
        std::vector<uint8_t> &out, unsigned inBytes);

    static unsigned encodeSample(State &state, int sample);
    static uint64_t encodePair(State &state, int s1, int s2, std::vector<uint8_t> &out);
};

#endif
