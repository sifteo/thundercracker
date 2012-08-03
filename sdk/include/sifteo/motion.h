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
    void calculate(_SYSMotionBuffer *mbuf, unsigned duration) {
        _SYS_motion_median(mbuf, duration, *this);
    }

    /// Construct an uninitialized MotionMedian
    MotionMedian() {}

    /// Construct a MotionMedian with data calculated from the supplied MotionBuffer
    MotionMedian(_SYSMotionBuffer *mbuf, unsigned duration) {
        calculate(mbuf, duration);
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
 * @brief A standard recognizer for shake and tilt gestures
 *
 * This class contains a MotionBuffer as well as other state which is necessary
 * to detect the standard tilt and shake gestures. This filter can be updated
 * as a result of an Events::cubeAccelChange event, or once per frame, or even
 * less often. We provide median statistics, as well as simple binary tilt and
 * shake outputs with hysteresis.
 */

class TiltShakeRecognizer {
public:
    static const int kFilterLatency = MotionBuffer<>::TICK_HZ / 10;
    static const int kTiltThresholdMin = 15;
    static const int kTiltThresholdMax = 30;
    static const int kShakeThresholdMin = 1000;
    static const int kShakeThresholdMax = 50000;

    /**
     * @brief The MotionBuffer used by this TiltShakeRecognizer
     *
     * This is part of the TiltShakeRecognizer class in order to make the
     * common use case convenient, but it may also be used directly if your
     * application must process raw motion data as well as these standard
     * gestures.
     */
    MotionBuffer<> buffer;

    /**
     * @brief The most recent median data calculated by the TiltShakeRecognizer
     *
     * This contains the results of the intermediate median, minimum, and
     * maximum statistics as calculated during update(). If your application
     * needs to perform additional processing on the motion data, these results
     * are available at no extra cost.
     */
    MotionMedian median;

    /**
     * @brief The most recent tilt value.
     *
     * Each axis is -1, 0, or +1. The coordinate system is identical to
     * that used by CubeID::accel().
     */
    Byte3 tilt;

    /// The most recent binary shake state.
    bool shake;

    /**
     * @brief Initialize this TiltShakeRecognizer and attach it to a cube.
     *
     * This invokes MotionBuffer::attach(), and resets the state 
     * of the TiltShakeRecognizer itself.
     */
    void attach(_SYSCubeID id)
    {
        buffer.attach(id);
        bzero(median);
        bzero(tilt);
        shake = false;
    }

    /**
     * @brief Return the physical tilt reading for the attached cube.
     *
     * The resulting vector is oriented with respect to the cube hardware.
     * The underlying tilt state is only updated after an explicit call
     * to update().
     */
    Byte3 physicalTilt() const {
        return tilt;
    }

    /**
     * @brief Return the virtual tilt reading for the attached cube.
     *
     * The resulting vector is oriented with respect to the current
     * LCD rotation. The underlying tilt state is only updated after
     * an explicit call to update().
     *
     * Requires the orientation of the attached cube, which can
     * be obtained via VideoBuffer::orientation().
     */
    Byte3 virtualTilt(Side orientation) const {
        return tilt.zRotateI(orientation);
    }

    /**
     * @brief Change flags, returned by update() to indicate what just changed
     */
    enum ChangeFlags {
        Shake_Begin      = 1 << 0,
        Shake_End        = 1 << 1,
        Shake_Change     = Shake_Begin | Shake_End,

        Tilt_XNeg        = 1 << 2,
        Tilt_XZero       = 1 << 3,
        Tilt_XPos        = 1 << 4,
        Tilt_XChange     = Tilt_XNeg | Tilt_XZero | Tilt_XPos,

        Tilt_YNeg        = 1 << 5,
        Tilt_YZero       = 1 << 6,
        Tilt_YPos        = 1 << 7,
        Tilt_YChange     = Tilt_YNeg | Tilt_YZero | Tilt_YPos,

        Tilt_ZNeg        = 1 << 8,
        Tilt_ZZero       = 1 << 9,
        Tilt_ZPos        = 1 << 10,
        Tilt_ZChange     = Tilt_ZNeg | Tilt_ZZero | Tilt_ZPos,

        Tilt_Change      = Tilt_XChange | Tilt_YChange | Tilt_ZChange,
    };

    /**
     * @brief Update the state of the TiltShakeRecognizer
     *
     * Using data captured in our MotionBuffer, this updates the state of
     * the `median` filter, and calculates a new tilt and shake state for
     * the attached cube. After this call, the `tilt` and `shake` members
     * will contain the latest state.
     *
     * Returns a bitmap of ChangeFlags which describe which changes just occurred.
     */
    unsigned update()
    {
        unsigned changed = 0;

        median.calculate(buffer, kFilterLatency);
        auto m = median.median();
        int wobble = median.range().len2();

        // Shake hysteresis
        if (wobble >= kShakeThresholdMax) {
            if (!shake) changed |= Shake_Begin;
            shake = true;

        } else if (wobble < kShakeThresholdMin) {
            if (shake) changed |= Shake_End;
            shake = false;

            // Only update tilt state when wobble is low.
            // Each tilt axis has hysteresis.

            if (m.x <= -kTiltThresholdMax) {
                if (tilt.x != -1) changed |= Tilt_XNeg;
                tilt.x = -1;
            } else if (m.x >= kTiltThresholdMax) {
                if (tilt.x != 1) changed |= Tilt_XPos;
                tilt.x = 1;
            } else if (abs(m.x) < kTiltThresholdMin) {
                if (tilt.x != 0) changed |= Tilt_XZero;
                tilt.x = 0;
            }

            if (m.y <= -kTiltThresholdMax) {
                if (tilt.y != -1) changed |= Tilt_YNeg;
                tilt.y = -1;
            } else if (m.y >= kTiltThresholdMax) {
                if (tilt.y != 1) changed |= Tilt_YPos;
                tilt.y = 1;
            } else if (abs(m.y) < kTiltThresholdMin) {
                if (tilt.y != 0) changed |= Tilt_YZero;
                tilt.y = 0;
            }

            if (m.z <= -kTiltThresholdMax) {
                if (tilt.z != -1) changed |= Tilt_ZNeg;
                tilt.z = -1;
            } else if (m.z >= kTiltThresholdMax) {
                if (tilt.z != 1) changed |= Tilt_ZPos;
                tilt.z = 1;
            } else if (abs(m.z) < kTiltThresholdMin) {
                if (tilt.z != 0) changed |= Tilt_ZZero;
                tilt.z = 0;
            }
        }

        return changed;
    }
};


/**
 * @} endgroup motion
*/

};  // namespace Sifteo
