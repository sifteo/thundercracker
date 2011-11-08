/*
 * This file is part of the internal implementation of the Sifteo SDK.
 * Confidential, not for redistribution.
 *
 * Copyright <c> 2011 Sifteo, Inc. All rights reserved.
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

    void push(uint8_t c);
    uint8_t pop();

    void write(const uint8_t *buf, int len);
    int read(uint8_t *buf, int len);

    inline bool full() const {
        return (((sys->tail + 1) & (sizeof(sys->data) - 1)) == sys->head);
    }

    inline bool empty() const {
        return (sys->head == sys->tail);
    }

    unsigned readAvailable() const;
    unsigned writeAvailable() const;

private:
    _SYSAudioBuffer *sys; // provided by userspace
};

#endif // AUDIOBUFFER_H_
