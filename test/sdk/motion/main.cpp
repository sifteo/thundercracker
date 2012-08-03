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
    mbuf.buf[0].value = 0x00010203;
    mbuf.buf[1].value = 0x00010204;
    mbuf.buf[2].value = 0x00010207;
    mbuf.buf[3].value = 0x000102FF;
    mbuf.buf[4].value = 0x00010203;
    mbuf.buf[5].value = 0x00010210;
    mbuf.buf[6].value = 0x000102FE;
    mbuf.buf[7].value = 0x00010203;

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
    mbuf.buf[96].value = 0x0101027F;
    mbuf.buf[97].value = 0x0F0102FF;
    mbuf.buf[98].value = 0x01010203;
    mbuf.buf[99].value = 0x02010210;

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

void main()
{
    testIntegrate();
    
    LOG("Success.\n");
}
