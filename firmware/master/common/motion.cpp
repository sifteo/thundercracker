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

void MotionUtil::integrate(const _SYSMotionBuffer *mbuf, unsigned duration, _SYSInt3 *result)
{
    /*
     * Integrate the last 'duration' ticks worth of data from the motion buffer,
     * returning the sum of these ticks in 'result'. If 'now' is the current time,
     * this integrates from (now - duration) to (now).
     *
     * Since we have a set of discrete timestamped samples, we need to approximate
     * the value of samples that lie in-between our buffer's actual samples. To do
     * this efficiently, we use the trapezoidal rule. Each buffered sample is treated
     * as a trapezoid which extends from the previous sample to itself.
     */

    // Work backwards from the current position
    uint8_t tail = mbuf->header.tail;
    uint8_t last = mbuf->header.last;
    uint8_t head = tail;

    // Point to the last stored sample
    head--;
    head = MIN(head, last);

    // Keep a two-sample buffer, representing the two edges of our trapezoid
    _SYSByte4 sampleNext;
    _SYSByte4 samplePrev = mbuf->buf[head];

    int x = 0, y = 0, z = 0;

    while (duration) {
        unsigned ticks;

        if (head == tail) {
            /*
             * Out of samples! Use a duplicate of the next sample, with
             * an arbitrarily long time delta. This effectively assumes that
             * prior to the beginning of the buffer, we were stuck with the
             * same sample forever.
             */

            sampleNext.value = samplePrev.value;
            ticks = duration;

        } else {
            // Roll back by one sample
            head--;
            head = MIN(head, last);

            // Shift new sample into buffer
            sampleNext.value = samplePrev.value;
            samplePrev.value = mbuf->buf[head].value;

            // Width of our trapezoid is the distance from prev to next, conveniently
            // already encoded in next's timestamp byte.
            ticks = unsigned(uint8_t(sampleNext.w)) + 1;
        }

        int pX = samplePrev.x;
        int pY = samplePrev.y;
        int pZ = samplePrev.z;

        int nX = sampleNext.x;
        int nY = sampleNext.y;
        int nZ = sampleNext.z;

        if (ticks <= duration) {
            /*
             * The Trapezoidal rule is equivalent to taking an average of the current
             * and the last sample, and weighting that average according to the distance
             * between the two samples divided by two. To avoid discarding precision,
             * we skip this division by two.
             */

            x += ticks * (nX + pX);
            y += ticks * (nY + pY);
            z += ticks * (nZ + pZ);

            duration -= ticks;

        } else {
            /*
             * This trapezoid is larger than the remainder of our integration window.
             * We need to slice it, effectively using linear interpolation to calculate
             * a substitute for samplePrev which is exactly 'duration' ticks prior to
             * sampleNext.
             */

            int interpX = nX + (pX - nX) * duration / ticks;
            int interpY = nY + (pY - nY) * duration / ticks;
            int interpZ = nZ + (pZ - nZ) * duration / ticks;

            x += duration * (nX + interpX);
            y += duration * (nY + interpY);
            z += duration * (nZ + interpZ);

            break;
        }
    }

    result->x = x;
    result->y = y;
    result->z = z;
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
