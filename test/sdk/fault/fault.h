#pragma once
#include <sifteo.h>
using namespace Sifteo;

/*
 * Utilities
 */

inline void call(const uint16_t *block, unsigned offset = 0) {
    // SVM magic.. convert RODATA address to call address (fnstack=0)
    typedef void (*fn)(int);
    LOG("--- Running %P +%02x\n", block, offset);
    reinterpret_cast<fn>(offset + (0xFFFFFF & reinterpret_cast<uintptr_t>(block)))(0);
}

// Convenience method for defining machine code chunks
#define CODE_BLOCK(name) static const uint16_t name[] __attribute__ ((aligned(256))) =

/*
 * Test entry points
 */

void testMemoryFaults();
void testSvmValidator();
