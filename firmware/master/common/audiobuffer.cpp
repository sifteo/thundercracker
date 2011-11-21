
#include "audiobuffer.h"
#include <sifteo/macros.h>

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
    sys->buf[sys->tail] = c;
    sys->tail = (sys->tail + 1) & (sizeof(sys->buf) - 1);
    ASSERT(sys->tail != sys->head);
}

uint8_t AudioBuffer::dequeue()
{
    ASSERT(isValid());
    ASSERT(!empty());
    ASSERT(sys->head < sizeof(sys->buf));
    uint8_t c = sys->buf[sys->head];
    sys->head = (sys->head + 1) & (sizeof(sys->buf) - 1);
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
    return ((sys->head > sys->tail) ? sizeof(sys->buf) : 0) + sys->tail - sys->head;
}

unsigned AudioBuffer::writeAvailable() const
{
    ASSERT(isValid());
    return (sizeof(sys->buf) - 1) - readAvailable();
}

