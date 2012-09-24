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
#include <sifteo/macros.h>

namespace Sifteo {

/**
 * @defgroup math Math
 *
 * @brief Floating point and integer math, matrices, vectors
 *
 * @{
 */

/*
 * Common mathematical constants
 */
 
#define M_E         2.71828182845904523536028747135266250   ///< e
#define M_LOG2E     1.44269504088896340735992468100189214   ///< log 2e
#define M_LOG10E    0.434294481903251827651128918916605082  ///< log 10e
#define M_LN2       0.693147180559945309417232121458176568  ///< log e2
#define M_LN10      2.30258509299404568401799145468436421   ///< log e10
#define M_PI        3.14159265358979323846264338327950288   ///< pi
#define M_TAU       6.2831853071795862                      ///< 2 * pi
#define M_PI_2      1.57079632679489661923132169163975144   ///< pi/2
#define M_PI_4      0.785398163397448309615660845819875721  ///< pi/4
#define M_1_PI      0.318309886183790671537767526745028724  ///< 1/pi
#define M_2_PI      0.636619772367581343075535053490057448  ///< 2/pi
#define M_2_SQRTPI  1.12837916709551257389615890312154517   ///< 2/sqrt(pi)
#define M_SQRT2     1.41421356237309504880168872420969808   ///< sqrt(2)
#define M_SQRT1_2   0.707106781186547524400844362104849039  ///< 1/sqrt(2)
#define MAXFLOAT    ((float)3.40282346638528860e+38)        ///< Largest single-precision float value
#define NAN         __builtin_nanf("0x7fc00000")            ///< Not a Number constant

/**
 * @brief For any type, clamp a value to the extremes 'low' and 'high'.
 *
 * If the value is less than 'low' we return 'low', and if it's greater than 'high'
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
 * @brief For any type, return the absolute value.
 *
 * If the value is less than zero, we return -value. Otherwise, we return
 * the unmodified value.
 */

template <typename T> inline T abs(const T& value)
{
    if (value < 0) {
        return -value;
    }
    return value;
}

/**
 * @brief Logical shift left with clamping.
 *
 * If the shift amount is negative,
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
 * @brief Logical shift right with clamping.
 *
 * If the shift amount is negative, it is treated as zero.
 * Shift amounts greater than or equal to the word width will always return zero.
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
 * @brief Return a value of type T which has bits set in the half-open
 * interval [begin, end).
 *
 * The range may include negative values
 * and/or values greater than the width of the type.
 */

template <typename T> inline T bitRange(int begin, int end)
{
    return lslc((T)-1, begin) & ~lslc((T)-1, end);
}

/**
 * @brief Count leading zeroes in a 32-bit word
 *
 * If the parameter is nonzero, returns a number between 0 and 31 indicating
 * how many leading zeroes are present in the parameter, starting at the
 * most significant bit. If the parameter is zero, returns 32.
 *
 * This function is very fast: it is implemented as a single CPU instruction.
 */

inline unsigned clz(uint32_t word)
{
    return __builtin_clz(word);
}

/**
 * @brief Find first bit set in a 32-bit word
 *
 * Finds the first bit set in a word, starting with the least significant bit.
 * If no bits are set (the argument was zero), returns zero. Otherwise,
 * returns a number between 1 (LSB) and 32 (MSB).
 *
 * This function is implemented using clz().
 */

inline unsigned ffs(uint32_t word)
{
    // Reference: http://en.wikipedia.org/wiki/Find_first_set#Properties_and_relations
    return 32 - __builtin_clz(word & -word);
}

/// Compute the remainder (modulo) operation for two floating point numbers. Single-precision.
float inline fmod(float a, float b)
{
    uint32_t r = _SYS_fmodf(reinterpret_cast<uint32_t&>(a), reinterpret_cast<uint32_t&>(b));
    return reinterpret_cast<float&>(r);
}

/// Compute the remainder (modulo) operation for two floating point numbers. Double-precision.
double inline fmod(double a, double b)
{
    uint64_t ia = reinterpret_cast<uint64_t&>(a);
    uint64_t ib = reinterpret_cast<uint64_t&>(b);
    uint64_t r = _SYS_fmod(ia, ia >> 32, ib, ib >> 32);
    return reinterpret_cast<double&>(r);
}

/// Compute 'x' raised to the power 'y'. Single-precision.
float inline pow(float a, float b)
{
    uint32_t r = _SYS_powf(reinterpret_cast<uint32_t&>(a), reinterpret_cast<uint32_t&>(b));
    return reinterpret_cast<float&>(r);
}

/// Compute 'x' raised to the power 'y'. Double-precision.
double inline pow(double a, double b)
{
    uint64_t ia = reinterpret_cast<uint64_t&>(a);
    uint64_t ib = reinterpret_cast<uint64_t&>(b);
    uint64_t r = _SYS_pow(ia, ia >> 32, ib, ib >> 32);
    return reinterpret_cast<double&>(r);
}

/// Compute the unsigned remainder from dividing two signed integers.
unsigned inline umod(int a, int b)
{
    int r = a % b;
    if (r < 0)
        r += b;
    return r;
}

/// Ceiling division. Divide, rounding up instead of down.
template <typename T> inline T ceildiv(T numerator, T denominator) {
    return (numerator + (denominator - 1)) / denominator;
}

/**
 * @brief Round 'numerator' up to the nearest multiple of 'denominator'.
 * Only for integer types.
 */
template <typename T> inline T roundup(T numerator, T denominator) {
    return ceildiv(numerator, denominator) * denominator;
}

/// Compute the natural log of a floating point number. Single-precision.
float inline log(float a)
{
    uint32_t r = _SYS_logf(reinterpret_cast<uint32_t&>(a));
    return reinterpret_cast<float&>(r);
}

/// Compute the natural log of a floating point number. Double-precision.
double inline log(double a)
{
    uint64_t ia = reinterpret_cast<uint64_t&>(a);
    uint64_t r = _SYS_logd(ia, ia >> 32);
    return reinterpret_cast<double&>(r);
}

/// Compute the square root of a floating point number. Single-precision.
float inline sqrt(float a)
{
    uint32_t r = _SYS_sqrtf(reinterpret_cast<uint32_t&>(a));
    return reinterpret_cast<float&>(r);
}

/// Compute the square root of a floating point number. Double-precision.
double inline sqrt(double a)
{
    uint64_t ia = reinterpret_cast<uint64_t&>(a);
    uint64_t r = _SYS_sqrt(ia, ia >> 32);
    return reinterpret_cast<double&>(r);
}

/**
 * @brief Calculate the sine of a specified angle, in radians. Single-precision.
 *
 * Trigonometry functions are very slow. If you're computing more than a few sine
 * or cosine values per frame, consider using the table-driven alternative tsin().
 */
float inline sin(float x)
{
    uint32_t r = _SYS_sinf(reinterpret_cast<uint32_t&>(x));
    return reinterpret_cast<float&>(r);
}

/**
 * @brief Calculate the cosine of a specified angle, in radians. Single-precision.
 *
 * Trigonometry functions are very slow. If you're computing more than a few sine
 * or cosine values per frame, consider using the table-driven alternative tsin().
 */
float inline cos(float x)
{
    uint32_t r = _SYS_cosf(reinterpret_cast<uint32_t&>(x));
    return reinterpret_cast<float&>(r);
}

/// Calculate the tangent of a specified angle, in radians. Single-precision.
float inline tan(float x)
{
    uint32_t r = _SYS_tanf(reinterpret_cast<uint32_t&>(x));
    return reinterpret_cast<float&>(r);
}

/// Single-precision arc tangent function, of one variable
float inline atan(float x)
{
    uint32_t r = _SYS_atanf(reinterpret_cast<uint32_t&>(x));
    return reinterpret_cast<float&>(r);
}

/**
 * @brief Single-precision arc tangent function, of two variables
 *
 * This computes the arc tangent of y/x, using the sign of each
 * argument to determine which quadrant the result is in.
 */
float inline atan2(float a, float b)
{
    uint32_t r = _SYS_atan2f(reinterpret_cast<uint32_t&>(a), reinterpret_cast<uint32_t&>(b));
    return reinterpret_cast<float&>(r);
}

/**
 * @brief Simultaneously compute the sine and cosine of a specified angle,
 * in radians.
 *
 * This yields two single-precision floating point results,
 * returned via the pointers 's' and 'c'.
 */
void inline sincos(float x, float *s, float *c)
{
    _SYS_sincosf(reinterpret_cast<uint32_t&>(x), s, c);
}

/**
 * @brief Table-driven drop-in replacement for sin()
 *
 * This calculates the sine of a specified angle, in radians. Usage is
 * identical to sin(), but the implementation is based on the same fast
 * table lookup used by tsini() and tcosi().
 */
float inline tsin(float x)
{
    uint32_t r = _SYS_tsinf(reinterpret_cast<uint32_t&>(x));
    return reinterpret_cast<float&>(r);
}

/**
 * @brief Table-driven drop-in replacement for cos()
 *
 * This calculates the cosine of a specified angle, in radians. Usage is
 * identical to cos(), but the implementation is based on the same fast
 * table lookup used by tsini() and tcosi().
 */
float inline tcos(float x)
{
    uint32_t r = _SYS_tcosf(reinterpret_cast<uint32_t&>(x));
    return reinterpret_cast<float&>(r);
}

/**
 * @brief Integer sine table lookup
 *
 * This is an all-integer table driven alternative to sin(). It is
 * very fast. The lookup table is stored in very fast internal flash
 * memory.
 *
 * The input angle is specified in units of 360/8192 degrees. A full
 * circle is exactly 8192 units, meaning that a 90 degree arc is 2048
 * units. The result is in fixed-point, with 16 bits to the right of
 * the binary point. A value of 1.0 is represented by exactly 65536.
 *
 * Only the low 13 bits of the input angle are used.
 */
int inline tsini(int x)
{
    return _SYS_tsini(x);
}

/**
 * @brief Integer cosine table lookup
 *
 * This is an all-integer table driven alternative to cos(). It is
 * very fast. The lookup table is stored in very fast internal flash
 * memory.
 *
 * The input angle is specified in units of 360/8192 degrees. A full
 * circle is exactly 8192 units, meaning that a 90 degree arc is 2048
 * units. The result is in fixed-point, with 16 bits to the right of
 * the binary point. A value of 1.0 is represented by exactly 65536.
 *
 * Only the low 13 bits of the input angle are used.
 * This function is implemented as a thin wrapper around tsini().
 */
int inline tcosi(int x)
{
    return _SYS_tcosi(x);
}

/**
 * @brief Unordered comparison.
 *
 * Given one or two single-precision floating point
 * numbers, is there no defined sort order between them?
 *
 * Returns true if either or both arguments are NaN.
 */

bool inline isunordered(float a, float b = 0.f)
{
    return __builtin_isunordered(a, b);
}

/**
 * @brief Unordered comparison.
 *
 * Given one or two double-precision floating point
 * numbers, is there no defined sort order between them?
 *
 * Returns true if either or both arguments are NaN.
 */

bool inline isunordered(double a, double b = 0.f)
{
    return __builtin_isunordered(a, b);
}

/**
 * @brief Returns the next integer value closer to positive infinity from 'value'.
 *
 * E.g.:
 *
 *     ceil(1.0) => 1.0
 *     ceil(1.1) => 2.0
 *     ceil(-1.0) => -1.0
 *     ceil(-1.1) => -1.0
 *     ceil(-1.9) => -1.0
 */

template <typename T> inline long ceil(const T value)
{
    long result = value;
    if (result >= 0 && result < value) {
        return result+1;
    }
    return result;
}

/**
 *  @brief Returns the next integer value closer to negative infinity from 'value'.
 *
 *  E.g.:
 *
 *      floor(1.0) => 1.0
 *      floor(1.9) => 1.0
 *      floor(-1.0) => -1.0
 *      floor(-1.1) => -2.0
 *      floor(-1.9) => -2.0
 */

template <typename T> inline long floor(const T value)
{
   long result = value;
   if (result <= 0 && result > value) {
       return result - 1;
   }
   return result;
}

/**
 * @brief Rounds 'value' to the nearest whole value.
 *
 * E.g.:
 *
 *     round(1.4) => 1.0
 *     round(1.5) => 2.0
 *
 *     round(-1.4) => -1.0
 *     round(-1.5) => -1.0
 *     round(-1.51) => -2.0
 */

template <typename T> inline long round(const T value)
{
    return floor(value + 0.5);
}

/**
 * @brief Returns true when 'a' and 'b' are within 'epsilon' of each other.
 */

template <typename T> inline bool almostEqual(const T a, const T b, const T epsilon)
{
    return abs(a-b) < epsilon;
}


/**
 * @brief Generalized two-element cartesian coordinate vector.
 */

template <typename T> struct Vector2 {
    T x;    ///< Vector component X
    T y;    ///< Vector component Y

    /**
     * @brief Modify this vector's value in-place.
     */
    void set(T _x, T _y) {
        x = _x;
        y = _y;
    }

    /**
     * @brief Set this vector to a new cartesian value, given a value in polar coordinates.
     */
    void setPolar(float angle, float magnitude) {
        float s, c;
        sincos(angle, &s, &c);
        x = c * magnitude;
        y = s * magnitude;
    }

    /**
     * @brief Rotate this vector about the origin counterclockwise by 'angle' radians.
     */
    Vector2<T> rotate(float angle) const {
        float s, c;
        sincos(angle, &s, &c);
        Vector2<T> result = { x*c - y*s, x*s + y*c };
        return result;
    }

    /**
     * @brief Rotate this vector about the origin counterclockwise by an integer
     * multiple of 90 degrees.
     *
     * The angle must be 0, 1, 2, or 3. If you
     * must pass larger angles, you can use umod(a, 4) to fold them into
     * this range.
     */
    Vector2<T> rotateI(int angle) const {
        Vector2<T> a1 = { -y,  x };
        Vector2<T> a2 = { -x, -y };
        Vector2<T> a3 = {  y, -x };
        switch (angle) {
            default: ASSERT(0);
            case 0: return *this;
            case 1: return a1;
            case 2: return a2;
            case 3: return a3;
        }
    }

    /**
     * @brief Create a unit vector corresponding to the given integer 'side'
     *
     *      0 = -Y (Up)
     *      1 = -X (Left)
     *      2 = +Y (Down)
     *      3 = +X (Right)
     */
    static Vector2<T> unit(int side) {
        Vector2<T> a0 = {  0, -1 };
        Vector2<T> a1 = { -1,  0 };
        Vector2<T> a2 = {  0,  1 };
        Vector2<T> a3 = {  1,  0 };
        switch(side) {
            default: ASSERT(0);
            case 0: return a0;
            case 1: return a1;
            case 2: return a2;
            case 3: return a3;
        }
    }

    /**
     * @brief Calculate the scalar length (magnitude) of this vector, squared.
     *
     * This avoids the costly square root calculation.
     */
    T len2() const {
        return ( x * x + y * y );
    }

    /**
     * @brief Calculate the scalar length (magnitude) of this vector.
     */
    T len() const {
        return sqrt(len2());
    }

    /**
     * @brief Calculate the manhattan (city block) distance of this vector.
     */
    T lenManhattan() const {
        return abs(x) + abs(y);
    }

    /**
     * @brief Return a normalized version of this vector.
     *
     * The returned vector will have a magnitude of 1.0.
     */
    Vector2<T> normalize() const {
        return *this / len();
    }
    
    /**
     * @brief Round a floating point vector to the nearest integer.
     */
    Vector2<int> round() const {
        Vector2<int> result = { x + 0.5f, y + 0.5f };
        return result;
    }

    /**
     * @brief Explicitly cast this vector to another vector type, using
     * default C++ truncation or extension rules.
     */
    template <typename R> Vector2<R> cast() const {
        Vector2<R> result = { x, y };
        return result;
    }

    /**
     * @brief Complex multiplication
     *
     * Treat this vector as a complex number, and multiply it with
     * another vector equivalently interpreted.
     */
    Vector2<T> cmul(Vector2<T> u) const {
        Vector2<T> result = { x*u.x - y*u.y, x*u.y + y*u.x };
        return result;
    }

    /// Explicit cast to int
    Vector2<int> toInt() const { return cast<int>(); }
    /// Explicit cast to float
    Vector2<float> toFloat() const { return cast<float>(); }
    /// Explicit cast to double
    Vector2<double> toDouble() const { return cast<double>(); }
    
    // Implicit casts
    operator Vector2<int>            () const { return cast<int>(); }
    operator Vector2<unsigned>       () const { return cast<unsigned>(); }
    operator Vector2<short>          () const { return cast<short>(); }
    operator Vector2<unsigned short> () const { return cast<unsigned short>(); }
    operator Vector2<int8_t>         () const { return cast<int8_t>(); }
    operator Vector2<uint8_t>        () const { return cast<uint8_t>(); }
    operator Vector2<float>          () const { return cast<float>(); }
    operator Vector2<double>         () const { return cast<double>(); }
};

typedef Vector2<int>                Int2;       ///< Typedef for a 2-vector of ints
typedef Vector2<unsigned>           UInt2;      ///< Typedef for a 2-vector of unsigned ints
typedef Vector2<short>              Short2;     ///< Typedef for a 2-vector of shorts
typedef Vector2<unsigned short>     UShort2;    ///< Typedef for a 2-vector of unsigned shorts
typedef Vector2<int8_t>             Byte2;      ///< Typedef for a 2-vector of bytes
typedef Vector2<uint8_t>            UByte2;     ///< Typedef for a 2-vector of unsigned bytes
typedef Vector2<float>              Float2;     ///< Typedef for a 2-vector of floats
typedef Vector2<double>             Double2;    ///< Typedef for a 2-vector of double-precision floats

/**
 * @brief Create a Vector2, from a set of (x,y) coordinates.
 *
 * This is a standalone function, instead of a constructor,
 * so that Vector2 can remain a POD type, and it can be used
 * in unions.
 */
 
template <typename T> inline Vector2<T> vec(T x, T y) {
    Vector2<T> result = { x, y };
    return result;
}

/// Vector dot-product
template <typename T> inline T dot(Vector2<T> u, Vector2<T> v) {
    return u.x * v.x + u.y * v.y;
}

/// Convert polar to cartesian
template <typename T> inline Vector2<T> polar(T angle, T magnitude) {
    Vector2<T> result;
    result.setPolar(angle, magnitude);
    return result;
}

// Vector operations
template <typename T> inline Vector2<T> operator-(Vector2<T> u) { return vec<T>(-u.x, -u.y); }
template <typename T> inline Vector2<T> operator+=(Vector2<T> &u, Vector2<T> v) { return vec<T>(u.x+=v.x, u.y+=v.y); }
template <typename T> inline Vector2<T> operator-=(Vector2<T> &u, Vector2<T> v) { return vec<T>(u.x-=v.x, u.y-=v.y); }
template <typename T> inline Vector2<T> operator+(Vector2<T> u, Vector2<T> v) { return vec<T>(u.x+v.x, u.y+v.y); }
template <typename T> inline Vector2<T> operator-(Vector2<T> u, Vector2<T> v) { return vec<T>(u.x-v.x, u.y-v.y); }
template <typename T> inline Vector2<T> operator*(Vector2<T> u, Vector2<T> v) { return vec<T>(u.x*v.x, u.y*v.y); }
template <typename T> inline bool operator==(Vector2<T> u, Vector2<T> v) { return u.x == v.x && u.y == v.y; }
template <typename T> inline bool operator!=(Vector2<T> u, Vector2<T> v) { return u.x != v.x || u.y != v.y; }

// Scalar int promotion
template <typename T> inline Int2 operator*(int k, Vector2<T> v) { return vec<int>(k*v.x, k*v.y); }
template <typename T> inline Int2 operator*(Vector2<T> v, int k) { return vec<int>(k*v.x, k*v.y); }
template <typename T> inline Int2 operator/(Vector2<T> v, int k) { return vec<int>(v.x/k, v.y/k); }
template <typename T> inline Int2 operator+=(Vector2<T> &u, int k) { return vec<int>(u.x+=k, u.y+=k); }
template <typename T> inline Int2 operator*=(Vector2<T> &u, int k) { return vec<int>(u.x*=k, u.y*=k); }
template <typename T> inline Int2 operator<<(Vector2<T> u, int shift) { return vec<int>(u.x<<shift, u.y<<shift); }
template <typename T> inline Int2 operator>>(Vector2<T> u, int shift) { return vec<int>(u.x>>shift, u.y>>shift); }

// Scalar float promotion
template <typename T> inline Float2 operator*(float k, Vector2<T> v) { return vec<float>(k*v.x, k*v.y); }
template <typename T> inline Float2 operator*(Vector2<T> v, float k) { return vec<float>(k*v.x, k*v.y); }
template <typename T> inline Float2 operator/(Vector2<T> v, float k) { return vec<float>(v.x/k, v.y/k); }
template <typename T> inline Float2 operator+=(Vector2<T> &u, float k) { return vec<float>(u.x+=k, u.y+=k); }
template <typename T> inline Float2 operator*=(Vector2<T> &u, float k) { return vec<float>(u.x*=k, u.y*=k); }

// Scalar double promotion
template <typename T> inline Double2 operator*(double k, Vector2<T> v) { return vec<double>(k*v.x, k*v.y); }
template <typename T> inline Double2 operator*(Vector2<T> v, double k) { return vec<double>(k*v.x, k*v.y); }
template <typename T> inline Double2 operator/(Vector2<T> v, double k) { return vec<double>(v.x/k, v.y/k); }
template <typename T> inline Double2 operator+=(Vector2<T> &u, double k) { return vec<double>(u.x+=k, u.y+=k); }
template <typename T> inline Double2 operator*=(Vector2<T> &u, double k) { return vec<double>(u.x*=k, u.y*=k); }

// Vector int -> float promotion
inline Vector2<float> operator+=(Vector2<float> &u, Int2 v) { return vec(u.x+=v.x, u.y+=v.y); }
inline Vector2<float> operator-=(Vector2<float> &u, Int2 v) { return vec(u.x-=v.x, u.y-=v.y); }
inline Vector2<float> operator+(Vector2<float> u, Int2 v) { return vec(u.x+v.x, u.y+v.y); }
inline Vector2<float> operator-(Vector2<float> u, Int2 v) { return vec(u.x-v.x, u.y-v.y); }
inline Vector2<float> operator*(Vector2<float> u, Int2 v) { return vec(u.x*v.x, u.y*v.y); }
inline Vector2<float> operator+(Int2 u, Vector2<float> v) { return vec(u.x+v.x, u.y+v.y); }
inline Vector2<float> operator-(Int2 u, Vector2<float> v) { return vec(u.x-v.x, u.y-v.y); }
inline Vector2<float> operator*(Int2 u, Vector2<float> v) { return vec(u.x*v.x, u.y*v.y); }


/**
 * @brief Generalized three-element cartesian coordinate vector.
 */

template <typename T> struct Vector3 {
    T x;    ///< Vector component X
    T y;    ///< Vector component Y
    T z;    ///< Vector component Z

    /// Modify this vector's value in-place.
    void set(T _x, T _y, T _z) {
        x = _x;
        y = _y;
        z = _z;
    }
    
    Vector2<T> xy() const { return vec(x, y); } ///< Extract a 2-vector with only the XY components
    Vector2<T> xz() const { return vec(x, z); } ///< Extract a 2-vector with only the XZ components
    Vector2<T> yx() const { return vec(y, x); } ///< Extract a 2-vector with only the YX components
    Vector2<T> yz() const { return vec(y, z); } ///< Extract a 2-vector with only the YZ components
    Vector2<T> zx() const { return vec(z, x); } ///< Extract a 2-vector with only the ZX components
    Vector2<T> zy() const { return vec(z, y); } ///< Extract a 2-vector with only the ZY components

    /**
     * @brief Calculate the scalar length (magnitude) of this vector, squared.
     *
     * This avoids the costly square root calculation.
     */
    T len2() const {
        return ( x * x + y * y + z * z );
    }

    /**
     * @brief Calculate the scalar length (magnitude) of this vector.
     */
    T len() const {
        return sqrt(len2());
    }

    /**
     * @brief Calculate the manhattan (city block) distance of this vector.
     */
    T lenManhattan() const {
        return abs(x) + abs(y) + abs(z);
    }

    /**
     * @brief Return a normalized version of this vector.
     *
     * The returned vector will have a magnitude of 1.0.
     */
    Vector3<T> normalize() const {
        return *this / len();
    }

    /**
     * @brief Round a floating point vector to the nearest integer.
     */
    Vector3<int> round() const {
        Vector3<int> result = { x + 0.5f, y + 0.5f, z + 0.5f };
        return result;
    }

    /**
     * @brief Rotate the vector about the Z axis counterclockwise by 'angle' radians.
     */
    Vector3<T> zRotate(float angle) const {
        float s, c;
        sincos(angle, &s, &c);
        Vector3<T> result = { x*c - y*s, x*s + y*c, z };
        return result;
    }

    /**
     * @brief Rotate this vector about the Z axis counterclockwise by an integer
     * multiple of 90 degrees.
     * 
     * The angle must be 0, 1, 2, or 3. If you
     * must pass larger angles, you can use umod(a, 4) to fold them into
     * this range.
     */
    Vector3<T> zRotateI(int angle) const {
        Vector3<T> a1 = { -y,  x, z };
        Vector3<T> a2 = { -x, -y, z };
        Vector3<T> a3 = {  y, -x, z };
        switch (angle) {
            default: ASSERT(0);
            case 0: return *this;
            case 1: return a1;
            case 2: return a2;
            case 3: return a3;
        }
    }

    /**
     * @brief Explicitly cast this vector to another vector type, using
     * default C++ truncation or extension rules.
     */
    template <typename R> Vector3<R> cast() const {
        Vector3<R> result = { x, y, z };
        return result;
    }
    
    /// Explicit cast to int
    Vector3<int> toInt() const { return cast<int>(); }
    /// Explicit cast to float
    Vector3<float> toFloat() const { return cast<float>(); }
    /// Explicit cast to double
    Vector3<double> toDouble() const { return cast<double>(); }
    
    // Implicit casts
    operator Vector3<int>            () const { return cast<int>(); }
    operator Vector3<unsigned>       () const { return cast<unsigned>(); }
    operator Vector3<short>          () const { return cast<short>(); }
    operator Vector3<unsigned short> () const { return cast<unsigned short>(); }
    operator Vector3<int8_t>         () const { return cast<int8_t>(); }
    operator Vector3<uint8_t>        () const { return cast<uint8_t>(); }
    operator Vector3<float>          () const { return cast<float>(); }
    operator Vector3<double>         () const { return cast<double>(); }
};

typedef Vector3<int>                Int3;       ///< Typedef for a 2-vector of ints
typedef Vector3<unsigned>           UInt3;      ///< Typedef for a 2-vector of unsigned ints
typedef Vector3<short>              Short3;     ///< Typedef for a 2-vector of shorts
typedef Vector3<unsigned short>     UShort3;    ///< Typedef for a 2-vector of unsigned shorts
typedef Vector3<int8_t>             Byte3;      ///< Typedef for a 2-vector of bytes
typedef Vector3<uint8_t>            UByte3;     ///< Typedef for a 2-vector of unsigned bytes
typedef Vector3<float>              Float3;     ///< Typedef for a 2-vector of floats
typedef Vector3<double>             Double3;    ///< Typedef for a 2-vector of double-precision floats

/**
 * @brief Create a Vector3, from a set of (x, y, z) coordinates.
 *
 * This is a standalone function, instead of a constructor,
 * so that Vector3 can remain a POD type, and it can be used
 * in unions.
 */
 
template <typename T> inline Vector3<T> vec(T x, T y, T z) {
    Vector3<T> result = { x, y, z };
    return result;
}

/// Vector dot-product
template <typename T> inline T dot(Vector3<T> u, Vector3<T> v) {
    return u.x * v.x + u.y * v.y + u.z * v.z;
}

/// Vector cross-product
template <typename T> inline Vector3<T> cross(T u, T v) {
    return vec(u.y * v.z - u.z * v.y,
               u.z * v.x - u.x * v.z,
               u.x * v.y - u.y * v.x);
}

// Vector operations
template <typename T> inline Vector3<T> operator-(Vector3<T> u) { return vec<T>(-u.x, -u.y, -u.z); }
template <typename T> inline Vector3<T> operator+=(Vector3<T> &u, Vector3<T> v) { return vec<T>(u.x+=v.x, u.y+=v.y, u.z+=v.z); }
template <typename T> inline Vector3<T> operator-=(Vector3<T> &u, Vector3<T> v) { return vec<T>(u.x-=v.x, u.y-=v.y, u.z-=v.z); }
template <typename T> inline Vector3<T> operator+(Vector3<T> u, Vector3<T> v) { return vec<T>(u.x+v.x, u.y+v.y, u.z+v.z); }
template <typename T> inline Vector3<T> operator-(Vector3<T> u, Vector3<T> v) { return vec<T>(u.x-v.x, u.y-v.y, u.z-v.z); }
template <typename T> inline Vector3<T> operator*(Vector3<T> u, Vector3<T> v) { return vec<T>(u.x*v.x, u.y*v.y, u.z*v.z); }
template <typename T> inline bool operator==(Vector3<T> u, Vector3<T> v) { return u.x == v.x && u.y == v.y && u.z == v.z; }
template <typename T> inline bool operator!=(Vector3<T> u, Vector3<T> v) { return u.x != v.x || u.y != v.y || u.z != v.z; }

// Scalar int promotion
template <typename T> inline Int3 operator*(int k, Vector3<T> v) { return vec<int>(k*v.x, k*v.y, k*v.z); }
template <typename T> inline Int3 operator*(Vector3<T> v, int k) { return vec<int>(k*v.x, k*v.y, k*v.z); }
template <typename T> inline Int3 operator/(Vector3<T> v, int k) { return vec<int>(v.x/k, v.y/k, v.z/k); }
template <typename T> inline Int3 operator+=(Vector3<T> &u, int k) { return vec<int>(u.x+=k, u.y+=k, u.z+=k); }
template <typename T> inline Int3 operator*=(Vector3<T> &u, int k) { return vec<int>(u.x*=k, u.y*=k, u.z*=k); }
template <typename T> inline Int3 operator<<(Vector3<T> u, int shift) { return vec<int>(u.x<<shift, u.y<<shift, u.z<<shift); }
template <typename T> inline Int3 operator>>(Vector3<T> u, int shift) { return vec<int>(u.x>>shift, u.y>>shift, u.z>>shift); }

// Scalar float promotion
template <typename T> inline Float3 operator*(float k, Vector3<T> v) { return vec<float>(k*v.x, k*v.y, k*v.z); }
template <typename T> inline Float3 operator*(Vector3<T> v, float k) { return vec<float>(k*v.x, k*v.y, k*v.z); }
template <typename T> inline Float3 operator/(Vector3<T> v, float k) { return vec<float>(v.x/k, v.y/k, v.z/k); }
template <typename T> inline Float3 operator+=(Vector3<T> &u, float k) { return vec<float>(u.x+=k, u.y+=k, u.z+=k); }
template <typename T> inline Float3 operator*=(Vector3<T> &u, float k) { return vec<float>(u.x*=k, u.y*=k, u.z*=k); }

// Scalar double promotion
template <typename T> inline Double3 operator*(double k, Vector3<T> v) { return vec<double>(k*v.x, k*v.y, k*v.z); }
template <typename T> inline Double3 operator*(Vector3<T> v, double k) { return vec<double>(k*v.x, k*v.y, k*v.z); }
template <typename T> inline Double3 operator/(Vector3<T> v, double k) { return vec<double>(v.x/k, v.y/k, v.z/k); }
template <typename T> inline Double3 operator+=(Vector3<T> &u, double k) { return vec<double>(u.x+=k, u.y+=k, u.z+=k); }
template <typename T> inline Double3 operator*=(Vector3<T> &u, double k) { return vec<double>(u.x*=k, u.y*=k, u.z*=k); }

// Vector int -> float promotion
inline Float3 operator+=(Float3 &u, Int3 v) { return vec(u.x+=v.x, u.y+=v.y, u.z+=v.z); }
inline Float3 operator-=(Float3 &u, Int3 v) { return vec(u.x-=v.x, u.y-=v.y, u.z-=v.z); }
inline Float3 operator+(Float3 u, Int3 v) { return vec(u.x+v.x, u.y+v.y, u.z+v.z); }
inline Float3 operator-(Float3 u, Int3 v) { return vec(u.x-v.x, u.y-v.y, u.z-v.z); }
inline Float3 operator*(Float3 u, Int3 v) { return vec(u.x*v.x, u.y*v.y, u.z*v.z); }
inline Float3 operator+(Int3 u, Float3 v) { return vec(u.x+v.x, u.y+v.y, u.z+v.z); }
inline Float3 operator-(Int3 u, Float3 v) { return vec(u.x-v.x, u.y-v.y, u.z-v.z); }
inline Float3 operator*(Int3 u, Float3 v) { return vec(u.x*v.x, u.y*v.y, u.z*v.z); }


/**
 * @brief Pseudo-random number generator.
 *
 * Each instance of the Random class has a distinct PRNG state.
 *
 * When possible, method semantics here have been designed to match
 * those used by Python's "random" module.
 */
 
struct Random {
    _SYSPseudoRandomState state;

    /// Construct a new random number generator, using an arbitrary seed.
    Random() {
        seed();
    }
    
    /// Construct a new random number generator with a well-defined seed.
    Random(uint32_t s) {
        seed(s);
    }
    
    /**
     * @brief Re-seed this random number generator.
     * 
     * For a given seed, the subsequent random numbers are guaranteed to be deterministic.
     */
    void seed(uint32_t s) {
        _SYS_prng_init(&state, s);
    }

    /**
     * @brief Re-seed this random number generator arbitrarily.
     *
     * This implementation uses the system's nanosecond timer.
     */
    void seed() {
        seed((uint32_t) _SYS_ticks_ns());
    }

    /// Returns the next raw 32-bit pseudo-random number
    uint32_t raw() {
        return _SYS_prng_value(&state);
    }
    
    /// Returns a uniformly distributed floating point number between 0 and 1, inclusive.
    float random() {
        return raw() * (1.0f / 0xFFFFFFFF);
    }
    
    /// Returns a generated pseudorandom inverval for a poisson process (e.g. whack-a-mole timing).
    float expovariate(float averageInterval) {
        return -averageInterval * log(1.0f - random());
    }
    
    /**
     * @brief Take a chance. Returns a boolean that has a 'probability'
     * chance of being 'true'.
     *
     * If the argument is constant, all floating point math folds
     * away at compile-time. For values of 0 and 1, we are guaranteed
     * to always return false or true, respectively.
     */
    bool chance(float probability) {
        // Use 31 bits, to give us range to represent probability=1.
        const uint32_t mask = 0x7FFFFFFF;
        uint32_t threshold = probability * (mask + 1);
        return (raw() & mask) < threshold;
    }

    /**
     * @brief Returns a uniformly distributed floating point number in the range [a, b) or
     * [a, b], depending on rounding.
     */
    float uniform(float a, float b) {
        // Order of operations here allows constant folding if the endpoints are constant
        return a + raw() * ((b-a) / 0xFFFFFFFF);
    }

    /**
     * @brief Returns a uniformly distributed random integer in the range [a, b], including both
     * end points.
     */
    template <typename T>
    T randint(T a, T b) {
        return T(a + _SYS_prng_valueBounded(&state, b - a));
    }

    /**
     * @brief Returns a uniformly distributed random integer in the half-open interval [a, b),
     * including the lower but not the upper end point.
     */
    template <typename T>
    T randrange(T a, T b) {
        return randint<T>(a, T(b - 1));
    }

    /**
     * @brief The one-argument variant of randrange() always starts at zero, and
     * returns an integer up to but not including 'count'.
     *
     * Guaranteed to be capable of returning 'count' distinct values, starting at zero.
     */
    template <typename T>
    T randrange(T count) {
        return randrange<T>(T(0), count);
    }

};


/**
 * @brief An augmented 3x2 matrix, for doing 2D affine transforms.
 *
 *      [ xx  yx  cx ]
 *      [ xy  yy  cy ]
 *      [  0   0   1 ]
 *
 * The way we use affine transforms for background scaling are
 * very similiar to the mechanism used by the GameBoy Advance
 * PPU. There's a great tutorial on this at:
 *
 * http://www.coranac.com/tonc/text/affine.htm
 */
struct AffineMatrix {
    float cx;   ///< Matrix member cx, the constant offset for X
    float cy;   ///< Matrix member cy, the constant offset for Y
    float xx;   ///< Matrix member xx, the horizontal X delta
    float xy;   ///< Matrix member xy, the horizontal Y delta
    float yx;   ///< Matrix member yx, the vertical X delta
    float yy;   ///< Matrix member yy, the vertical Y delta

    /// Create an uninitialized matrix
    AffineMatrix() {}

    /// Create a matrix from six scalar values
    AffineMatrix(float _xx, float _yx, float _cx,
                 float _xy, float _yy, float _cy)
        : cx(_cx), cy(_cy), xx(_xx),
          xy(_xy), yx(_yx), yy(_yy) {}
    
    /// Create the identity matrix.
    static AffineMatrix identity() {
        return AffineMatrix(1, 0, 0,
                            0, 1, 0);
    }

    /// Create a matrix which scales by a factor of 's'
    static AffineMatrix scaling(float s) {
        float inv_s = 1.0f / s;
        return AffineMatrix(inv_s, 0, 0,
                            0, inv_s, 0);
    }

    /// Create a matrix which translates by (x, y)
    static AffineMatrix translation(float x, float y) {
        return AffineMatrix(1, 0, x,
                            0, 1, y);
    }

    /// Create a matrix which translates by vector 'v'
    static AffineMatrix translation(Float2 v) {
        return AffineMatrix(1, 0, v.x,
                            0, 1, v.y);
    }

    /// Create a matrix which rotates by 'angle' radians.
    static AffineMatrix rotation(float angle) {
        float s, c;
        sincos(angle, &s, &c);
        return AffineMatrix(c, -s, 0,
                            s, c, 0);
    }

    /// Matrix multiplication
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

    /// Compose this matrix with a translation by (x, y)
    void translate(float x, float y) {
        *this *= translation(x, y);
    }

    /// Compose this matrix with a translation by vector 'v'
    void translate(Float2 v) {
        *this *= translation(v);
    }

    /// Compose this matrix with a rotation by 'angle' radians
    void rotate(float angle) {
        *this *= rotation(angle);
    }

    /// Compose this matrix with a scale by factor 's'
    void scale(float s) {
        *this *= scaling(s);
    }
};

/**
 * @} endgroup math
*/

}   // namespace Sifteo
