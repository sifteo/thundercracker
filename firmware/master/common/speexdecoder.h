/*
 * This file is part of the internal implementation of the Sifteo SDK.
 * Confidential, not for redistribution.
 *
 * Copyright <c> 2011 Sifteo, Inc. All rights reserved.
 */

#ifndef SPEEXDECODER_H_
#define SPEEXDECODER_H_

#include <stdint.h>
#include "speex/speex.h"
#include <../speex/STM32/config.h>

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

    enum DecodeStatus {
        Ok = 0,
        EndOfStream = -1,
        CorruptStream = -2
    };

    SpeexDecoder();
    void init();
    void deinit();

    void setData(const uint8_t *srcaddr, int size);
    void setOffset(const uint32_t offset, int size);
    int decodeFrame(uint8_t *buf, int size);
    bool endOfStream() const {
        return (status == Ok) ? srcBytesRemaining <= 0 : true;
    }

private:
    void* decodeState;
    SpeexBits bits;
    uintptr_t srcaddr;
    //uint32_t srcaddr;
    int srcBytesRemaining;
    DecodeStatus status;
};

#endif /* SPEEXDECODER_H_ */
