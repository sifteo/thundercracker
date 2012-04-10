/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * This file is part of the public interface to the Sifteo SDK.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#ifndef _SIFTEO_MEMORY_H
#define _SIFTEO_MEMORY_H

#ifdef NO_USERSPACE_HEADERS
#   error This is a userspace-only header, not allowed by the current build.
#endif

#include <sifteo/abi.h>

namespace Sifteo {

/// memset(), with an explicit 8-bit data width
inline void memset8(uint8_t *dest, uint8_t value, unsigned count) {
    _SYS_memset8(dest, value, count);
}

/// memset(), with an explicit 16-bit data width
inline void memset16(uint16_t *dest, uint16_t value, unsigned count) {
    _SYS_memset16(dest, value, count);
}

/// memset(), with an explicit 32-bit data width
inline void memset32(uint32_t *dest, uint32_t value, unsigned count) {
    _SYS_memset32(dest, value, count);
}

/// memcpy(), with an explicit 8-bit data width
inline void memcpy8(uint8_t *dest, const uint8_t *src, unsigned count) {
    _SYS_memcpy8(dest, src, count);
}

/// memcpy(), with an explicit 16-bit data width
inline void memcpy16(uint16_t *dest, const uint16_t *src, unsigned count) {
    _SYS_memcpy16(dest, src, count);
}

/// memcpy(), with an explicit 32-bit data width
inline void memcpy32(uint32_t *dest, const uint32_t *src, unsigned count) {
    _SYS_memcpy32(dest, src, count);
}

/// memcmp(), with an explicit 8-bit data width
inline int memcmp8(const uint8_t *a, const uint8_t *b, unsigned count) {
    return _SYS_memcmp8(a, b, count);
}

/// memset(), with an implicit 8-bit data width
inline void memset(uint8_t *dest, uint8_t value, unsigned count) {
    _SYS_memset8(dest, value, count);
}

/// memset(), with an implicit 16-bit data width
inline void memset(uint16_t *dest, uint16_t value, unsigned count) {
    _SYS_memset16(dest, value, count);
}

/// memset(), with an implicit 32-bit data width
inline void memset(uint32_t *dest, uint32_t value, unsigned count) {
    _SYS_memset32(dest, value, count);
}

/// memcpy(), with an implicit 8-bit data width
inline void memcpy(uint8_t *dest, const uint8_t *src, unsigned count) {
    _SYS_memcpy8(dest, src, count);
}

/// memcpy(), with an implicit 16-bit data width
inline void memcpy(uint16_t *dest, const uint16_t *src, unsigned count) {
    _SYS_memcpy16(dest, src, count);
}

/// memcpy(), with an implicit 32-bit data width
inline void memcpy(uint32_t *dest, const uint32_t *src, unsigned count) {
    _SYS_memcpy32(dest, src, count);
}

/// memcmp(), with an implicit 8-bit data width
inline int memcmp(const uint8_t *a, const uint8_t *b, unsigned count) {
    return _SYS_memcmp8(a, b, count);
}

/// Write 'n' zero bytes to memory
inline void bzero(void *s, unsigned count) {
    _SYS_memset8((uint8_t*)s, 0, count);
}

/// One-argument form of bzero: Templatized to zero any fixed-size object
template <typename T>
inline void bzero(T &s) {
    _SYS_memset8((uint8_t*)&s, 0, sizeof s);
}


};  // namespace Sifteo

#endif
