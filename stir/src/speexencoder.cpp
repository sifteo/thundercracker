#include "speexencoder.h"

#include <errno.h>
#include <string.h>
#include <string>
#include <fstream>
#include <vector>
#include <assert.h>

using namespace std;

// #define SAMPLE_RATE 8000
// #define SPEEX_MODE  SPEEX_MODEID_NB

#define SAMPLE_RATE 16000
#define SPEEX_MODE  SPEEX_MODEID_WB

// #define SAMPLE_RATE 32000
// #define SPEEX_MODE  SPEEX_MODEID_UWB

SpeexEncoder::SpeexEncoder(int quality /* = 8 */) :
    frameSize(0)
{
    encoderState = speex_encoder_init(speex_lib_get_mode(SPEEX_MODE));
    speex_bits_init(&bits);

    // 8 is the default for speexenc 
    speex_encoder_ctl(encoderState, SPEEX_SET_QUALITY, &quality);
    speex_encoder_ctl(encoderState,SPEEX_GET_FRAME_SIZE, &frameSize);
}

SpeexEncoder::~SpeexEncoder()
{
    speex_bits_destroy(&bits);
    if (encoderState) {
        speex_encoder_destroy(encoderState);
    }
}

int SpeexEncoder::encodeFile(const std::string &path, int channels, int format, std::ofstream & outStream)
{
    fprintf(stdout, "SPEEX ENCODING...\n");
    
    FILE *fin = fopen(path.c_str(), "rb");
    if (fin == 0) {
        fprintf(stderr, "error: couldn't open %s for reading (%s)\n", path.c_str(), strerror(errno));
        return 0;
    }
    
    short inbuf[MAX_FRAME_SIZE];
    char outbuf[MAX_FRAME_BYTES];
    //char buf[16];
    
    int bytecount = 0;
    int frameCount = 0;
    
    for (;;) {
        int rx = fread(inbuf, sizeof(short), frameSize, fin);
        if (feof(fin) || rx != frameSize) {
            break;
        }
        
        // encode this frame and write it to our raw encoded file
        speex_bits_reset(&bits);
        speex_encode_int(encoderState, inbuf, &bits);
        
        int nbBytes = speex_bits_write(&bits, outbuf, sizeof(outbuf));
        assert(nbBytes < 0xFF && "frame is too large for format :(\n");
        //uint8_t num_bytes = nbBytes & 0xFF;
        //fwrite(&num_bytes, sizeof(uint8_t), 1, encodedOut);
        //fwrite(outbuf, sizeof(uint8_t), nbBytes, encodedOut);
        
        uint8_t num_bytes = nbBytes & 0xFF;
        outStream.write((char *)&num_bytes, sizeof(uint8_t));
        outStream.write(outbuf, nbBytes);
        
        bytecount += (nbBytes + 1);
        
        frameCount++;
    }
    
    fprintf(stdout, "Speex - byteCount: %d, frameCount: %d\n", bytecount, frameCount);
    
    return bytecount;
}




PCMEncoder::PCMEncoder()
{
}

PCMEncoder::~PCMEncoder()
{
}

int PCMEncoder::encodeFile(const std::string &path, std::ofstream & outStream)
{
    FILE *fin = fopen(path.c_str(), "rb");
    if (fin == 0) {
        fprintf(stderr, "error: couldn't open %s for reading (%s)\n", path.c_str(), strerror(errno));
        return 0;
    }
    
    // just dump the raw file.
    
    char inbuf[512];
    int bytecount = 0;
    
    for (;;) {
        int rx = fread(inbuf, sizeof(char), 512, fin);
        if (feof(fin) && rx == 0) {
            break;
        }
        
        outStream.write(inbuf, rx);
        bytecount += rx;
    }
    
    return bytecount;
}
