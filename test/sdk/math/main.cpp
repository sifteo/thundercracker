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

void main()
{
    testClamp();
    testCeil();
    testFloor();
    testRound();
    testAlmostEqual();

    LOG("Success.\n");
}