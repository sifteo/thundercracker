/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * This file is part of the public interface to the Sifteo SDK.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#pragma once
#ifdef NOT_USERSPACE
#   error This is a userspace-only header, not allowed by the current build.
#endif

#include <sifteo/macros.h>
#include <sifteo/abi.h>

namespace Sifteo {

class SystemTime;


/**
 * @defgroup time Time
 *
 * @brief Timekeeping objects
 *
 * @{
 */

/**
 * @brief Represents a difference between two SystemTimes, with moderate resolution.
 *
 * Deltas are internally stored as 32-bit millisecond counts, but they can be
 * converted to many other units.
 *
 * TimeDeltas can be directly compared to floating point values efficiently.
 * In cases where the comparison is made against a constant, we can convert the
 * entire operation to integer math at compile-time.
 */

class TimeDelta {
public:

    /**
     * @brief Construct a new TimeDelta from a floating point time, in seconds.
     *
     * These constructors also provide an implicit conversion that makes
     * tests like "delta < 1.5" work.
     */
    TimeDelta(float sec) : mMilli(sec * 1e3) {}
    TimeDelta(double sec) : mMilli(sec * 1e3) {}

    /**
     * @brief Construct a TimeDelta representing the period which corresponds
     * with a given frequency.
     *
     * This can be used, for example, with frames() in order to specify
     * the frame rate in HZ (FPS) rather than in seconds.
     */
    static TimeDelta hz(float h) {
        return TimeDelta(1.0f / h);
    }

    /**
     * @brief Construct a new TimeDelta from an integer time, in milliseconds.
     */
    static TimeDelta fromMillisec(int32_t m) {
        return TimeDelta(m);
    }

    /**
     * @brief Return the delta in milliseconds.
     */
    int32_t milliseconds() const {
        return mMilli;
    }

    /**
     * @brief Return the delta in nanoseconds.
     */
    int64_t nanoseconds() const {
        return mMilli * (int64_t)1000000;
    }

    /**
     * @brief Return the delta in seconds, as a floating point value.
     */
    float seconds() const {
        return mMilli * 1e-3;
    }
    
    /**
     * @brief Return the number of frames, of a particular duration, that
     * are represented by this time delta, rounding down.
     *
     * This is integer division with truncation.
     *
     * You can use this to implement animations which start at a
     * particular reference point. At each frame, look at the current
     * time, subtract the reference SystemTime to yield a TimeDelta,
     * then use frames() to convert that value to an integer frame
     * count.
     */

    unsigned frames(TimeDelta duration) const {
        ASSERT(!isNegative());
        ASSERT(duration.isPositive());
        return mMilli / duration.mMilli;
    }

    /**
     * @brief Return a frame count, and subtract (pull) the time corresponding
     * with those frames from this TimeDelta.
     *
     * This leaves the remainder, i.e. the time since the beginning of
     * the current frame, in this Time Delta.
     *
     * This can be used as an alternative to frames() in
     * cases where time is being accumulated frame-by-frame instead of
     * being calculated based on a fixed reference point. This is also
     * very handy for animating at a fixed rate or running a fixed timestep
     * physics simulation, while saving the remainders for later.
     */

    unsigned pullFrames(TimeDelta duration) {
        unsigned f = frames(duration);
        // Could be "*this -= duration * f", but operator* isn't defined yet.
        mMilli -= duration.mMilli * f;
        return f;
    } 
    
    /**
     * @brief Is this time value negative?
     *
     * Time deltas are signed, but in many cases it makes sense to reject
     * negative deltas. This predicate can be used quickly in ASSERTs.
     */
    bool isNegative() const {
        return mMilli < 0;
    }
    
    /// Is this time value positive?
    bool isPositive() const {
        return mMilli > 0;
    }

    /// Explicit conversion to float
    operator float() const { return seconds(); }
    /// Explicit converstion to double
    operator double() const { return seconds(); }
    // Accumulate time from another TimeDelta
    TimeDelta operator+= (TimeDelta b) { mMilli += b.mMilli; return *this; }
    // Accumulate negative time from another TimeDelta
    TimeDelta operator-= (TimeDelta b) { mMilli -= b.mMilli; return *this; }

private:
    friend class SystemTime;

    TimeDelta(int32_t m) : mMilli(m) {}
    int32_t mMilli;
};

inline bool operator== (TimeDelta a, TimeDelta b) { return a.milliseconds() == b.milliseconds(); }
inline bool operator!= (TimeDelta a, TimeDelta b) { return a.milliseconds() != b.milliseconds(); }
inline bool operator<  (TimeDelta a, TimeDelta b) { return a.milliseconds() <  b.milliseconds(); }
inline bool operator>  (TimeDelta a, TimeDelta b) { return a.milliseconds() >  b.milliseconds(); }
inline bool operator<= (TimeDelta a, TimeDelta b) { return a.milliseconds() <= b.milliseconds(); }
inline bool operator>= (TimeDelta a, TimeDelta b) { return a.milliseconds() >= b.milliseconds(); }

inline bool operator== (TimeDelta a, float b) { return a.milliseconds() == TimeDelta(b).milliseconds(); }
inline bool operator!= (TimeDelta a, float b) { return a.milliseconds() != TimeDelta(b).milliseconds(); }
inline bool operator<  (TimeDelta a, float b) { return a.milliseconds() <  TimeDelta(b).milliseconds(); }
inline bool operator>  (TimeDelta a, float b) { return a.milliseconds() >  TimeDelta(b).milliseconds(); }
inline bool operator<= (TimeDelta a, float b) { return a.milliseconds() <= TimeDelta(b).milliseconds(); }
inline bool operator>= (TimeDelta a, float b) { return a.milliseconds() >= TimeDelta(b).milliseconds(); }

inline bool operator== (float a, TimeDelta b) { return TimeDelta(a).milliseconds() == b.milliseconds(); }
inline bool operator!= (float a, TimeDelta b) { return TimeDelta(a).milliseconds() != b.milliseconds(); }
inline bool operator<  (float a, TimeDelta b) { return TimeDelta(a).milliseconds() <  b.milliseconds(); }
inline bool operator>  (float a, TimeDelta b) { return TimeDelta(a).milliseconds() >  b.milliseconds(); }
inline bool operator<= (float a, TimeDelta b) { return TimeDelta(a).milliseconds() <= b.milliseconds(); }
inline bool operator>= (float a, TimeDelta b) { return TimeDelta(a).milliseconds() >= b.milliseconds(); }

inline bool operator== (TimeDelta a, double b) { return a.milliseconds() == TimeDelta(b).milliseconds(); }
inline bool operator!= (TimeDelta a, double b) { return a.milliseconds() != TimeDelta(b).milliseconds(); }
inline bool operator<  (TimeDelta a, double b) { return a.milliseconds() <  TimeDelta(b).milliseconds(); }
inline bool operator>  (TimeDelta a, double b) { return a.milliseconds() >  TimeDelta(b).milliseconds(); }
inline bool operator<= (TimeDelta a, double b) { return a.milliseconds() <= TimeDelta(b).milliseconds(); }
inline bool operator>= (TimeDelta a, double b) { return a.milliseconds() >= TimeDelta(b).milliseconds(); }

inline bool operator== (double a, TimeDelta b) { return TimeDelta(a).milliseconds() == b.milliseconds(); }
inline bool operator!= (double a, TimeDelta b) { return TimeDelta(a).milliseconds() != b.milliseconds(); }
inline bool operator<  (double a, TimeDelta b) { return TimeDelta(a).milliseconds() <  b.milliseconds(); }
inline bool operator>  (double a, TimeDelta b) { return TimeDelta(a).milliseconds() >  b.milliseconds(); }
inline bool operator<= (double a, TimeDelta b) { return TimeDelta(a).milliseconds() <= b.milliseconds(); }
inline bool operator>= (double a, TimeDelta b) { return TimeDelta(a).milliseconds() >= b.milliseconds(); }

inline TimeDelta operator+ (TimeDelta a, TimeDelta b) { return TimeDelta::fromMillisec(a.milliseconds() + b.milliseconds()); }
inline TimeDelta operator- (TimeDelta a, TimeDelta b) { return TimeDelta::fromMillisec(a.milliseconds() - b.milliseconds()); }

inline TimeDelta operator* (TimeDelta a, float b)    { return TimeDelta::fromMillisec(a.milliseconds() * b); }
inline TimeDelta operator* (TimeDelta a, double b)   { return TimeDelta::fromMillisec(a.milliseconds() * b); }
inline TimeDelta operator* (TimeDelta a, int b)      { return TimeDelta::fromMillisec(a.milliseconds() * b); }
inline TimeDelta operator* (TimeDelta a, unsigned b) { return TimeDelta::fromMillisec(a.milliseconds() * b); }

inline TimeDelta operator* (float a,    TimeDelta b) { return TimeDelta::fromMillisec(b.milliseconds() * a); }
inline TimeDelta operator* (double a,   TimeDelta b) { return TimeDelta::fromMillisec(b.milliseconds() * a); }
inline TimeDelta operator* (int a,      TimeDelta b) { return TimeDelta::fromMillisec(b.milliseconds() * a); }
inline TimeDelta operator* (unsigned a, TimeDelta b) { return TimeDelta::fromMillisec(b.milliseconds() * a); }

inline float operator+= (float &a, TimeDelta b) { return a += b.seconds(); }
inline float operator-= (float &a, TimeDelta b) { return a -= b.seconds(); }


/**
 * @brief Absolute time, measured by the system's monotonically
 * increasing nanosecond timer.
 *
 * This clock is high resolution, guaranteed to never go backwards,
 * and it will not roll over in any reasonable amount of time.
 *
 * This clock will pause when the application is paused by the system,
 * such as when the app falls outside of its registered Metadata::cubeRange()
 * or when it is explicitly paused by the user.
 *
 * SystemTimes are represented internally as a 64-bit count of nanoseconds
 * since system boot. Applications should never rely on any particular absolute
 * time having a particular meaning, and there isn't necessarily a reliable
 * mapping from SystemTime to wall-clock time.
 *
 * SystemTime objects can be compared against each other, and you can
 * subtract two SystemTimes to yield a TimeDelta object, which
 * can be converted to seconds, milliseconds, etc.
 *
 * TimeDelta objects are 32-bit, with millisecond resolution. They are much
 * more efficient to store and to perform arithmetic on than SystemTime.
 */

class SystemTime {
public:
    /**
     * @brief Creates an invalid SystemTime.
     *
     * Using it will result in an ASSERT failure, but this invalid
     * value can be used as a sentinel. You can test for this value with isValid().
     */
    SystemTime() : mTicks(0) {}

    /**
     * @brief Returns a new SystemTime representing the current system clock value.
     */
    static SystemTime now() {
        return SystemTime(_SYS_ticks_ns());
    }
    
    /**
     * @brief Is this SystemTime valid?
     *
     * Returns true if it was returned by now(), false if it was an
     * uninitialized value or a copy of an uninitialized value.
     */
    bool isValid() const {
        return mTicks != 0;
    }

    /**
     * @brief Is this time in the future?
     */
    bool inFuture() const {
        return mTicks > now().mTicks;
    }

    /**
     * @brief Is this time in the past?
     */
    bool inPast() const {
        return mTicks < now().mTicks;
    }

    /**
     * @brief Return the SystemTime as a count of nanoseconds since boot.
     */
    uint64_t uptimeNS() const {
        ASSERT(isValid());
        return mTicks;
    }

    /**
     * @brief Return the SystemTime as a count of microseconds since boot.
     */
    uint64_t uptimeUS() const {
        ASSERT(isValid());
        return mTicks / (uint64_t)1000;
    }

    /**
     * @brief Return the SystemTime as a count of milliseconds since boot.
     */
    uint64_t uptimeMS() const {
        ASSERT(isValid());
        return mTicks / (uint64_t)1000000;
    }

    /**
     * @brief Return the SystemTime, in seconds since boot. Returns a
     * double-precision floating point value.
     */
    double uptime() const {
        ASSERT(isValid());
        return mTicks * 1e-9;
    }

    /**
     * @brief Measure the amount of time since the beginning of a repeating
     * cycle with arbitrary phase and the specified period.
     *
     * This can be used, for example, to implement ambient animations which
     * must repeat at a particular rate but which don't need any particular
     * phase alignment with respect to the rest of the game.
     */

    TimeDelta cycleDelta(TimeDelta period) const {
        ASSERT(period.milliseconds() > 0);
        return TimeDelta((int32_t)(uptimeMS() % (uint64_t)period.milliseconds()));
    }

    /**
     * @brief Like cycleDelta(), but scales the result to the range [0,1], where
     * 0 and 1 represent the beginning and end of the current cycle.
     */

    float cyclePhase(TimeDelta period) const {
        return cycleDelta(period).milliseconds() / (float) period.milliseconds();
    }

    /**
     * @brief Like cycleDelta(), but scales the result to the range [0, frames-1].
     *
     * This can be used directly to compute the frame in an animation that repeats
     * with the specified period.
     */

    unsigned cycleFrame(TimeDelta period, unsigned frames) const {
        return cycleDelta(period).milliseconds() * frames / period.milliseconds();
    }

    /**
     * @brief Subtract two SystemTimes, and return a 32-bit TimeDelta,
     * with millisecond resolution.
     *
     * TimeDeltas are truncated to the neares  millisecond boundary.
     * (Round toward zero)
     */

    TimeDelta operator- (SystemTime b) const {
        int64_t diff = uptimeNS() - b.uptimeNS();
        return TimeDelta((int32_t)(diff / 1000000));
    }

    SystemTime &operator+= (const TimeDelta &rhs) { mTicks += rhs.nanoseconds(); return *this; }
    SystemTime &operator-= (const TimeDelta &rhs) { mTicks -= rhs.nanoseconds(); return *this; }

    SystemTime operator+ (TimeDelta b) const { return SystemTime(mTicks + b.nanoseconds()); }
    SystemTime operator- (TimeDelta b) const { return SystemTime(mTicks - b.nanoseconds()); }

private:
    SystemTime(uint64_t t) : mTicks(t) {}
    uint64_t mTicks;
};

inline bool operator== (SystemTime a, SystemTime b) { return a.uptimeNS() == b.uptimeNS(); }
inline bool operator!= (SystemTime a, SystemTime b) { return a.uptimeNS() != b.uptimeNS(); }
inline bool operator<  (SystemTime a, SystemTime b) { return a.uptimeNS() <  b.uptimeNS(); }
inline bool operator>  (SystemTime a, SystemTime b) { return a.uptimeNS() >  b.uptimeNS(); }
inline bool operator<= (SystemTime a, SystemTime b) { return a.uptimeNS() <= b.uptimeNS(); }
inline bool operator>= (SystemTime a, SystemTime b) { return a.uptimeNS() >= b.uptimeNS(); }

inline SystemTime operator+ (TimeDelta a, SystemTime b) { return b + a; }
inline SystemTime operator- (TimeDelta a, SystemTime b) { return b - a; }


/**
 * @brief TimeStep is a higher-level utility for keeping track of time
 * the duration of game timesteps.
 *
 * At any time, delta() can be used to
 * retrieve the duration of the last timestep, as a TimeDelta object.
 * The next() call ends the current timestep and begins the next one
 * simultaneously, without losing any time in-between.
 *
 * TimeStep is guaranteed not to inclur cumulative rounding errors due
 * to loss of precision when converting SystemTime into TimeDelta.
 */

class TimeStep {
public:
    TimeStep() : mPrevTime(), mDelta(0.0) {}

    /**
     * @brief Retrieve the duration of the last time interval.
     *
     * If less than two calls to next() have elapsed, this returns a zero-length interval.
     */

    TimeDelta delta() const {
        return mDelta;
    }

    /**
     * @brief Retrieve the SystemTime at the end of the interval described by delta().
     *
     * This is the time at the last call to next(), which must have been called at
     * least once.
     */

    SystemTime end() const {
        ASSERT(mPrevTime.isValid());
        return mPrevTime;
    }

    /**
     * @brief Retrieve the SystemTime at the beginning of the interval described by delta().
     *
     * This is the time at the second to last call to next(), which must have been
     * called at least twice.
     */

    SystemTime begin() const {
        ASSERT(mDelta != TimeDelta(0.0));
        ASSERT(mPrevTime.isValid());
        return mPrevTime - mDelta;
    }

    /**
     * @brief Advance to the next time interval.
     *
     * This samples the system clock, as the end of the previous
     * interval and the beginning of the next.
     */
    void next() {
        SystemTime now = SystemTime::now();

        if (mPrevTime.isValid()) {
            // This subtraction loses sub-millisecond precision.
            // We feed that error back into mPrevTime, so that it will
            // be corrected instead of amplified.

            mDelta = now - mPrevTime;
            mPrevTime += mDelta;
            
        } else {
            // Beginning of the very first timestep
            mPrevTime = now;
        }
    }

private:
    SystemTime mPrevTime;
    TimeDelta mDelta;
};


/**
 * @brief TimeTicker is a utility for converting a stream of time deltas into a stream
 * of discrete ticks.
 *
 * It can be used, for example, to advance an animation to
 * the next frame, or to run a fixed-timestep physics simulation.
 *
 * The Ticker is initialized with a particular tick rate, in Hz. You feed in
 * TimeDeltas (from a TimeStep object or any other source), and this object
 * returns the number of discrete steps that have occurred, while saving
 * the remainder for later.
 *
 * This class is implemented using TimeDelta::pullFrames().
 */

class TimeTicker {
public:
    TimeTicker() : mRemainder(0.0f), mPeriod(0.0) {}
    TimeTicker(float hz) : mRemainder(0.0f), mPeriod(TimeDelta::hz(hz)) {}

    void setPeriod(TimeDelta period) {
        mPeriod = period;
    }

    void setRate(float hz) {
        mPeriod = TimeDelta::hz(hz);
    }

    unsigned tick(TimeDelta dt) {
        ASSERT(!dt.isNegative());
        mRemainder += dt;
        return mRemainder.pullFrames(mPeriod);
	}
    
    TimeDelta getPeriod() const {
        return mPeriod;
    }
    
private:
    TimeDelta mRemainder;
    TimeDelta mPeriod;
};

/**
 * @} endgroup Time
*/

}   // namespace Sifteo

