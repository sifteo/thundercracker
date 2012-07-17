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

    // pow(0, -1.5) == infinity
    ASSERT(pow(b(0.f), b(-1.5f)) > MAXFLOAT);
    ASSERT(pow(b(0.0), b(-1.5 )) > MAXFLOAT);

    // pow(-1, 1.5) == NAN
    ASSERT(isunordered(pow(b(-1.f), b(1.5f))) == true);
    ASSERT(isunordered(pow(b(-1.0), b(1.5 ))) == true);
}

void testBits()
{
    // Test bit operations

    ASSERT(clz(b(0x80000000)) == 0);
    ASSERT(clz(b(0x4fffffff)) == 1);
    ASSERT(clz(b(0x35555555)) == 2);
    ASSERT(clz(b(0x1aaaaaaa)) == 3);
    ASSERT(clz(b(0x0fffffff)) == 4);
    ASSERT(clz(b(0x04000000)) == 5);
    ASSERT(clz(b(0x02000001)) == 6);
    ASSERT(clz(b(0x01000010)) == 7);
    ASSERT(clz(b(0x00800100)) == 8);
    ASSERT(clz(b(0x00401000)) == 9);
    ASSERT(clz(b(0x00210000)) == 10);
    ASSERT(clz(b(0x00100003)) == 11);
    ASSERT(clz(b(0x00080030)) == 12);
    ASSERT(clz(b(0x00040300)) == 13);
    ASSERT(clz(b(0x00023000)) == 14);
    ASSERT(clz(b(0x00010000)) == 15);
    ASSERT(clz(b(0x00008007)) == 16);
    ASSERT(clz(b(0x00004070)) == 17);
    ASSERT(clz(b(0x00002700)) == 18);
    ASSERT(clz(b(0x0000100c)) == 19);
    ASSERT(clz(b(0x000008c0)) == 20);
    ASSERT(clz(b(0x000004ff)) == 21);
    ASSERT(clz(b(0x000003ff)) == 22);
    ASSERT(clz(b(0x00000100)) == 23);
    ASSERT(clz(b(0x00000080)) == 24);
    ASSERT(clz(b(0x00000040)) == 25);
    ASSERT(clz(b(0x00000020)) == 26);
    ASSERT(clz(b(0x00000010)) == 27);
    ASSERT(clz(b(0x00000008)) == 28);
    ASSERT(clz(b(0x00000004)) == 29);
    ASSERT(clz(b(0x00000003)) == 30);
    ASSERT(clz(b(0x00000001)) == 31);
    ASSERT(clz(b(0x00000000)) == 32);

    ASSERT(ffs(b(0x00000000)) == 0);
    ASSERT(ffs(b(0xffffffff)) == 1);
    ASSERT(ffs(b(0x7ffffff2)) == 2);
    ASSERT(ffs(b(0x5555555c)) == 3);
    ASSERT(ffs(b(0xaaaaaaa8)) == 4);
    ASSERT(ffs(b(0x80000010)) == 5);
    ASSERT(ffs(b(0x40000020)) == 6);
    ASSERT(ffs(b(0x20000040)) == 7);
    ASSERT(ffs(b(0x10000080)) == 8);
    ASSERT(ffs(b(0x08000100)) == 9);
    ASSERT(ffs(b(0x04000200)) == 10);
    ASSERT(ffs(b(0x02000400)) == 11);
    ASSERT(ffs(b(0x01000800)) == 12);
    ASSERT(ffs(b(0x00801000)) == 13);
    ASSERT(ffs(b(0x00402000)) == 14);
    ASSERT(ffs(b(0x00204000)) == 15);
    ASSERT(ffs(b(0x00108000)) == 16);
    ASSERT(ffs(b(0x00010000)) == 17);
    ASSERT(ffs(b(0x00020000)) == 18);
    ASSERT(ffs(b(0x00040000)) == 19);
    ASSERT(ffs(b(0x00080000)) == 20);
    ASSERT(ffs(b(0x00100000)) == 21);
    ASSERT(ffs(b(0x00200000)) == 22);
    ASSERT(ffs(b(0x00400000)) == 23);
    ASSERT(ffs(b(0x00800000)) == 24);
    ASSERT(ffs(b(0x01000000)) == 25);
    ASSERT(ffs(b(0x02000000)) == 26);
    ASSERT(ffs(b(0x04000000)) == 27);
    ASSERT(ffs(b(0x08000000)) == 28);
    ASSERT(ffs(b(0x10000000)) == 29);
    ASSERT(ffs(b(0x20000000)) == 30);
    ASSERT(ffs(b(0x40000000)) == 31);
    ASSERT(ffs(b(0x80000000)) == 32);
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
    testBits();

    LOG("Success.\n");
}