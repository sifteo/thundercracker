/*
 * Thundercracker Firmware -- Confidential, not for redistribution.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

/*
 * Implementations for string operation syscalls.
 *
 * This is similar to the functionality in string.h, but provided
 * as a set of syscalls that operate on virtual memory.
 */

#include <sifteo/abi.h>
#include "svmmemory.h"

extern "C" {

#define MEMSET_BODY() {                                                 \
    if (SvmMemory::mapRAM(dest,                                         \
            SvmMemory::arraySize(sizeof *dest, count))) {               \
        while (count) {                                                 \
            *(dest++) = value;                                          \
            count--;                                                    \
        }                                                               \
    }                                                                   \
}

void _SYS_memset8(uint8_t *dest, uint8_t value, uint32_t count) MEMSET_BODY()
void _SYS_memset16(uint16_t *dest, uint16_t value, uint32_t count) MEMSET_BODY()
void _SYS_memset32(uint32_t *dest, uint32_t value, uint32_t count) MEMSET_BODY()

void _SYS_memcpy8(uint8_t *dest, const uint8_t *src, uint32_t count)
{
    FlashBlockRef ref;
    if (SvmMemory::mapRAM(dest, count))
        SvmMemory::copyROData(ref, dest, reinterpret_cast<SvmMemory::VirtAddr>(src), count);
}

void _SYS_memcpy16(uint16_t *dest, const uint16_t *src, uint32_t count)
{
    // Currently implemented in terms of memcpy8. We may provide a
    // separate optimized implementation of this syscall in the future.   
    _SYS_memcpy8((uint8_t*) dest, (const uint8_t*) src,
        SvmMemory::arraySize(sizeof *dest, count));
}

void _SYS_memcpy32(uint32_t *dest, const uint32_t *src, uint32_t count)
{
    // Currently implemented in terms of memcpy8. We may provide a
    // separate optimized implementation of this syscall in the future.   
    _SYS_memcpy8((uint8_t*) dest, (const uint8_t*) src,
        SvmMemory::arraySize(sizeof *dest, count));
}

int32_t _SYS_memcmp8(const uint8_t *a, const uint8_t *b, uint32_t count)
{
    FlashBlockRef refA, refB;
    SvmMemory::VirtAddr vaA = reinterpret_cast<SvmMemory::VirtAddr>(a);
    SvmMemory::VirtAddr vaB = reinterpret_cast<SvmMemory::VirtAddr>(b);
    SvmMemory::PhysAddr paA, paB;

    while (count) {
        uint32_t chunkA = count;
        uint32_t chunkB = count;

        if (!SvmMemory::mapROData(refA, vaA, chunkA, paA))
            break;
        if (!SvmMemory::mapROData(refB, vaB, chunkB, paB))
            break;

        uint32_t chunk = MIN(chunkA, chunkB);
        vaA += chunk;
        vaB += chunk;
        count -= chunk;

        while (chunk) {
            int diff = *(paA++) - *(paB++);
            if (diff)
                return diff;
            chunk--;
        }
    }

    return 0;
}

uint32_t _SYS_strnlen(const char *str, uint32_t maxLen)
{
    FlashBlockRef ref;
    SvmMemory::VirtAddr va = reinterpret_cast<SvmMemory::VirtAddr>(str);
    SvmMemory::PhysAddr pa;
    
    uint32_t len = 0;

    while (len < maxLen) {
        uint32_t chunk = maxLen - len;
        if (!SvmMemory::mapROData(ref, va, chunk, pa))
            break;

        va += chunk;        
        while (chunk) {
            if (len == maxLen || !*pa)
                return len;
            pa++;
            len++;
            chunk--;
        }
    }
    
    return len;
}

void _SYS_strlcpy(char *dest, const char *src, uint32_t destSize)
{
    /*
     * Like the BSD strlcpy(), guaranteed to NUL-terminate.
     * We check the src pointer as we go, since the size is not known ahead of time.
     */
     
    if (destSize == 0 || !SvmMemory::mapRAM(dest, destSize))
        return;

    FlashBlockRef ref;
    SvmMemory::VirtAddr srcVA = reinterpret_cast<SvmMemory::VirtAddr>(src);
    char *last = dest + destSize - 1;

    while (dest < last) {
        char c = SvmMemory::peek<uint8_t>(ref, srcVA);
        if (c) {
            *(dest++) = c;
            srcVA++;
        } else {
            break;
        }
    }

    // Guaranteed to NUL-terminate
    *dest = '\0';
}

void _SYS_strlcat(char *dest, const char *src, uint32_t destSize)
{
    if (destSize == 0 || !SvmMemory::mapRAM(dest, destSize))
        return;

    FlashBlockRef ref;
    SvmMemory::VirtAddr srcVA = reinterpret_cast<SvmMemory::VirtAddr>(src);
    char *last = dest + destSize - 1;

    // Skip to NUL character
    while (dest < last && *dest)
        dest++;

    // Append all the bytes we can
    while (dest < last) {
        char c = SvmMemory::peek<uint8_t>(ref, srcVA);
        if (c) {
            *(dest++) = c;
            srcVA++;
        } else {
            break;
        }
    }

    // Guaranteed to NUL-termiante
    *dest = '\0';
}

void _SYS_strlcat_int(char *dest, int src, uint32_t destSize)
{
    /*
     * Integer to string conversion. Currently uses snprintf
     * (or sniprintf on embedded builds) but doesn't necessarily need to.
     */

    if (destSize == 0 || !SvmMemory::mapRAM(dest, destSize))
        return;

    char *last = dest + destSize - 1;

    // Skip to NUL character
    while (dest < last && *dest)
        dest++;

    if (dest < last)
        snprintf(dest, last-dest, "%d", src);

    // Guaranteed to NUL-termiante
    *last = '\0';
}

void _SYS_strlcat_int_fixed(char *dest, int src, unsigned width, unsigned lz, uint32_t destSize)
{
    if (destSize == 0 || !SvmMemory::mapRAM(dest, destSize))
        return;

    char *last = dest + destSize - 1;

    // Skip to NUL character
    while (dest < last && *dest)
        dest++;

    if (dest < last)
        snprintf(dest, last-dest, lz ? "%0*d" : "%*d", width, src);

    // Guaranteed to NUL-termiante
    *last = '\0';
}

void _SYS_strlcat_int_hex(char *dest, int src, unsigned width, unsigned lz, uint32_t destSize)
{
    if (destSize == 0 || !SvmMemory::mapRAM(dest, destSize))
        return;

    char *last = dest + destSize - 1;

    // Skip to NUL character
    while (dest < last && *dest)
        dest++;

    if (dest < last)
        snprintf(dest, last-dest, lz ? "%0*x" : "%*x", width, src);

    // Guaranteed to NUL-termiante
    *last = '\0';
}

int32_t _SYS_strncmp(const char *a, const char *b, uint32_t count)
{
    FlashBlockRef refA, refB;
    SvmMemory::VirtAddr vaA = reinterpret_cast<SvmMemory::VirtAddr>(a);
    SvmMemory::VirtAddr vaB = reinterpret_cast<SvmMemory::VirtAddr>(b);
    SvmMemory::PhysAddr paA, paB;

    while (count) {
        uint32_t chunkA = count;
        uint32_t chunkB = count;

        if (!SvmMemory::mapROData(refA, vaA, chunkA, paA))
            break;
        if (!SvmMemory::mapROData(refB, vaB, chunkB, paB))
            break;

        uint32_t chunk = MIN(chunkA, chunkB);
        vaA += chunk;
        vaB += chunk;
        count -= chunk;

        while (chunk) {
            uint8_t aV = *(paA++);
            uint8_t bV = *(paB++);
            int diff = aV - bV;
            if (diff)
                return diff;
            if (!aV)
                return 0;
            chunk--;
        }
    }

    return 0;
}

}  // extern "C"
