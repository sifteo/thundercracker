#include <string.h>
#include "speexdecoder.h"
#include "flashlayer.h"
#include <sifteo/macros.h>
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

SpeexDecoder::SpeexDecoder() :
    decodeState(0),
    srcBytesRemaining(0),
    status(EndOfStream)
{
}

void SpeexDecoder::init()
{
    // speex decoding intilalization
    // speex_bits_init_buffer prevents speex from doing extra reallocs
//    speex_bits_init_buffer(&speexBits, bitsBuffer, sizeof(bitsBuffer));
    // TODO - speex_bits_init_buffer appears to result in different decoded data...
    // would be nicer to use it though...look into it
    speex_bits_init(&this->bits);
    this->decodeState = speex_decoder_init(speex_lib_get_mode(SIFT_SPEEX_MODE));
    if (this->decodeState == 0) {
        while (1); // TODO - handle error
    }
    int enh = 1;
    speex_decoder_ctl(this->decodeState, SPEEX_SET_ENH, &enh); // enhancement on

#ifdef DEBUG
    // verify framesize
    int framesize;
    speex_decoder_ctl(this->decodeState, SPEEX_GET_FRAME_SIZE, &framesize);
    if (framesize != (DECODED_FRAME_SIZE / sizeof(short))) {
        while (1);
    }
#endif
}

void SpeexDecoder::setData(const uint8_t *srcaddr, int size)
{
    // FIXME: audio hacking
    //this->srcaddr = (uintptr_t)srcaddr;
    //this->srcBytesRemaining = size;
    
    //this->srcaddr = 0;  // 20480
    //this->srcaddr = 20480;  // 20480
    //this->srcBytesRemaining = 769924;
    
    status = Ok;
}

void SpeexDecoder::setOffset(const uint32_t offset, int size)
{
    this->srcaddr = offset;
    this->srcBytesRemaining = size;
    
    status = Ok;
}

//SpeexDecoder::~SpeexDecoder()
//{
//    deinit();
//}

void SpeexDecoder::deinit()
{
    speex_bits_destroy(&this->bits); // remove this once we use speex_bits_init_buffer in init()
    if (this->decodeState) {
        speex_decoder_destroy(this->decodeState);
        this->decodeState = 0;
    }
}

/*
 * Return the number of bytes decoded.
 */
int SpeexDecoder::decodeFrame(uint8_t *buf, int size)
{
    if (size < (int)DECODED_FRAME_SIZE || status != Ok) {
        return 0;
    }

    int rsize = 0;

    //char *localAddr = FlashLayer::getRegion(this->srcaddr, DECODED_FRAME_SIZE + sizeof(uint8_t));
    char *localAddr = FlashLayer::getRegionFromOffset(this->srcaddr, DECODED_FRAME_SIZE + sizeof(uint8_t), &rsize);
    if (!localAddr) {
        return 0; // ruh roh, TODO error handling
    }

    //fprintf(stdout, "  wanted %lu bytes, read %d, sz: %d\n", DECODED_FRAME_SIZE + sizeof(uint8_t), rsize, sz);

    // format: uint8_t of framesize, followed by framesize bytes of frame data
    int sz = *localAddr++;
    if ((unsigned)this->srcBytesRemaining < (sz + sizeof(uint8_t))) {
        status = EndOfStream;
        return 0;   // not enough data left
    }

    speex_bits_read_from(&this->bits, localAddr, sz);
    this->srcBytesRemaining -= (sz + sizeof(uint8_t));

    this->status = (DecodeStatus)speex_decode_int(this->decodeState, &this->bits, (spx_int16_t*)buf);
    if (this->srcBytesRemaining <= 0) {
        status = EndOfStream;
    }

    // might be able to do this before the decode step
    //FlashLayer::releaseRegion(this->srcaddr);
    FlashLayer::releaseRegionFromOffset(this->srcaddr);
    this->srcaddr += (sz + sizeof(uint8_t));

    return DECODED_FRAME_SIZE;
}

void _speex_fatal(const char *str, const char *file, int line)
{
    (void)str;
    (void)file;
    (void)line;
    for (;;) {
        ;
    }
}

void _speex_putc(int ch, void *file)
{
    (void)ch;
    (void)file;
    for (;;) {
        ;
    }
}


PCMDecoder::PCMDecoder() :
    //decodeState(0),
    srcBytesRemaining(0),
    status(EndOfStream)
{
}

void PCMDecoder::setOffset(const uint32_t offset, int size)
{
    this->srcaddr = offset;
    this->srcBytesRemaining = size;
    
    status = Ok;
}

int PCMDecoder::decodeFrame(uint8_t *buf, int size)
{
    if (size < (int)FRAME_SIZE || status != Ok) {
        return 0;
    }
    
    if ((unsigned)this->srcBytesRemaining < FRAME_SIZE) {
        status = EndOfStream;
        return 0;   // not enough data left
    }
    
    int rsize = 0;
    
    char *localAddr = FlashLayer::getRegionFromOffset(this->srcaddr, FRAME_SIZE, &rsize);
    if (!localAddr) {
        return 0; // ruh roh, TODO error handling
    }
    
    memcpy(buf, localAddr, FRAME_SIZE);
    //buf = (uint8_t *)localAddr;
    this->srcBytesRemaining -= FRAME_SIZE;
    
    
    FlashLayer::releaseRegionFromOffset(this->srcaddr);
    this->srcaddr += FRAME_SIZE;

    return FRAME_SIZE;
}
