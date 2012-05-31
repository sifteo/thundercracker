#include <sifteo/math.h>
using namespace Sifteo;

// Optimization barrier
template <typename T> T b(T x) {
    volatile T y = x;
    return y;
}

void testClamp()
{
    ASSERT((uint32_t) b(10) == clamp<uint32_t>(b(5), b(10), b(100)));
    ASSERT((uint32_t) b(50) == clamp<uint32_t>(b(50), b(10), b(100)));
    ASSERT((uint32_t) b(100) == clamp<uint32_t>(b(105), b(10), b(100)));
}

void testCeil()
{
    ASSERT(b(1) == ceil(b(1.0f)));
    ASSERT(b(2) == ceil(b(1.1f)));
    ASSERT(b(2) == ceil(b(1.9f)));
    ASSERT(b(2) == ceil(b(1.999999f)));
    ASSERT(b(-1) == ceil(b(-1.0f)));
    ASSERT(b(-1) == ceil(b(-1.1f)));
    ASSERT(b(-1) == ceil(b(-1.9f)));
}

void testFloor()
{
    ASSERT(b(1) == floor(b(1.0f)));
    ASSERT(b(1) == floor(b(1.1f)));
    ASSERT(b(1) == floor(b(1.9f)));
    ASSERT(b(1) == floor(b(1.999999f)));

    ASSERT(b(-1) == floor(b(-1.0f)));
    ASSERT(b(-2) == floor(b(-1.1f)));
    ASSERT(b(-2) == floor(b(-1.9f)));
    ASSERT(b(-2) == floor(b(-1.999999f)));
}

void testRound()
{
    ASSERT(b(1) == round(b(1.0f)));
    ASSERT(b(1) == round(b(1.1f)));
    ASSERT(b(1) == round(b(1.49999f)));
    ASSERT(b(2) == round(b(1.5f)));
    ASSERT(b(2) == round(b(1.6f)));

    ASSERT(b(-1) == round(b(-1.0f)));
    ASSERT(b(-1) == round(b(-1.1f)));
    ASSERT(b(-1) == round(b(-1.5f)));
    ASSERT(b(-2) == round(b(-1.51f)));
    ASSERT(b(-2) == round(b(-1.9f)));
}

void testAlmostEqual()
{
    ASSERT(almostEqual(1.0f, 1.0009f, 1e-3f));
    ASSERT(!almostEqual(1.0f, 1.001f, 1e-3f));

    ASSERT(almostEqual(1.0f, 0.999f, 1e-3f));
    ASSERT(!almostEqual(1.0f, 0.9989f, 1e-3f));
}

void testLog()
{
    // No need to use almostEqual, these should be bit-accurate.
    ASSERT(log(2.0) == b(M_LN2));
    ASSERT(log(2.0f) == float(b(M_LN2)));
}

void testExceptions()
{
    /*
     * None of these often-illegal operations should generate faults.
     * Check that they return expected values.
     */

    // Integer divide by zero == 0
    ASSERT(b(50) / b(0) == 0);
    ASSERT(b(50U) / b(0U) == 0);
    ASSERT(b(50LL) / b(0LL) == 0);
    ASSERT(b(50LLU) / b(0LLU) == 0);
    
    // Integer modulo (a % 0) == a
    ASSERT((b(50) % b(0)) == 50);
    ASSERT((b(50U) % b(0U)) == 50);
    ASSERT((b(50LL) % b(0LL)) == 50);
    ASSERT((b(50LLU) % b(0LLU)) == 50);

    // Float divide by zero == +/- infinity
    ASSERT(b(50.f) / b(0.f) > MAXFLOAT);
    ASSERT(b(-50.f) / b(0.f) < -MAXFLOAT);

    // NaN
    ASSERT(isunordered(b(0.f)) == false);
    ASSERT(isunordered(b(NAN)) == true);
    ASSERT(isunordered(b(1.f), b(0.f)) == false);
    ASSERT(isunordered(b(1.f), b(NAN)) == true);
    ASSERT(isunordered(b(0.0)) == false);
    ASSERT(isunordered(b(double(NAN))) == true);
    ASSERT(isunordered(b(1.0), b(0.0)) == false);
    ASSERT(isunordered(b(1.0), b(double(NAN))) == true);

    // sqrt(-1) == NAN
    ASSERT(isunordered(sqrt(b(-1.f))) == true);
    ASSERT(isunordered(sqrt(b(-1.0))) == true);

    // log(0) == -infinity
    ASSERT(log(b(0.f)) < -MAXFLOAT);
    ASSERT(log(b(0.0)) < -MAXFLOAT);

    // log(-1) == NAN
    ASSERT(isunordered(log(b(-1.f))) == true);
    ASSERT(isunordered(log(b(-1.0))) == true);

    // fmod(0,1) == 0
    ASSERT(fmod(b(0.f), b(1.f)) == 0.f);
    ASSERT(fmod(b(0.0), b(1.0)) == 0.0);

    // fmod(inf, 1) == NAN
    ASSERT(isunordered(fmod(b(log(-1.f)), b(1.f))) == true);
    ASSERT(isunordered(fmod(b(log(-1.0)), b(1.0))) == true);

    // fmod(1, 0) == NAN
    ASSERT(isunordered(fmod(b(1.f), b(0.f))) == true);
    ASSERT(isunordered(fmod(b(1.0), b(0.0))) == true);
}

void main()
{
    testClamp();
    testCeil();
    testFloor();
    testRound();
    testAlmostEqual();
    testLog();
    testExceptions();

    LOG("Success.\n");
}