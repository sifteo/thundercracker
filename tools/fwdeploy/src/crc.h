#ifndef CRC_H
#define CRC_H

#include <stdint.h>
#include "macros.h"

/**
 * This is a thin wrapper around the STM32's hardware CRC engine.
 * It uses the standard CRC-32 polynomial, and always operates on
 * an entire 32-bit word at a time.
 *
 * Since there's a single global CRC engine with global state, we have
 * to be careful about when the Crc32 is used. Currently it is NOT
 * allowed to use the CRC asynchronously from SVM execution. So, not in
 * the radio ISR, button ISR, or any other exception handler other than
 * SVC. It is also NOT acceptable to use Crc32 from inside low-level
 * parts of the flash stack, since it's used internally by the Volume
 * and LFS layers.
 *
 * This CRC engine is also exposed to userspace, via the _SYS_crc32()
 * system call.
 */

class Crc32 {
public:
    static void init();
    static void deinit();

    static void reset();
    static uint32_t get();
    static void add(uint32_t word);

    /**
     * CRC a block of words. Data must be word-aligned.
     */
    static uint32_t block(const uint32_t *words, uint32_t count);

    template <typename T>
    static uint32_t object(const T &o) {
        STATIC_ASSERT((sizeof(o) & 3) == 0);
        ASSERT((reinterpret_cast<uintptr_t>(&o) & 3) == 0);
        return block(reinterpret_cast<const uint32_t*>(&o), sizeof(o) / 4);
    }
};

#endif // CRC_H
