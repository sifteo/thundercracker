/*
 * Thundercracker Firmware -- Confidential, not for redistribution.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#include "motion.h"


_SYSByte4 MotionUtil::captureAccelState(const RF_ACKType &ack)
{
    /*
     * All bytes in protocol happen to be inverted
     * relative to the SDK's coordinate system.
     */

    _SYSByte4 state;
    state.x = -ack.accel[0];
    state.y = -ack.accel[1];
    state.z = -ack.accel[2];
    state.w = 0;
    return state;
}

void MotionWriter::write(_SYSByte4 reading, SysTime::Ticks timestamp)
{
    /*
     * Don't trust userspace, or expect mutual exclusion!
     * We must capture the motion buffer pointer exactly once,
     * and validate the tail pointer on read.
     */

    _SYSMotionBuffer *buffer = mbuf;
    unsigned tail = buffer->header.tail;
    unsigned last = buffer->header.last;
    if (tail > last)
        tail = 0;

    /*
     * Quantize the time difference between the last reading and this one.
     * We need to make sure to propagate quantization errors forward, so they
     * don't accumulate.
     */

    SysTime::Ticks timeDelta = timestamp - lastTimestamp;
    const SysTime::Ticks timeUnit = SysTime::nsTicks(_SYS_MOTION_TIMESTAMP_NS);
    unsigned tickDelta = timeDelta / timeUnit;

    if (tickDelta < 1) {
        /*
         * Too soon since the last event. Replace the last event we buffered,
         * but don't update the timestamps at all.  (It would be a little easier
         * to just drop this event, but that's bad if this happens to be the
         * last event we'll see in a while- we leave a stale state in the newest
         * slot in our buffer.)
         */

        uint8_t previous = tail ? (tail - 1) : last;
        _SYSByte4 &prevSlot = buffer->buf[previous];

        // Copy timestamp byte from previous slot
        reading.w = prevSlot.w;

        // Overwrite the whole sample atomically
        prevSlot.value = reading.value;
        return;
    }

    // Move our timestamp forward by the quantized amount
    lastTimestamp += tickDelta * timeUnit;

    /*
     * Has it been so long since the last event that we'll end up replacing
     * the entire contents of the buffer? If so, we can clamp the delta to
     * that amount.
     */

    const unsigned maxTickDelta = (last + 1) << 8;
    tickDelta = MIN(tickDelta, maxTickDelta);

    /*
     * Write multiple entries if necessary to express our total tickDelta.
     */

    while (tickDelta) {
        unsigned eventDelta = MIN(tickDelta, 256);
        tickDelta -= eventDelta;
        reading.w = eventDelta - 1;

        buffer->buf[tail].value = reading.value;
        tail++;
        if (tail > last)
            tail = 0;
    }

    // Make new data available to userspace
    buffer->header.tail = tail;
}
