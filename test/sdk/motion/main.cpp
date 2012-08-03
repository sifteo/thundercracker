#include <sifteo/array.h>
#include <sifteo/cube.h>
using namespace Sifteo;

void testIntegrate()
{
    _SYSInt3 result;
    _SYSMotionBuffer mbuf;
    bzero(mbuf);

    // Any length integrate over an empty buffer should yield zero.

    _SYS_motion_integrate(&mbuf, 0, &result);
    ASSERT(result.x == 0);
    ASSERT(result.y == 0);
    ASSERT(result.z == 0);

    _SYS_motion_integrate(&mbuf, 1, &result);
    ASSERT(result.x == 0);
    ASSERT(result.y == 0);
    ASSERT(result.z == 0);

    _SYS_motion_integrate(&mbuf, -1, &result);
    ASSERT(result.x == 0);
    ASSERT(result.y == 0);
    ASSERT(result.z == 0);

    // Integrate several samples with equal weights

    mbuf.header.last = 7;
    mbuf.header.tail = 0;
    mbuf.samples[0].value = 0x00010203;
    mbuf.samples[1].value = 0x00010204;
    mbuf.samples[2].value = 0x00010207;
    mbuf.samples[3].value = 0x000102FF;
    mbuf.samples[4].value = 0x00010203;
    mbuf.samples[5].value = 0x00010210;
    mbuf.samples[6].value = 0x000102FE;
    mbuf.samples[7].value = 0x00010203;

    _SYS_motion_integrate(&mbuf, 8, &result);
    ASSERT(result.x == 2 * (3 + 4 + 7 - 1 + 3 + 16 - 2 + 3));
    ASSERT(result.y == 2 * 8 * 2);
    ASSERT(result.z == 1 * 8 * 2);

    _SYS_motion_integrate(&mbuf, 4, &result);
    ASSERT(result.x == (-1 + 3) + 2 * (16 - 2 + 3));
    ASSERT(result.y == 2 * 4 * 2);
    ASSERT(result.z == 1 * 4 * 2);

    // Integrate with nonequal weights

    mbuf.header.last = 255;
    mbuf.header.tail = 100;
    mbuf.samples[96].value = 0x0101027F;
    mbuf.samples[97].value = 0x0F0102FF;
    mbuf.samples[98].value = 0x01010203;
    mbuf.samples[99].value = 0x02010210;

    // Start just after sample [97]
    _SYS_motion_integrate(&mbuf, 5, &result);
    ASSERT(result.x == (16+3)*3 + (3-1)*2);
    ASSERT(result.y == 2 * 5 * 2);
    ASSERT(result.z == 1 * 5 * 2);

    // Starting halfway through sample [97]
    _SYS_motion_integrate(&mbuf, 13, &result);
    ASSERT(result.x == (16+3)*3 + (3-1)*2 + (-1+63)*8);
    ASSERT(result.y == 2 * 13 * 2);
    ASSERT(result.z == 1 * 13 * 2);
}

void testMedian()
{
    _SYSMotionMedian result;
    _SYSMotionBuffer mbuf;
    bzero(mbuf);

    // Any length median over an empty buffer should yield zero.

    _SYS_motion_median(&mbuf, 0, &result);
    ASSERT(result.axes[0].median == 0);
    ASSERT(result.axes[1].median == 0);
    ASSERT(result.axes[2].median == 0);
    ASSERT(result.axes[0].minimum == 0);
    ASSERT(result.axes[1].minimum == 0);
    ASSERT(result.axes[2].minimum == 0);
    ASSERT(result.axes[0].maximum == 0);
    ASSERT(result.axes[1].maximum == 0);
    ASSERT(result.axes[2].maximum == 0);

    _SYS_motion_median(&mbuf, 1, &result);
    ASSERT(result.axes[0].median == 0);
    ASSERT(result.axes[1].median == 0);
    ASSERT(result.axes[2].median == 0);
    ASSERT(result.axes[0].minimum == 0);
    ASSERT(result.axes[1].minimum == 0);
    ASSERT(result.axes[2].minimum == 0);
    ASSERT(result.axes[0].maximum == 0);
    ASSERT(result.axes[1].maximum == 0);
    ASSERT(result.axes[2].maximum == 0);

    _SYS_motion_median(&mbuf, -1, &result);
    ASSERT(result.axes[0].median == 0);
    ASSERT(result.axes[1].median == 0);
    ASSERT(result.axes[2].median == 0);
    ASSERT(result.axes[0].minimum == 0);
    ASSERT(result.axes[1].minimum == 0);
    ASSERT(result.axes[2].minimum == 0);
    ASSERT(result.axes[0].maximum == 0);
    ASSERT(result.axes[1].maximum == 0);
    ASSERT(result.axes[2].maximum == 0);

    // Same simple test case from above, equal weights

    mbuf.header.last = 7;
    mbuf.header.tail = 0;
    mbuf.samples[0].value = 0x00010203;
    mbuf.samples[1].value = 0x00010204;
    mbuf.samples[2].value = 0x00010207;
    mbuf.samples[3].value = 0x000102FF;
    mbuf.samples[4].value = 0x00010203;
    mbuf.samples[5].value = 0x00010210;
    mbuf.samples[6].value = 0x000102FE;
    mbuf.samples[7].value = 0x00010203;

    _SYS_motion_median(&mbuf, 8, &result);
    ASSERT(result.axes[0].median == 3);
    ASSERT(result.axes[1].median == 2);
    ASSERT(result.axes[2].median == 1);
    ASSERT(result.axes[0].minimum == -2);
    ASSERT(result.axes[1].minimum == 2);
    ASSERT(result.axes[2].minimum == 1);
    ASSERT(result.axes[0].maximum == 16);
    ASSERT(result.axes[1].maximum == 2);
    ASSERT(result.axes[2].maximum == 1);
}

void main()
{
    testIntegrate();
    testMedian();
    
    LOG("Success.\n");
}
