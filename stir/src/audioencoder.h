/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * STIR -- Sifteo Tiled Image Reducer
 * M. Elizabeth Scott <beth@sifteo.com>
 *
 * Copyright <c> 2011 Sifteo, Inc. All rights reserved.
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
