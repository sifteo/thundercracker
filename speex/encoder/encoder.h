
#ifndef _ENCODER_H
#define _ENCODER_H

#include <stdio.h>
#include <iostream>
#include <vector>
#include <speex/speex.h>

class Encoder {
public:
    Encoder();
    ~Encoder();
    bool addOutput(const char *path);
    bool encode(const char *configpath, int channels, int format);
    
private:
    // max sizes from speexenc
    static const unsigned MAX_FRAME_SIZE     = 2000;
    static const unsigned MAX_FRAME_BYTES    = 2000;

    const char* headerFile;
    const char* sourceFile;
    
    SpeexBits bits;
    void *encoderState;
    int frameSize;
    
    void collectInputs(const char *path, std::vector<std::string> &inputs);
    int encodeFile(const std::string &dir, const std::string &file, int channels, int format, std::ofstream & headerStream, std::ofstream & srcStream);
#if 0
    void calculateSNR(FILE *foriginal, FILE *froundtrip, float *snr, float *seg_snr);
#endif
};

#endif
