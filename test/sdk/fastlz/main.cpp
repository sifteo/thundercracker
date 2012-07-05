/*
 * Check the FastLZ1 implementation against a reference.
 * The compressor uses too much memory to run inside SVM,
 * so we use some pre-canned compressed data to test.
 *
 * This same compression engine is also used for the binary's
 * initialized data. Check out some initialized data too, assuming
 * the raw decompress worked.
 */

#include <sifteo.h>
using namespace Sifteo;

#include "fastlz.h"
#include "testdata.h"   

static const uint8_t refString[] = "Hello Worrrrrrrrrrrrrrrrrrrrrrrrrrrrrld! This is a test of compressed RWDATA!";
uint8_t rwString[] = "Hello Worrrrrrrrrrrrrrrrrrrrrrrrrrrrrld! This is a test of compressed RWDATA!";

// Optimization barrier
template <typename T> T ob(T x) {
    volatile T y = x;
    return y;
}

void main()
{
    static uint8_t buffer[32000];

    // First, use the reference decoder to check our data.
    unsigned size = fastlz_decompress(testdata_compressed, sizeof testdata_compressed, buffer, sizeof buffer);
    ASSERT(size == sizeof testdata_plaintext - 1);
    ASSERT(!memcmp8(testdata_plaintext, buffer, size));
    bzero(buffer);
    ASSERT(memcmp8(testdata_plaintext, buffer, size));

    // Now check the firmware's decoder
    unsigned size2 = _SYS_decompress_fastlz1(buffer, sizeof buffer, testdata_compressed, sizeof testdata_compressed);
    ASSERT(size2 == size);
    ASSERT(!memcmp8(testdata_plaintext, buffer, size));

    // Yes, we can write to rwdata
    rwString[0] ^= ob(0xFF);
    rwString[0] ^= ob(0xFF);

    // Check the rwdata
    STATIC_ASSERT(sizeof refString == sizeof rwString);
    ASSERT(!memcmp8(&refString[0], &rwString[0], sizeof refString));

    LOG("Success.\n");
}