/*
 * Thundercracker Firmware -- Confidential, not for redistribution.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#ifndef CRC_H
#define CRC_H

#include "macros.h"


class Crc32 {
public:
    static void init();
    static void deinit();

    static void reset();
    static uint32_t get();
    static void add(uint32_t word);

    /**
     * Add platform-specific data which is unique per-device.
     * Used to create harder-to-guess object hashes. This will add()
     * some platform-specific nonzero number of words to the CRC unit.
     */
    static void addUniqueness();

    /**
     * CRC a block of words. Data must be word-aligned.
     */
    static uint32_t block(const uint32_t *words, uint32_t count);

    template <typename T>
    static uint32_t object(T &o) {
        STATIC_ASSERT((sizeof(o) & 3) == 0);
        ASSERT((reinterpret_cast<uintptr_t>(&o) & 3) == 0);
        return block(reinterpret_cast<const uint32_t*>(&o), sizeof(o) / 4);
    }
};

#endif // CRC_H
