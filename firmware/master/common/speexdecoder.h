/*
 * Thundercracker Firmware -- Confidential, not for redistribution.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#ifndef SPEEXDECODER_H_
#define SPEEXDECODER_H_

#include "audiobuffer.h"
#include "flashlayer.h"
#include "speex/speex.h"
#include "../speex/STM32/config.h"

#include <stdint.h>

#ifndef SIFT_SPEEX_MODE
#error "SIFT_SPEEX_MODE not defined"
#endif


class SpeexDecoder
{
public:
#if SIFT_SPEEX_MODE == SPEEX_MODEID_NB
    static const unsigned DECODED_FRAME_SIZE = 160 * sizeof(short);
#elif SIFT_SPEEX_MODE == SPEEX_MODEID_WB
    static const unsigned DECODED_FRAME_SIZE = 320 * sizeof(short);
#elif SIFT_SPEEX_MODE == SPEEX_MODEID_UWB
    static const unsigned DECODED_FRAME_SIZE = 640 * sizeof(short);
#else
#error "Unknown SIFT_SPEEX_MODE defined"
#endif

    void init();
    bool decodeFrame(FlashStream &in, AudioBuffer &out);

private:
    void* decodeState;
};


#endif /* SPEEXDECODER_H_ */
