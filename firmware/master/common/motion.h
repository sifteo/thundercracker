/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * Thundercracker firmware
 *
 * Copyright <c> 2012 Sifteo, Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#ifndef _MOTION_H
#define _MOTION_H

#include <sifteo/abi.h>
#include <protocol.h>
#include "macros.h"
#include "systime.h"


/**
 * Static utilities for dealing with motion buffers
 */

class MotionUtil {
public:

    /// Convert a raw ACK buffer to a packed _SYSByte4 accelerometer reading.
    static _SYSByte4 captureAccelState(const RF_ACKType &ack, uint8_t cubeVersion);

    /// Numeric integral using the trapezoidal rule. Result is scaled by 2.
    static void integrate(const _SYSMotionBuffer *mbuf, unsigned duration, _SYSInt3 *result);

    /// Compute a component-wise time-weighted median of the acceleration data, over the specified duration
    static void median(const _SYSMotionBuffer *mbuf, unsigned duration, _SYSMotionMedian *result);

private:
    MotionUtil();    // Do not implement

    static void medianAxis(const _SYSMotionBuffer *mbuf, unsigned duration, unsigned shift, _SYSMotionMedianAxis &result);
};


/**
 * MotionWriter knows how to enqueue raw accelerometer samples
 * into a userspace _SYSMotionBuffer. This is used by CubeSlot to
 * buffer accelerometer data received in its radio ACK callback.
 */

class MotionWriter {
public:
	ALWAYS_INLINE void setBuffer(_SYSMotionBuffer *m) {
		mbuf = m;
	}

	ALWAYS_INLINE bool hasBuffer() {
		return mbuf != 0;
	}

    /// Returns the buffer's suggested rate in ticks, if any, or an arbitrary large value otherwise.
    ALWAYS_INLINE unsigned getBufferRate() const {
        return mbuf ? mbuf->header.rate : -1;
    }

	// Safe to call from ISR context
    void write(_SYSByte4 reading, SysTime::Ticks timestamp);

private:
    SysTime::Ticks lastTimestamp;		// Accessed by ISR only
    _SYSMotionBuffer *mbuf;				// Pointer written on main thread, read on ISR
};


#endif
