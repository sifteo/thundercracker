
#ifndef _ENCODER_H
#define _ENCODER_H

#include <stdio.h>
#include <iostream>
#include <vector>
#include <speex/speex.h>

class SpeexEncoder {
public:
    SpeexEncoder();
    ~SpeexEncoder();
    
    int encodeFile(const std::string &file, int channels, int format, std::ofstream & outStream);
    
private:
    // max sizes from speexenc
    static const unsigned MAX_FRAME_SIZE     = 2000;
    static const unsigned MAX_FRAME_BYTES    = 2000;
    
    SpeexBits bits;
    void *encoderState;
    int frameSize;
};

#endif
