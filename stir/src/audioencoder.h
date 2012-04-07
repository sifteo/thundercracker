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


class AudioEncoder {
public:
    virtual ~AudioEncoder() {}
    virtual void encodeFile(const std::string &path, std::vector<uint8_t> &out, float &kbps) = 0;

    virtual const char *getTypeSymbol() = 0;
    virtual const char *getName() = 0;
    
    static AudioEncoder *create(std::string name, float quality, bool vbr);
};


class PCMEncoder : public AudioEncoder {
public:
    virtual void encodeFile(const std::string &path, std::vector<uint8_t> &out, float &kbps);

    virtual const char *getTypeSymbol() {
        return "_SYS_PCM";
    }

    virtual const char *getName() {
        return "Uncompressed PCM";
    }
};

class ADPCMEncoder : public AudioEncoder {
public:
    ADPCMEncoder() :
        index(0),
        predsample(0)
    {}
    virtual void encodeFile(const std::string &path, std::vector<uint8_t> &out, float &kbps);

    virtual const char *getTypeSymbol() {
        return "_SYS_ADPCM";
    }

    virtual const char *getName() {
        return "IMA 4-bit ADPCM";
    }

private:
    int16_t index;
    int32_t predsample;

    static const uint16_t stepSizeTable[];
    static const int8_t indexTable[];

    uint8_t encodeSample(int16_t sample);
};

#endif
