#include <sifteo/memory.h>
#include <sifteo/macros.h>
#include <sifteo/math.h>
using namespace Sifteo;


/*
 * This is the reference implementation we include in the SDK docs.
 * Test it both to test the real CRC engine and to make sure our docs aren't lying.
 */
uint32_t referenceCRC32(const uint8_t *bytes, int count)
{
    int32_t crc = -1;
    while (count > 0)
    {
        uint8_t byte0 = count-- > 0 ? *bytes++ : 0xFF;
        uint8_t byte1 = count-- > 0 ? *bytes++ : 0xFF;
        uint8_t byte2 = count-- > 0 ? *bytes++ : 0xFF;
        uint8_t byte3 = count-- > 0 ? *bytes++ : 0xFF;

        int32_t word = (byte3 << 24) | (byte2 << 16) | (byte1 << 8) | byte0;

        for (unsigned bit = 32; bit; --bit) {
            crc = (crc << 1) ^ ((crc ^ word) < 0 ? 0x04c11db7 : 0);
            word = word << 1;
        }
    }
    return crc;
}


void testVectors()
{
    /*
     * Run some pre-determined test vectors, and comprare against
     * both reference values and our reference implementation.
     */

    ASSERT(crc32(0, 0) == 0xffffffff);

    static const uint32_t a[] = { 0x00000000, 0x00000000, 0x12345678, 0x70116d20 };

    ASSERT(crc32(a, 0) == 0xffffffff);
    ASSERT(crc32(a, 4) == 0xc704dd7b);
    ASSERT(crc32(a, 8) == 0x6904bb59);
    ASSERT(crc32(a, 12) == 0x11120371);
    ASSERT(crc32(a, 16) == 0x97d3a9b4);

    for (unsigned i = 0; i < sizeof a; ++i)
        ASSERT(crc32(a, i) == referenceCRC32((const uint8_t*)a, i));

    static const uint32_t b[] = { 0x00000000, 0x00000001 };

    ASSERT(crc32(b, 0) == 0xffffffff);
    ASSERT(crc32(b, 4) == 0xc704dd7b);
    ASSERT(crc32(b, 8) == 0x6dc5a6ee);

    for (unsigned i = 0; i < sizeof b; ++i)
        ASSERT(crc32(b, i) == referenceCRC32((const uint8_t*)b, i));

    static const uint32_t c[] = { 0x12345678, 0x0000abcd };
    ASSERT(crc32(c, 8) == 0xa93e040d);
    ASSERT(crc32(c) == 0xa93e040d);
    for (unsigned i = 0; i < sizeof c; ++i)
        ASSERT(crc32(c, i) == referenceCRC32((const uint8_t*)c, i));
}

void testFlash()
{
    /*
     * Test CRCs against data in flash, using various alignments.
     *
     * For the sake of having some arbitrary data to test with, we
     * use the text of our program.
     */

    static const uint32_t sentinel = 0x1234;
    const uint8_t *begin = reinterpret_cast<const uint8_t*>(0x80000000);
    const uint8_t *end = reinterpret_cast<const uint8_t*>(&sentinel);
    uint32_t length = 0xFFFFFF & (end - begin);

    ASSERT(length < 0x100000);
    ASSERT(length > 1024);

    // Front side alignments
    for (unsigned i = 0; i < 32; i++)
        ASSERT(crc32(begin + i, length - i) == referenceCRC32(begin + i, length - i));

    // Front side alignments near a block boundary
    for (unsigned i = 240; i < 280; i++)
        ASSERT(crc32(begin + i, length - i) == referenceCRC32(begin + i, length - i));

    // Back side alignments
    for (unsigned i = 0; i < 32; i++)
        ASSERT(crc32(begin, length - i) == referenceCRC32(begin, length - i));
}

void main()
{
    testVectors();
    testFlash();

    LOG("Success.\n");
}