/*
 * Thundercracker Firmware -- Confidential, not for redistribution.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#ifndef CRC_H
#define CRC_H

#include "macros.h"

class CrcStream;


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
    static uint32_t object(const T &o) {
        STATIC_ASSERT((sizeof(o) & 3) == 0);
        ASSERT((reinterpret_cast<uintptr_t>(&o) & 3) == 0);
        return block(reinterpret_cast<const uint32_t*>(&o), sizeof(o) / 4);
    }

private:
    /**
     * This is an internal version of add() which allows inlining.
     *
     * Normally we do *not* inline add(), since we use the function calling
     * delay to enforce the required delay of 4 AHB clock cycles per word
     * in our CRC hardware. The inlined version does not include this delay,
     * so using it directly without care can cause incorrect CRC results.
     * However, we want this fast version available for internal use in
     * block() and CrcStream.
     */
    static void addInline(uint32_t word);

    friend class CrcStream;
};


/**
 * A stateful utility class for CRC'ing streams of unaligned bytes.
 * This buffers partial words of data, and allows the end of the stream
 * to be padded up until a specified alignment boundary.
 */

class CrcStream {
public:
    void reset() {
        byteTotal = 0;
        Crc32::reset();
    }

    uint32_t get(unsigned alignment = 4) {
        padToAlignment(alignment);
        return Crc32::get();
    }

    void addBytes(const uint8_t *bytes, uint32_t count);
    void padToAlignment(unsigned alignment, uint8_t padByte = 0xFF);

private:
    unsigned byteTotal;
    union {
        uint32_t word;
        uint8_t bytes[4];
    } buffer;
};


/**********************************************************************
 *
 * Implementation is inlined on hardware, since these entry points are
 * simply thin wrappers around the hardware CRC32 engine.
 */
 
#ifndef SIFTEO_SIMULATOR

ALWAYS_INLINE void Crc32::init()
{
    RCC.AHBENR |= (1 << 6);
}

ALWAYS_INLINE void Crc32::deinit()
{
    RCC.AHBENR &= ~(1 << 6);
}

ALWAYS_INLINE void Crc32::reset()
{
    CRC.CR = (1 << 0);
}

ALWAYS_INLINE uint32_t Crc32::get()
{
    return CRC.DR;
}

ALWAYS_INLINE void Crc32::addInline(uint32_t word)
{
    /*
     * This is only called directly by block operations
     * in Crc32 and CrcStream which take care to ensure that
     * at least the minimum delay of 4 AHB clock cycles has
     * elapsed between words.
     */
    CRC.DR = word;
}

#endif // SIFTEO_SIMULATOR

#endif // CRC_H
