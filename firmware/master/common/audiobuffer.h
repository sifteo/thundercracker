/*
 * Thundercracker Firmware -- Confidential, not for redistribution.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#ifndef AUDIOBUFFER_H_
#define AUDIOBUFFER_H_

#include <sifteo/audio.h>

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
        return (((sys->tail + 1) & (sizeof(sys->buf) - 1)) == sys->head);
    }

    inline bool empty() const {
        return (sys->head == sys->tail);
    }

    unsigned readAvailable() const;
    unsigned writeAvailable() const;

    uint8_t *reserve(unsigned numBytes, unsigned *outBytesAvailable);
    void commit(unsigned numBytes);

private:
    _SYSAudioBuffer *sys;                       // provided by userspace
    // to support reserve/commit API, when we need to wrap around the end of our
    // actual sys buffer, provide a coalescer
    uint8_t coalescer[_SYS_AUDIO_BUF_SIZE];
};

#endif // AUDIOBUFFER_H_
