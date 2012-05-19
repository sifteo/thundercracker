/*
 * Thundercracker Firmware -- Confidential, not for redistribution.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#ifndef RING_BUFFER_H_
#define RING_BUFFER_H_

#include <stdint.h>
#include "macros.h"

template <unsigned _size>
class RingBuffer {
public:
    RingBuffer()
    {}

    void init() {
        STATIC_ASSERT((sizeof(buf) & (sizeof(buf) - 1)) == 0); // must be power of 2
        head = tail = 0;
    }

    inline unsigned capacity() const {
        return sizeof(buf) - 1;
    }

    void enqueue(uint8_t c) {
        ASSERT(!full());

        unsigned masked_tail = mask(tail);
        buf[masked_tail] = c;
        tail = masked_tail + 1;

        ASSERT(mask(tail) != mask(head));
    }

    uint8_t dequeue() {
        ASSERT(!empty());

        unsigned masked_head = mask(head);
        uint8_t c = buf[masked_head];
        head = masked_head + 1;

        return c;
    }

    void write(const uint8_t *buf, int len) {
        // for now. optimize later
        while (len--) {
            enqueue(*buf++);
        }
    }

    int read(uint8_t *buf, int len) {
        // for now. optimize later
        int count = len;
        while (len--) {
            *buf++ = dequeue();
        }
        return count;
    }

    inline bool full() const {
        return mask(tail + 1) == mask(head);
    }

    inline bool empty() const {
        return mask(head) == mask(tail);
    }

    unsigned readAvailable() const {
        unsigned masked_head = mask(head);
        unsigned masked_tail = mask(tail);
        return ((masked_head > masked_tail) ? sizeof(buf) : 0) + masked_tail - masked_head;
    }

    unsigned writeAvailable() const {
        return capacity() - readAvailable();
    }

    /*
        XXX: this reserve/commit implementation is quite fake at the moment.
        We should be able to write directly into our real buffer instead of always
        into our coalescer buffer, to avoid copying. At least now, clients can avoid
        copying, and we can implement it later.
    */
    uint8_t *reserve(unsigned numBytes, unsigned *outBytesAvailable) {
        // TODO - give a pointer to our buffer directly if possible to avoid copying
        *outBytesAvailable = MIN(numBytes, writeAvailable());
        return coalescer;
    }

    void commit(unsigned numBytes) {
        // TODO - determine whether data was written directly to our real buffer,
        // and update write pointer atomically.
        for (unsigned i = 0; i < numBytes; ++i) {
            enqueue(coalescer[i]);
        }
    }

private:
    // Mask off invalid bits from head/tail addresses.
    // This must be done on *read* from the buffer.
    inline unsigned mask(unsigned x) const {
        return x & (sizeof(buf) - 1);
    }

    uint16_t head;          /// Index of the next item to read
    uint16_t tail;          /// Index of the next empty slot to write into
    uint8_t buf[_size];

    // to support reserve/commit API, when we need to wrap around the end of our
    // actual sys buffer, provide a coalescer.
    // XXX: If we do still need this, it should be allocated on the stack or
    //      shared globally. This is negating the whole RAM footprint benefit
    //      of keeping buffers in userspace!
    uint8_t coalescer[_size];
};

#endif // RING_BUFFER_H_
