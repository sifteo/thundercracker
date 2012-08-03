/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * This file is part of the public interface to the Sifteo SDK.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#pragma once
#ifdef NOT_USERSPACE
#   error This is a userspace-only header, not allowed by the current build.
#endif

#include <sifteo/abi.h>
#include <sifteo/cube.h>
#include <sifteo/math.h>

namespace Sifteo {

/**
 * @defgroup motion Motion
 *
 * Motion tracking subsystem. By attaching MotionBuffer objects to cubes,
 * applications can capture high-frequency motion data and detect gestures
 * using this data.
 *
 * @{
 */


/**
 * @brief A memory buffer which holds captured motion data.
 *
 * This is a FIFO buffer which captures timestamped accelerometer samples.
 * Samples are enqueued and timestamps are generated the moment a relevant
 * packet arrives over the radio from a cube. Timestamps are collected
 * using the system clock, and stored as 8-bit deltas.
 *
 * Like a VideoBuffer, a MotionBuffer can be used with at most one CubeID
 * at a time, but this association can change at runtime. The size of a
 * MotionBuffer may also be modified, in order to trade off between capture
 * duration and memory usage.
 *
 * This captured accelerometer data may be directly read by signal processing
 * code in userspace, or it may be processed with lower CPU overhead with
 * the help of the provided higher-level methods.
 *
 * MotionBuffers are templatized by buffer size. The default should be
 * sufficient for most uses, but if you need to save a longer motion
 * history the size can be adjusted to a maximum of 256 samples.
 */

template < unsigned tSize = 32 >
struct MotionBuffer {
    struct SysType {
        _SYSMotionBufferHeader header;
        _SYSByte4 samples[tSize];
    } sys;

    /// Duration of the MotionBuffer's timestamp unit, in nanoseconds
    static const unsigned TICK_NS = _SYS_MOTION_TIMESTAMP_NS;

    /// Duration of the MotionBuffer's timestamp unit, in microseconds
    static const unsigned TICK_US = _SYS_MOTION_TIMESTAMP_NS / 1000;

    /// Reciprocal of the MotionBuffer's timestamp unit, in Hertz
    static const unsigned TICK_HZ = _SYS_MOTION_TIMESTAMP_HZ;

    // Implicit conversions
    operator _SYSMotionBuffer* () { return reinterpret_cast<_SYSMotionBuffer*>(&sys); }
    operator const _SYSMotionBuffer* () const { return reinterpret_cast<const _SYSMotionBuffer*>(&sys); }

    /**
     * @brief Initialize the MotionBuffer and attach it to a cube
     *
     * When this cube is connected, this MotionBuffer will
     * begin asynchronously receiving a sample every time we receive
     * accelerometer data from the cube over the air.
     *
     * We will store data as fast as it arrives, but due to power
     * management the system will often try to reduce the rate
     * at which we poll each cube's sensors. When a MotionBuffer
     * is attached to a cube, we can request a particular rate of
     * service, specified here in hertz. This is not a guranteed
     * rate, just a suggestion for power management purposes.
     *
     * The caller is responsible for ensuring that a MotionBuffer is
     * only attached to one cube at a time.
     */
    void attach(_SYSCubeID id, unsigned hz=100)
    {
        STATIC_ASSERT(id < _SYS_NUM_CUBE_SLOTS);
        STATIC_ASSERT(tSize <= _SYS_MOTION_MAX_ENTRIES);
        bzero(sys);
        sys.header.last = tSize - 1;
        sys.header.rate = TICK_HZ / hz;
        _SYS_setMotionBuffer(id, *this);
    }

    /**
     * @brief Calculate a numerical integral over recent motion data
     *
     * The duration is specified in units as defined by TICK_NS, TICK_US, and TICK_HZ.
     * Numerical integration is performed by the Trapezoid rule, using a non-uniform
     * sampling grid. If the integral duration extends beyond the oldest buffered,
     * sample, we act as if that sample lasted forever. If the integral ends in-between
     * samples, we linearly interpolate.
     *
     * The result will be scaled by (2 * duration), meaning you'll need to divide
     * by this number to take an average. However, avoiding this division is both faster
     * and preserves precision, so we recommend using the scaled values as-is if possible.
     *
     * This integration can be used as a very simple "boxcar" or moving-average
     * low pass filter, with a fixed temporal window size. This kind of filter will
     * feel like it's "slowing" or "blurring" motions, but for short durations the
     * filtered results can feel very natural.
     */
    Int3 integrate(unsigned duration)
    {
        _SYSInt3 result;
        _SYS_motion_integrate(*this, duration, &result);
        return vec(result.x, result.y, result.z);
    }
};


/**
 * @brief Calculate median, minimum, and maximum statistics from a MotionBuffer
 *
 * This structure contains the results of a median
 * calculation, including the median vector itself as well as min/max
 * stats that we get "for free" as a result of the median calculation.
 */

class MotionMedian {
public:
    _SYSMotionMedian sys;

    // Implicit conversions
    operator _SYSMotionMedian* () { return &sys; }
    operator const _SYSMotionMedian* () const { return &sys; }

    /**
     * @brief Calculate the component-wise median of recent motion data
     *
     * The duration is specified in units as defined by TICK_NS, TICK_US, and TICK_HZ.
     * The median accelerometer sample over this duration is calculated and returned.
     * Medians are calculated independently for each of the three channels. No
     * scaling is applied.
     *
     * The result is written to the supplied MotionMedian buffer. This includes
     * the median itself, as well as the maximum and minimum values, which we
     * get 'for free' as a result of the median calculation.
     *
     * Unlike the moving-average filter that can be built with integrate(), this
     * algorithm provides a way of filtering data which is very good at eliminating
     * brief spikes but preserving hard edges. The 'duration' parameter will set
     * the latency at which we detect these edges. Spikes up to half this duration
     * can be completely eliminated from the output, unlike in a typical low-pass
     * filter where such spikes just end up 'smeared' into your data.
     */
    MotionMedian(_SYSMotionBuffer *mbuf, unsigned duration)
    {
        _SYS_motion_median(mbuf, duration, *this);
    }

    /// Return the median itself, as a vector
    Byte3 median() const {
        return vec(sys.axes[0].median, sys.axes[1].median, sys.axes[2].median);
    }

    /// Return the minimum values for each axis, as a vector
    Byte3 minimum() const {
        return vec(sys.axes[0].minimum, sys.axes[1].minimum, sys.axes[2].minimum);
    }

    /// Return the maximum values for each axis, as a vector
    Byte3 maximum() const {
        return vec(sys.axes[0].maximum, sys.axes[1].maximum, sys.axes[2].maximum);
    }

    /// Return the difference between maximum and minimum, as a vector
    Int3 range() const {
        return Int3(maximum()) - Int3(minimum());
    }
};


/**
 * @} endgroup motion
*/

};  // namespace Sifteo
