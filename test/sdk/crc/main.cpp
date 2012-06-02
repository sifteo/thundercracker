#include <sifteo/memory.h>
#include <sifteo/macros.h>
#include <sifteo/math.h>
using namespace Sifteo;


/*
 * This is the reference implementation we include in the SDK docs.
 * Test it both to test the real CRC engine and to make sure our docs aren't lying.
 */
uint32_t slowCrc32(const uint32_t *data, uint32_t count)
{
    int32_t crc = -1;
    while (count--) {
        int32_t word = *(data++);
        for (unsigned bit = 32; bit; --bit) {
            crc = (crc << 1) ^ ((crc ^ word) < 0 ? 0x04c11db7 : 0);
            word = word << 1;
        }
    }
    return crc;
}

void main()
{
    ASSERT(crc32(0, 0) == 0xffffffff);

    static const uint32_t a[] = { 0x00000000, 0x00000000, 0x12345678, 0x70116d20 };
    ASSERT(crc32(a, 0) == 0xffffffff);
    ASSERT(crc32(a, 1) == 0xc704dd7b);
    ASSERT(crc32(a, 2) == 0x6904bb59);
    ASSERT(crc32(a, 3) == 0x11120371);
    ASSERT(crc32(a, 4) == 0x97d3a9b4);
    ASSERT(crc32(a, 0) == slowCrc32(a, 0));
    ASSERT(crc32(a, 1) == slowCrc32(a, 1));
    ASSERT(crc32(a, 2) == slowCrc32(a, 2));
    ASSERT(crc32(a, 3) == slowCrc32(a, 3));
    ASSERT(crc32(a, 4) == slowCrc32(a, 4));

    static const uint32_t b[] = { 0x00000000, 0x00000001 };
    ASSERT(crc32(b, 0) == 0xffffffff);
    ASSERT(crc32(b, 1) == 0xc704dd7b);
    ASSERT(crc32(b, 2) == 0x6dc5a6ee);
    ASSERT(crc32(b, 0) == slowCrc32(b, 0));
    ASSERT(crc32(b, 1) == slowCrc32(b, 1));
    ASSERT(crc32(b, 2) == slowCrc32(b, 2));

    static const uint32_t c[] = { 0x12345678, 0x0000abcd };
    ASSERT(crc32(c, arraysize(c)) == 0xa93e040d);
    ASSERT(crc32(c, arraysize(c)) == slowCrc32(c, arraysize(c)));
    ASSERT(crc32(a, 1) == slowCrc32(a, 1));
    ASSERT(crc32(a, 2) == slowCrc32(a, 2));
    ASSERT(crc32(a, 3) == slowCrc32(a, 3));
    ASSERT(crc32(a, 4) == slowCrc32(a, 4));

    Random r;
    for (int i = 0; i < 100; i++) {
        uint32_t value = r.raw();
        ASSERT(crc32(&value, 1) == slowCrc32(&value, 1));
    }

    LOG("Success.\n");
}