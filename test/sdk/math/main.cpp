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

void main()
{
    testClamp();
}