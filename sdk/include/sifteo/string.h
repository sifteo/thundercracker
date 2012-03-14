/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * This file is part of the public interface to the Sifteo SDK.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#ifndef _SIFTEO_STRING_H
#define _SIFTEO_STRING_H

#include <sifteo/abi.h>

namespace Sifteo {

    
struct Fixed {
    Fixed(int value, unsigned width, bool leadingZeroes=false)
        : value(value), width(width), leadingZeroes(leadingZeroes) {}
    int value;
    unsigned width;
    bool leadingZeroes;
};

struct Hex {
    Hex(uint32_t value, unsigned width=8, bool leadingZeroes=true)
        : value(value), width(width), leadingZeroes(leadingZeroes) {}
    uint32_t value;
    unsigned width;
    bool leadingZeroes;
};

struct Hex64 {
    Hex64(uint64_t value, unsigned width=16, bool leadingZeroes=true)
        : value(value), width(width), leadingZeroes(leadingZeroes) {}
    uint64_t value;
    unsigned width;
    bool leadingZeroes;
};


/**
 * A statically sized character buffer, with output formatting support.
 *
 * Method naming and conventions are STL-inspired, but designed for very
 * minimal runtime memory footprint.
 */

template <unsigned _capacity>
class String {
public:

    String() {
        clear();
    }

    char * c_str() {
        return buffer;
    }
    
    const char * c_str() const {
        return buffer;
    }
    
    operator char *() {
        return buffer;
    }

    operator const char *() const {
        return buffer;
    }

    static unsigned capacity() {
        return _capacity;
    }

    unsigned size() const {
        return _SYS_strnlen(buffer, _capacity-1);
    }

    void clear() {
        buffer[0] = '\0';
    }
    
    bool empty() const {
        return buffer[0] == '\0';
    }

    String& operator=(const char *src) {
        _SYS_strlcpy(buffer, src, _capacity);
        return *this;
    }

    String& operator+=(const char *src) {
        _SYS_strlcat(buffer, src, _capacity);
        return *this;
    }

    char operator[](unsigned index) const {
        return buffer[index];
    }

    char operator[](int index) const {
        return buffer[index];
    }

    String& operator<<(const char *src) {
        _SYS_strlcat(buffer, src, _capacity);
        return *this;
    }

    String& operator<<(int src) {
        _SYS_strlcat_int(buffer, src, _capacity);
        return *this;
    }

    String& operator<<(const Fixed &src) {
        _SYS_strlcat_int_fixed(buffer, src.value, src.width, src.leadingZeroes, _capacity);
        return *this;
    }

    String& operator<<(const Hex &src) {
        _SYS_strlcat_int_hex(buffer, src.value, src.width, src.leadingZeroes, _capacity);
        return *this;
    }

    String& operator<<(const Hex64 &src) {
        uint32_t high = src.value >> 32;
        uint32_t low = src.value;
        if (src.width > 8 || high != 0) {
            _SYS_strlcat_int_hex(buffer, high, src.width - 8, src.leadingZeroes, _capacity);
            _SYS_strlcat_int_hex(buffer, low, 8, true, _capacity);
        } else {
            _SYS_strlcat_int_hex(buffer, low, src.width, src.leadingZeroes, _capacity);
        }
        return *this;
    }
    
private:
    char buffer[_capacity];
};


};  // namespace Sifteo

#endif
