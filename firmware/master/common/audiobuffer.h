/*
 * Thundercracker Firmware -- Confidential, not for redistribution.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#ifndef AUDIOBUFFER_H_
#define AUDIOBUFFER_H_

#include <sifteo/abi.h>


class AudioBuffer {
public:
    AudioBuffer() : sys(0)
    {}

    void init(_SYSAudioBuffer *buf);
    bool isValid() const {
        return sys != 0;
    }

    inline unsigned capacity() const {
        return sizeof(sys->buf) - 1;
    }

    void enqueue(uint8_t c);
    uint8_t dequeue();

    void write(const uint8_t *buf, int len);
    int read(uint8_t *buf, int len);

    inline bool full() const {
        return mask(sys->tail + 1) == mask(sys->head);
    }

    inline bool empty() const {
        return mask(sys->head) == mask(sys->tail);
    }

    unsigned readAvailable() const;
    unsigned writeAvailable() const;

    uint8_t *reserve(unsigned numBytes, unsigned *outBytesAvailable);
    void commit(unsigned numBytes);

private:
    _SYSAudioBuffer *sys;                       // provided by userspace

    // Mask off invalid bits from head/tail addresses.
    // This must be done on *read* from the buffer.
    inline unsigned mask(unsigned x) const {
        return x & (sizeof(sys->buf) - 1);
    }

    // to support reserve/commit API, when we need to wrap around the end of our
    // actual sys buffer, provide a coalescer.
    // XXX: If we do still need this, it should be allocated on the stack or
    //      shared globally. This is negating the whole RAM footprint benefit
    //      of keeping buffers in userspace!
    uint8_t coalescer[_SYS_AUDIO_BUF_SIZE];
};

#endif // AUDIOBUFFER_H_
