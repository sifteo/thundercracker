/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * This file is part of the public interface to the Sifteo SDK.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#ifndef _SIFTEO_MATH_H
#define _SIFTEO_MATH_H

#ifdef NO_USERSPACE_HEADERS
#   error This is a userspace-only header, not allowed by the current build.
#endif

#include <sifteo/abi.h>

namespace Sifteo {


/**
 * Common mathematical constants
 */
 
#define M_E         2.71828182845904523536028747135266250   // e
#define M_LOG2E     1.44269504088896340735992468100189214   // log 2e
#define M_LOG10E    0.434294481903251827651128918916605082  // log 10e
#define M_LN2       0.693147180559945309417232121458176568  // log e2
#define M_LN10      2.30258509299404568401799145468436421   // log e10
#define M_PI        3.14159265358979323846264338327950288   // pi
#define M_TAU       6.2831853071795862                      // 2 * pi
#define M_PI_2      1.57079632679489661923132169163975144   // pi/2
#define M_PI_4      0.785398163397448309615660845819875721  // pi/4
#define M_1_PI      0.318309886183790671537767526745028724  // 1/pi
#define M_2_PI      0.636619772367581343075535053490057448  // 2/pi
#define M_2_SQRTPI  1.12837916709551257389615890312154517   // 2/sqrt(pi)
#define M_SQRT2     1.41421356237309504880168872420969808   // sqrt(2)
#define M_SQRT1_2   0.707106781186547524400844362104849039  // 1/sqrt(2)
#define MAXFLOAT    ((float)3.40282346638528860e+38)


/**
 * For any type, clamp a value to the extremes 'low' and 'high'. If the
 * value is less than 'low' we return 'low', and if it's greater than 'high'
 * we return 'high'. Otherwise we return the value unmodified.
 */

template <typename T> inline T clamp(const T& value, const T& low, const T& high)
{
    if (value < low) {
        return low;
    }
    if (value > high) {
        return high;
    }
    return value;
}

/**
 * For any type, return the absolute value. If the value is less than zero,
 * we return -value. Otherwise, we return the unmodified value.
 */

template <typename T> inline T abs(const T& value)
{
    if (value < 0) {
        return -value;
    }
    return value;
}

/**
 * Logical shift left with clamping. If the shift amount is negative,
 * it is treated as zero. Shift amounts greater than or equal to the word
 * width will always return zero.
 */

template <typename T> inline T lslc(const T& value, int bits)
{
    if (bits < 0)
        return value;
    if (bits >= sizeof(T) * 8)
        return 0;
    return value << (unsigned)bits;
}

/**
 * Logical shift right with clamping. If the shift amount is negative,
 * it is treated as zero. Shift amounts greater than or equal to the word
 * width will always return zero.
 */

template <typename T> inline T lsrc(const T& value, int bits)
{
    if (bits < 0)
        return value;
    if (bits >= sizeof(T) * 8)
        return 0;
    return value >> (unsigned)bits;
}

/**
 * Return a value of type T which has bits set in the half-open
 * interval [begin, end). The range may include negative values
 * and/or values greater than the width of the type.
 */

template <typename T> inline T bitRange(int begin, int end)
{
    return lslc((T)-1, begin) & ~lslc((T)-1, end);
}

/**
 * Compute the remainder (modulo) operation for two floating point numbers.
 * This variant operates on single-precision floats.
 */

float inline fmod(float a, float b)
{
    uint32_t r = _SYS_fmodf(reinterpret_cast<uint32_t&>(a), reinterpret_cast<uint32_t&>(b));
    return reinterpret_cast<float&>(r);
}

/**
 * Compute the remainder (modulo) operation for two floating point numbers.
 * This variant operates on double-precision floats.
 */

double inline fmod(double a, double b)
{
    uint64_t ia = reinterpret_cast<uint64_t&>(a);
    uint64_t ib = reinterpret_cast<uint64_t&>(b);
    uint64_t r = _SYS_fmod(ia, ia >> 32, ib, ib >> 32);
    return reinterpret_cast<double&>(r);
}

/**
 * Compute the square root of a floating point number.
 * This variant operates on single-precision floats.
 */

float inline sqrt(float a)
{
    uint32_t r = _SYS_sqrtf(reinterpret_cast<uint32_t&>(a));
    return reinterpret_cast<float&>(r);
}

/**
 * Compute the square root of a floating point number.
 * This variant operates on double-precision floats.
 */

double inline sqrt(double a)
{
    uint64_t ia = reinterpret_cast<uint64_t&>(a);
    uint64_t r = _SYS_sqrt(ia, ia >> 32);
    return reinterpret_cast<double&>(r);
}

/**
 * Simultaneously compute the sine and cosine of a specified angle,
 * in radians. This yields two single-precision floating point results,
 * returned via the pointers 's' and 'c'.
 */

void inline sincos(float x, float *s, float *c)
{
    _SYS_sincosf(reinterpret_cast<uint32_t&>(x), s, c);
}

/**
 * Calculate the sine of a specified angle, in radians.
 * Returns a single-precision floating point result.
 */

float inline sin(float x)
{
    float s;
    sincos(x, &s, 0);
    return s;
}

/**
 * Calculate the cosine of a specified angle, in radians.
 * Returns a single-precision floating point result.
 */

float inline cos(float x)
{
    float c;
    sincos(x, 0, &c);
    return c;
}


/**
 * Generalized two-element cartesian coordinate vector.
 */

template <typename T> struct Vector2 {
    T x, y;

    /**
     * Modify this vector's value in-place.
     */
    void set(T _x, T _y) {
        x = _x;
        y = _y;
    }

    /**
     * Set this vector to a new cartesian value, given a value in polar coordinates.
     */
    void setPolar(float angle, float magnitude) {
        float s, c;
        sincos(angle, &s, &c);
        x = c * magnitude;
        y = s * magnitude;
    }

    /**
     * Rotate this vector about the origin counterclockwise by 'angle' radians.
     */
    Vector2<T> rotate(float angle) const {
        float s, c;
        sincos(angle, &s, &c);
        Vector2<T> result = { x*c - y*s, x*s + y*c };
        return result;
    }

    /**
     * Calculate the scalar length (magnitude) of this vector, squared.
     * This avoids the costly square root calculation.
     */
    T len2() const {
        return ( x * x + y * y );
    }

    /**
     * Calculate the scalar length (magnitude) of this vector.
     */
    T len() const {
        return sqrt(len2());
    }

    /**
     * Return a normalized version of this vector.
     * The returned vector will have a magnitude of 1.0.
     */
    Vector2<T> normalize() const {
        return *this / len();
    }
    
    /**
     * Round a floating point vector to the nearest integer.
     */
    Vector2<int> round() const {
        Vector2<int> result = { x + 0.5f, y + 0.5f };
        return result;
    }

    /**
     * Explicitly cast this vector to another vector type, using
     * default C++ truncation or extension rules.
     */
    template <typename R> Vector2<R> cast() const {
        Vector2<R> result = { x, y };
        return result;
    }
    
    /// Shortcuts for common explicit casts
    Vector2<int> toInt() const { return cast<int>(); }
    Vector2<float> toFloat() const { return cast<float>(); }
    Vector2<double> toDouble() const { return cast<double>(); }
    
    /// Implicit casts
    operator Vector2<int>            () const { return cast<int>(); }
    operator Vector2<unsigned>       () const { return cast<unsigned>(); }
    operator Vector2<short>          () const { return cast<short>(); }
    operator Vector2<unsigned short> () const { return cast<unsigned short>(); }
    operator Vector2<int8_t>         () const { return cast<int8_t>(); }
    operator Vector2<uint8_t>        () const { return cast<uint8_t>(); }
    operator Vector2<float>          () const { return cast<float>(); }
    operator Vector2<double>         () const { return cast<double>(); }
};

typedef Vector2<int>                Int2;
typedef Vector2<unsigned>           UInt2;
typedef Vector2<short>              Short2;
typedef Vector2<unsigned short>     UShort2;
typedef Vector2<int8_t>             Byte2;
typedef Vector2<uint8_t>            UByte2;
typedef Vector2<float>              Float2;
typedef Vector2<double>             Double2;

/**
 * Create a Vector2, from a set of (x,y) coordinates.
 * This is a standalone function, instead of a constructor,
 * so that Vector2 can remain a POD type, and it can be used
 * in unions.
 */
 
template <typename T> inline Vector2<T> Vec2(T x, T y) {
    Vector2<T> result = { x, y };
    return result;
}

template <typename T> inline T dot(Vector2<T> u, Vector2<T> v) {
    return u.x * v.x + u.y * v.y;
}

template <typename T> inline Vector2<T> polar(T angle, T magnitude) {
    Vector2<T> result;
    result.setPolar(angle, magnitude);
    return result;
}

// Vector operations
template <typename T> inline Vector2<T> operator-(Vector2<T> u) { return Vec2<T>(-u.x, -u.y); }
template <typename T> inline Vector2<T> operator+=(Vector2<T> &u, Vector2<T> v) { return Vec2<T>(u.x+=v.x, u.y+=v.y); }
template <typename T> inline Vector2<T> operator-=(Vector2<T> &u, Vector2<T> v) { return Vec2<T>(u.x-=v.x, u.y-=v.y); }
template <typename T> inline Vector2<T> operator+(Vector2<T> u, Vector2<T> v) { return Vec2<T>(u.x+v.x, u.y+v.y); }
template <typename T> inline Vector2<T> operator-(Vector2<T> u, Vector2<T> v) { return Vec2<T>(u.x-v.x, u.y-v.y); }
template <typename T> inline Vector2<T> operator*(Vector2<T> u, Vector2<T> v) { return Vec2<T>(u.x*v.x, u.y*v.y); }
template <typename T> inline bool operator==(Vector2<T> u, Vector2<T> v) { return u.x == v.x && u.y == v.y; }
template <typename T> inline bool operator!=(Vector2<T> u, Vector2<T> v) { return u.x != v.x || u.y != v.y; }

// Scalar int promotion
template <typename T> inline Int2 operator*(int k, Vector2<T> v) { return Vec2<int>(k*v.x, k*v.y); }
template <typename T> inline Int2 operator*(Vector2<T> v, int k) { return Vec2<int>(k*v.x, k*v.y); }
template <typename T> inline Int2 operator/(Vector2<T> v, int k) { return Vec2<int>(v.x/k, v.y/k); }
template <typename T> inline Int2 operator+=(Vector2<T> &u, int k) { return Vec2<int>(u.x+=k, u.y+=k); }
template <typename T> inline Int2 operator*=(Vector2<T> &u, int k) { return Vec2<int>(u.x*=k, u.y*=k); }
template <typename T> inline Int2 operator<<(Vector2<T> u, int shift) { return Vec2<int>(u.x<<shift, u.y<<shift); }
template <typename T> inline Int2 operator>>(Vector2<T> u, int shift) { return Vec2<int>(u.x>>shift, u.y>>shift); }

// Scalar float promotion
template <typename T> inline Float2 operator*(float k, Vector2<T> v) { return Vec2<float>(k*v.x, k*v.y); }
template <typename T> inline Float2 operator*(Vector2<T> v, float k) { return Vec2<float>(k*v.x, k*v.y); }
template <typename T> inline Float2 operator/(Vector2<T> v, float k) { return Vec2<float>(v.x/k, v.y/k); }
template <typename T> inline Float2 operator+=(Vector2<T> &u, float k) { return Vec2<float>(u.x+=k, u.y+=k); }
template <typename T> inline Float2 operator*=(Vector2<T> &u, float k) { return Vec2<float>(u.x*=k, u.y*=k); }

// Scalar double promotion
template <typename T> inline Double2 operator*(double k, Vector2<T> v) { return Vec2<double>(k*v.x, k*v.y); }
template <typename T> inline Double2 operator*(Vector2<T> v, double k) { return Vec2<double>(k*v.x, k*v.y); }
template <typename T> inline Double2 operator/(Vector2<T> v, double k) { return Vec2<double>(v.x/k, v.y/k); }
template <typename T> inline Double2 operator+=(Vector2<T> &u, double k) { return Vec2<double>(u.x+=k, u.y+=k); }
template <typename T> inline Double2 operator*=(Vector2<T> &u, double k) { return Vec2<double>(u.x*=k, u.y*=k); }


/**
 * Generalized three-element cartesian coordinate vector.
 */

template <typename T> struct Vector3 {
    T x, y, z;

    /**
     * Modify this vector's value in-place.
     */
    void set(T _x, T _y, T _z) {
        x = _x;
        y = _y;
        z = _z;
    }
    
    /**
     * Extract a 2-vector with just the named components.
     */

    Vector2<T> xy() const { return Vec2(x, y); }
    Vector2<T> xz() const { return Vec2(x, z); }
    Vector2<T> yx() const { return Vec2(y, x); }
    Vector2<T> yz() const { return Vec2(y, z); }
    Vector2<T> zx() const { return Vec2(z, x); }
    Vector2<T> zy() const { return Vec2(z, y); }

    /**
     * Calculate the scalar length (magnitude) of this vector, squared.
     * This avoids the costly square root calculation.
     */
    T len2() const {
        return ( x * x + y * y + z * z );
    }

    /**
     * Calculate the scalar length (magnitude) of this vector.
     */
    T len() const {
        return sqrt(len2());
    }

    /**
     * Return a normalized version of this vector.
     * The returned vector will have a magnitude of 1.0.
     */
    T normalize() const {
        return *this / len();
    }

    /**
     * Round a floating point vector to the nearest integer.
     */
    Vector3<int> round() const {
        Vector3<int> result = { x + 0.5f, y + 0.5f, z + 0.5f };
        return result;
    }

    /**
     * Explicitly cast this vector to another vector type, using
     * default C++ truncation or extension rules.
     */
    template <typename R> Vector3<R> cast() const {
        Vector3<R> result = { x, y };
        return result;
    }
    
    /// Shortcuts for common explicit casts
    Vector3<int> toInt() const { return cast<int>(); }
    Vector3<float> toFloat() const { return cast<float>(); }
    Vector3<double> toDouble() const { return cast<double>(); }
    
    /// Implicit casts
    operator Vector3<int>            () const { return cast<int>(); }
    operator Vector3<unsigned>       () const { return cast<unsigned>(); }
    operator Vector3<short>          () const { return cast<short>(); }
    operator Vector3<unsigned short> () const { return cast<unsigned short>(); }
    operator Vector3<int8_t>         () const { return cast<int8_t>(); }
    operator Vector3<uint8_t>        () const { return cast<uint8_t>(); }
    operator Vector3<float>          () const { return cast<float>(); }
    operator Vector3<double>         () const { return cast<double>(); }
};

typedef Vector3<int>                Int3;
typedef Vector3<unsigned>           UInt3;
typedef Vector3<short>              Short3;
typedef Vector3<unsigned short>     UShort3;
typedef Vector3<int8_t>             Byte3;
typedef Vector3<uint8_t>            UByte3;
typedef Vector3<float>              Float3;
typedef Vector3<double>             Double3;

/**
 * Create a Vector3, from a set of (x, y, z) coordinates.
 * This is a standalone function, instead of a constructor,
 * so that Vector3 can remain a POD type, and it can be used
 * in unions.
 */
 
template <typename T> inline Vector3<T> Vec3(T x, T y, T z) {
    Vector3<T> result = { x, y, z };
    return result;
}

// Vector operations
template <typename T> inline Vector3<T> operator-(Vector3<T> u) { return Vec3<T>(-u.x, -u.y, -u.z); }
template <typename T> inline Vector3<T> operator+=(Vector3<T> &u, Vector3<T> v) { return Vec3<T>(u.x+=v.x, u.y+=v.y, u.z+=v.z); }
template <typename T> inline Vector3<T> operator-=(Vector3<T> &u, Vector3<T> v) { return Vec3<T>(u.x-=v.x, u.y-=v.y, u.z-=v.z); }
template <typename T> inline Vector3<T> operator+(Vector3<T> u, Vector3<T> v) { return Vec3<T>(u.x+v.x, u.y+v.y, u.z+v.z); }
template <typename T> inline Vector3<T> operator-(Vector3<T> u, Vector3<T> v) { return Vec3<T>(u.x-v.x, u.y-v.y, u.z-v.z); }
template <typename T> inline Vector3<T> operator*(Vector3<T> u, Vector3<T> v) { return Vec3<T>(u.x*v.x, u.y*v.y, u.z*v.z); }
template <typename T> inline bool operator==(Vector3<T> u, Vector3<T> v) { return u.x == v.x && u.y == v.y && u.z == v.z; }
template <typename T> inline bool operator!=(Vector3<T> u, Vector3<T> v) { return u.x != v.x || u.y != v.y || u.z != v.z; }

// Scalar int promotion
template <typename T> inline Int3 operator*(int k, Vector3<T> v) { return Vec3<int>(k*v.x, k*v.y, k*v.z); }
template <typename T> inline Int3 operator*(Vector3<T> v, int k) { return Vec3<int>(k*v.x, k*v.y, k*v.z); }
template <typename T> inline Int3 operator/(Vector3<T> v, int k) { return Vec3<int>(v.x/k, v.y/k, v.z/k); }
template <typename T> inline Int3 operator+=(Vector3<T> &u, int k) { return Vec3<int>(u.x+=k, u.y+=k, u.z+=k); }
template <typename T> inline Int3 operator*=(Vector3<T> &u, int k) { return Vec3<int>(u.x*=k, u.y*=k, u.z*=k); }
template <typename T> inline Int3 operator<<(Vector3<T> u, int shift) { return Vec3<int>(u.x<<shift, u.y<<shift, u.z<<shift); }
template <typename T> inline Int3 operator>>(Vector3<T> u, int shift) { return Vec3<int>(u.x>>shift, u.y>>shift, u.z>>shift); }

// Scalar float promotion
template <typename T> inline Float3 operator*(float k, Vector3<T> v) { return Vec3<float>(k*v.x, k*v.y, k*v.z); }
template <typename T> inline Float3 operator*(Vector3<T> v, float k) { return Vec3<float>(k*v.x, k*v.y, k*v.z); }
template <typename T> inline Float3 operator/(Vector3<T> v, float k) { return Vec3<float>(v.x/k, v.y/k, v.z/k); }
template <typename T> inline Float3 operator+=(Vector3<T> &u, float k) { return Vec3<float>(u.x+=k, u.y+=k, u.z+=k); }
template <typename T> inline Float3 operator*=(Vector3<T> &u, float k) { return Vec3<float>(u.x*=k, u.y*=k, u.z*=k); }

// Scalar double promotion
template <typename T> inline Double3 operator*(double k, Vector3<T> v) { return Vec3<double>(k*v.x, k*v.y, k*v.z); }
template <typename T> inline Double3 operator*(Vector3<T> v, double k) { return Vec3<double>(k*v.x, k*v.y, k*v.z); }
template <typename T> inline Double3 operator/(Vector3<T> v, double k) { return Vec3<double>(v.x/k, v.y/k, v.z/k); }
template <typename T> inline Double3 operator+=(Vector3<T> &u, double k) { return Vec3<double>(u.x+=k, u.y+=k, u.z+=k); }
template <typename T> inline Double3 operator*=(Vector3<T> &u, double k) { return Vec3<double>(u.x*=k, u.y*=k, u.z*=k); }


/**
 * Pseudo-random number generator.
 * Each instance of the Random class has a distinct PRNG state.
 *
 * When possible, method semantics here have been designed to match
 * those used by Python's "random" module.
 */
 
struct Random {
    _SYSPseudoRandomState state;

    /**
     * Construct a new random number generator, using an arbitrary seed.
     */

    Random() {
        seed();
    }
    
    /**
     * Construct a new random number generator with a well-defined seed.
     */
     
    Random(uint32_t s) {
        seed(s);
    }
    
    /**
     * Re-seed this random number generator. For a given seed, the subsequent
     * random numbers are guaranteed to be deterministic.
     */
    
    void seed(uint32_t s) {
        _SYS_prng_init(&state, s);
    }

    /**
     * Re-seed this random number generator arbitrarily.
     * This implementation uses the system's nanosecond timer.
     */
    
    void seed() {
        seed((uint32_t) _SYS_ticks_ns());
    }

    /**
     * Returns the next raw 32-bit pseudo-random number
     */
    
    uint32_t raw() {
        return _SYS_prng_value(&state);
    }
    
    /**
     * Returns a uniformly distributed floating point number between 0 and 1, inclusive.
     */
    
    float random() {
        return raw() * (1.0f / 0xFFFFFFFF);
    }

    /**
     * Returns a uniformly distributed floating point number in the range [a, b) or
     * [a, b], depending on rounding.
     */
    
    float uniform(float a, float b) {
        // Order of operations here allows constant folding if the endpoints are constant
        return a + raw() * ((b-a) / 0xFFFFFFFF);
    }

    /**
     * Returns a uniformly distributed random integer in the range [a, b], including both
     * end points.
     */
    
    int randint(int a, int b) {
        return a + _SYS_prng_valueBounded(&state, b - a);
    }

    unsigned randint(unsigned a, unsigned b) {
        return a + _SYS_prng_valueBounded(&state, b - a);
    }

    /**
     * Returns a uniformly distributed random integer in the half-open interval [a, b),
     * including the lower but not the upper end point.
     */
    
    int randrange(int a, int b) {
        return randint(a, b - 1);
    }

    unsigned randrange(unsigned a, unsigned b) {
        return randint(a, b - 1);
    }
    
    /**
     * The one-argument variant of randrange() always starts at zero, and
     * returns an integer up to but not including 'count'. It is guaranteed
     * to be capable of returning 'count' distinct values, starting at zero.
     */

    int randrange(int count) {
        return randrange(0, count);
    }

    unsigned randrange(unsigned count) {
        return randrange((unsigned)0, count);
    }

};


/**
 * An augmented 3x2 matrix, for doing 2D affine transforms.
 *
 *  [ xx  yx  cx ]
 *  [ xy  yy  cy ]
 *  [  0   0   1 ]
 *
 * The way we use affine transforms for background scaling are
 * very similiar to the mechanism used by the GameBoy Advance
 * PPU. There's a great tutorial on this at:
 *
 * http://www.coranac.com/tonc/text/affine.htm
 */

struct AffineMatrix {
    float cx, cy;
    float xx, xy;
    float yx, yy;

    AffineMatrix() {}

    AffineMatrix(float _xx, float _yx, float _cx,
                 float _xy, float _yy, float _cy)
        : cx(_cx), cy(_cy), xx(_xx),
          xy(_xy), yx(_yx), yy(_yy) {}
    
    static AffineMatrix identity() {
        return AffineMatrix(1, 0, 0,
                            0, 1, 0);
    }

    static AffineMatrix scaling(float s) {
        float inv_s = 1.0f / s;
        return AffineMatrix(inv_s, 0, 0,
                            0, inv_s, 0);
    }

    static AffineMatrix translation(float x, float y) {
        return AffineMatrix(1, 0, x,
                            0, 1, y);
    }

    static AffineMatrix translation(Float2 v) {
        return AffineMatrix(1, 0, v.x,
                            0, 1, v.y);
    }

    static AffineMatrix rotation(float angle) {
        float s, c;
        sincos(angle, &s, &c);
        return AffineMatrix(c, -s, 0,
                            s, c, 0);
    }

    void operator*= (const AffineMatrix &m) {
        AffineMatrix n;

        n.cx = xx*m.cx + yx*m.cy + cx;
        n.cy = xy*m.cx + yy*m.cy + cy;
        n.xx = xx*m.xx + yx*m.xy;
        n.xy = xy*m.xx + yy*m.xy;
        n.yx = xx*m.yx + yx*m.yy;
        n.yy = xy*m.yx + yy*m.yy;

        *this = n;
    }

    void translate(float x, float y) {
        *this *= translation(x, y);
    }

    void translate(Float2 v) {
        *this *= translation(v);
    }

    void rotate(float angle) {
        *this *= rotation(angle);
    }
    
    void scale(float s) {
        *this *= scaling(s);
    }
};


}   // namespace Sifteo

#endif
