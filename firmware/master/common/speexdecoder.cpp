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
    if (decodeState)
        speex_decoder_destroy(decodeState);

    decodeState = speex_decoder_init(speex_lib_get_mode(SIFT_SPEEX_MODE));
    ASSERT(decodeState != 0);

    int enh = 1;
    speex_decoder_ctl(decodeState, SPEEX_SET_ENH, &enh); // enhancement on

#ifdef DEBUG
    // verify framesize
    int framesize;
    speex_decoder_ctl(decodeState, SPEEX_GET_FRAME_SIZE, &framesize);
    if (framesize != (DECODED_FRAME_SIZE / sizeof(short))) {
        while (1);
    }
#endif
}


bool SpeexDecoder::decodeFrame(FlashStream &in, AudioBuffer &out)
{
    unsigned outAvailable;
    uint8_t *outBuf = out.reserve(DECODED_FRAME_SIZE, &outAvailable);
    if (outAvailable != DECODED_FRAME_SIZE)
        return true;   // Output flow control; nothing to do yet.

    // Temporary buffer for Speex bits
    FlashStreamBuffer<256> inStream;
    SpeexBits inBits;
    inStream.reset();

    // Each frame is prefixed with a uint8_t size
    uint8_t *pSize = inStream.read(in, 1);
    if (!pSize)
        return false;   // Unexpected EOF
    uint8_t frameSize = *pSize;
    if (frameSize == 0)
        return false;   // Encoding error

    uint8_t *frameData = inStream.read(in, frameSize);
    if (!frameData)
        return false;   // Unexpected EOF

    speex_bits_set_bit_buffer(&inBits, (void*) frameData, frameSize);

    if (speex_decode_int(this->decodeState, &inBits, (spx_int16_t*)outBuf))
        return false;

    out.commit(DECODED_FRAME_SIZE);
    return true;
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
