/*
 * Thundercracker Firmware -- Confidential, not for redistribution.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */
 
/*
 * Remember that _SYSAudioBuffer is in userspace! We can verify that 'sys'
 * points to real user RAM once, but we can never trust the values we pull
 * from this buffer.
 */

#include "audiobuffer.h"
#include "macros.h"


void AudioBuffer::init(_SYSAudioBuffer *buf) {
    STATIC_ASSERT((sizeof(buf->buf) & (sizeof(buf->buf) - 1)) == 0); // must be power of 2
    ASSERT(sys == 0); // should only get initialized once
    sys = buf;
    sys->head = sys->tail = 0;
}

void AudioBuffer::enqueue(uint8_t c)
{
    ASSERT(isValid());
    ASSERT(!full());
    
    unsigned tail = mask(sys->tail);
    sys->buf[tail] = c;
    sys->tail = tail + 1;

    ASSERT(mask(sys->tail) != mask(sys->head));
}

uint8_t AudioBuffer::dequeue()
{
    ASSERT(isValid());
    ASSERT(!empty());

    unsigned head = mask(sys->head);
    uint8_t c = sys->buf[head];
    sys->head = head + 1;

    return c;
}

void AudioBuffer::write(const uint8_t *buf, int len)
{
    // for now. optimize later
    while (len--) {
        enqueue(*buf++);
    }
}

int AudioBuffer::read(uint8_t *buf, int len)
{
    // for now. optimize later
    int count = len;
    while (len--) {
        *buf++ = dequeue();
    }
    return count;
}

unsigned AudioBuffer::readAvailable() const
{
    ASSERT(isValid());
    unsigned head = mask(sys->head);
    unsigned tail = mask(sys->tail);
    return ((head > tail) ? sizeof(sys->buf) : 0) + tail - head;
}

unsigned AudioBuffer::writeAvailable() const
{
    ASSERT(isValid());
    return capacity() - readAvailable();
}

/*
    XXX: this reserve/commit implementation is quite fake at the moment.
    We should be able to write directly into our real buffer instead of always
    into our coalescer buffer, to avoid copying. At least now, clients can avoid
    copying, and we can implement it later.
*/
uint8_t *AudioBuffer::reserve(unsigned numBytes, unsigned *outBytesAvailable)
{
    ASSERT(isValid());
    // TODO - give a pointer to our buffer directly if possible to avoid copying
    *outBytesAvailable = MIN(numBytes, writeAvailable());
    return coalescer;
}

void AudioBuffer::commit(unsigned numBytes)
{
    ASSERT(isValid());
    // TODO - determine whether data was written directly to our real buffer,
    // and update write pointer atomically.
    for (unsigned i = 0; i < numBytes; ++i) {
        enqueue(coalescer[i]);
    }
}
