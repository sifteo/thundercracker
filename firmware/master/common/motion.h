/*
 * Thundercracker Firmware -- Confidential, not for redistribution.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
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
    static _SYSByte4 captureAccelState(const RF_ACKType &ack);

    /// Numeric integral using the trapezoidal rule. Result is scaled by 2.
    static void integrate(const _SYSMotionBuffer *mbuf, unsigned duration, _SYSInt3 *result);

private:
    MotionUtil();    // Do not implement
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

	// Safe to call from ISR context
    void write(_SYSByte4 reading, SysTime::Ticks timestamp);

private:
    SysTime::Ticks lastTimestamp;		// Accessed by ISR only
    _SYSMotionBuffer *mbuf;				// Pointer written on main thread, read on ISR
};


#endif
