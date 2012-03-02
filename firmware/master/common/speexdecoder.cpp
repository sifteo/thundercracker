/*
 * Thundercracker Firmware -- Confidential, not for redistribution.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#include <string.h>
#include "speexdecoder.h"
#include "flashlayer.h"
#include <sifteo/macros.h>
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

void SpeexDecoder::init()
{
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

void SpeexDecoder::deinit()
{
    speex_bits_destroy(&this->bits); // remove this once we use speex_bits_init_buffer in init()
    if (this->decodeState) {
        speex_decoder_destroy(this->decodeState);
        this->decodeState = 0;
    }
}

int SpeexDecoder::decodeFrame(FlashStream &in, uint8_t *dest, uint32_t destSize)
{
    if (destSize < (int)DECODED_FRAME_SIZE) {
        return 0;
    }

    FlashRegion fr1;
    if (!FlashLayer::getRegion(this->srcaddr, DECODED_FRAME_SIZE + 1, &fr1)) {
        return 0; // ruh roh, TODO error handling
    }

    char contiguous[FlashLayer::BLOCK_SIZE];
    char *localAddr = static_cast<char*>(fr1.data());

    // format: uint8_t of framesize, followed by framesize bytes of frame data
    unsigned sz = *localAddr++;
    unsigned fr1size = fr1.size();
    memcpy(contiguous, localAddr, fr1size - 1);
    FlashLayer::releaseRegion(fr1);

    // for the given sz, do we have enough left?
    if ((unsigned)this->srcBytesRemaining < (sz + 1)) {
        status = EndOfStream;
        return 0;   // not enough data left
    }

    if (sz > fr1size - 1) {
        // the requested size straddles flash block boundaries. fetch a second
        // block so we can stich them together
        FlashRegion fr2;
        if (!FlashLayer::getRegion(this->srcaddr + fr1size, DECODED_FRAME_SIZE + 1, &fr2)) {
            return 0; // ruh roh, TODO error handling
        }
        memcpy(contiguous + fr1size - 1, fr2.data(), sz - fr1size);
        FlashLayer::releaseRegion(fr2);
    }

    speex_bits_read_from(&this->bits, contiguous, sz);
    this->srcBytesRemaining -= (sz + 1);
    this->srcaddr += (sz + 1);

    this->status = (DecodeStatus)speex_decode_int(this->decodeState, &this->bits, (spx_int16_t*)buf);
    if (this->srcBytesRemaining <= 0) {
        status = EndOfStream;
    }

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
