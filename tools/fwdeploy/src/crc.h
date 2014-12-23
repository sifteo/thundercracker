/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * Thundercracker firmware deployment tool
 *
 * Copyright <c> 2012 Sifteo, Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

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
