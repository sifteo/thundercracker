
#include "speexdecoder.h"
#include "flashlayer.h"

SpeexDecoder::SpeexDecoder() :
    decodeState(0),
    srcBytesRemaining(0),
    status(Ok)
{
    // TODO Auto-generated constructor stub
}

void SpeexDecoder::init()
{
    // speex decoding intilalization
    // speex_bits_init_buffer prevents speex from doing extra reallocs
//    speex_bits_init_buffer(&speexBits, bitsBuffer, sizeof(bitsBuffer));
    // TODO - speex_bits_init_buffer appears to result in different decoded data...
    // would be nicer to use it though...look into it
    speex_bits_init(&this->bits);
    this->decodeState = speex_decoder_init(&speex_nb_mode); // narrowband mode
    if (this->decodeState == 0) {
        while (1); // TODO - handle error
    }
    int enh = 0;
    speex_decoder_ctl(this->decodeState, SPEEX_SET_ENH, &enh); // enhancement off

#if 1
    // verify framesize
    int framesize;
    speex_decoder_ctl(this->decodeState, SPEEX_GET_FRAME_SIZE, &framesize);
    if (framesize != DECODED_FRAME_SIZE) {
        while (1); // better error handling
    }
#endif
}

void SpeexDecoder::setData(char *srcaddr, int size)
{
    this->srcaddr = (uint32_t)srcaddr;
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
int SpeexDecoder::decodeFrame(char *buf, int size)
{
    if (size < DECODED_FRAME_SIZE || status != Ok) {
        return 0;
    }

    char *localAddr = FlashLayer::getRegion(this->srcaddr, DECODED_FRAME_SIZE + sizeof(int));
    if (!localAddr) {
        return 0; // ruh roh, TODO error handling
    }

    // format: int of framesize, followed by framesize bytes of frame data
    int sz = *(int*)localAddr;
    localAddr += sizeof(int);
    this->srcBytesRemaining -= sizeof(int);
    if (this->srcBytesRemaining < sz) {
        status = EndOfStream;
        return 0;   // not enough data left
    }

    speex_bits_read_from(&this->bits, localAddr, sz);
    this->srcBytesRemaining -= sz;

    this->status = (DecodeStatus)speex_decode_int(this->decodeState, &this->bits, (spx_int16_t*)buf);

    // might be able to do this before the decode step
    FlashLayer::releaseRegion(this->srcaddr);
    this->srcaddr += (sz + sizeof(int));

    return DECODED_FRAME_SIZE;
}

