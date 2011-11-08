
#include "audiobuffer.h"

void AudioBuffer::init(_SYSAudioBuffer *buf) {
    STATIC_ASSERT((sizeof(buf->data) & (sizeof(buf->data) - 1)) == 0); // must be power of 2
    ASSERT(sys == 0); // should only get initialized once
    sys = buf;
    sys->head = sys->tail = 0;
}

void AudioBuffer::push(uint8_t c)
{
    ASSERT(isValid());
    ASSERT(!full());
    sys->data[sys->tail] = c;
    sys->tail = (sys->tail + 1) & (sizeof(sys->data) - 1);
    ASSERT(sys->tail != sys->head);
}

uint8_t AudioBuffer::pop()
{
    ASSERT(isValid());
    ASSERT(!empty());
    ASSERT(sys->head < sizeof(sys->data));
    uint8_t c = sys->data[sys->head];
    sys->head = (sys->head + 1) & (sizeof(sys->data) - 1);
    return c;
}

void AudioBuffer::write(const uint8_t *buf, int len)
{
    // for now. optimize later
    while (len--) {
        push(*buf++);
    }
}

int AudioBuffer::read(uint8_t *buf, int len)
{
    // for now. optimize later
    int count = len;
    while (len--) {
        *buf++ = pop();
    }
    return count;
}

unsigned AudioBuffer::readAvailable() const
{
    ASSERT(isValid());
    return ((sys->head > sys->tail) ? sizeof(sys->data) : 0) + sys->tail - sys->head;
}

unsigned AudioBuffer::writeAvailable() const
{
    ASSERT(isValid());
    return (sizeof(sys->data) - 1) - readAvailable();
}

