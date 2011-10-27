
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
    static const int MAX_FRAME_SIZE     = 2000;
    static const int MAX_FRAME_BYTES    = 2000;
    static const int DECODED_FRAME_SIZE = 160;

    const char* headerFile;
    const char* sourceFile;
    
    SpeexBits bits;
    void *encoderState;
    
    std::string directoryPath(const char *filepath);
    std::string baseName(const std::string &filepath);
    std::string guardName(const char *filepath);
    
    void collectInputs(const char *path, std::vector<std::string> &inputs);
    int encodeFile(const std::string &path, int channels, int format, std::ofstream & headerStream, std::ofstream & srcStream);
    int readSamples(int framesize, int bits, int channels, int lsb, short *input, std::ifstream & ins);
};

#endif
