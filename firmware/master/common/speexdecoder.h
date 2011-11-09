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

class SpeexDecoder
{
public:
    static const unsigned DECODED_FRAME_SIZE = 160 * sizeof(short);

    enum DecodeStatus {
        Ok = 0,
        EndOfStream = -1,
        CorruptStream = -2
    };

    SpeexDecoder();
    void init();
    void deinit();

    void setData(const uint8_t *srcaddr, int size);
    int decodeFrame(uint8_t *buf, int size);
    bool endOfStream() const {
        return (status == Ok) ? srcBytesRemaining <= 0 : true;
    }

private:
    void* decodeState;
    SpeexBits bits;
    uint32_t srcaddr;
    int srcBytesRemaining;
    DecodeStatus status;
};

#endif /* SPEEXDECODER_H_ */
