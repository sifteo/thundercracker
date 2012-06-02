/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * This file is part of the public interface to the Sifteo SDK.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#pragma once
#ifdef NOT_USERSPACE
#   error This is a userspace-only header, not allowed by the current build.
#endif

#include <sifteo/abi.h>

namespace Sifteo {

/**
 * @defgroup string String
 *
 * @{
 */

/**
 * @brief String formatting wrapper for fixed-width integers.
 *
 * This is a type which can be used with String's << operator,
 * to format integers with a fixed field width.
 */
struct Fixed {
    /**
     * @brief Format 'value' using exactly 'width' characters.
     *
     * If 'leadingZeroes', extra leading characters will be '0',
     * otherwise they will be spaces.
     */
    Fixed(int value, unsigned width, bool leadingZeroes=false)
        : value(value), width(width), leadingZeroes(leadingZeroes) {}
    int value;
    unsigned width;
    bool leadingZeroes;
};

/**
 * @brief String formatting wrapper for fixed-width hexadecimal integers.
 *
 * This is a type which can be used with String's << operator,
 * to format integers in hexadecimal, with a fixed field width.
 */
struct Hex {
    /**
     * @brief Format 'value' using exactly 'width' characters.
     *
     * If 'leadingZeroes', extra leading characters will be '0',
     * otherwise they will be spaces.
     */
    Hex(uint32_t value, unsigned width=8, bool leadingZeroes=true)
        : value(value), width(width), leadingZeroes(leadingZeroes) {}
    uint32_t value;
    unsigned width;
    bool leadingZeroes;
};

/**
 * @brief String formatting wrapper for fixed-width hexadecimal 64-bit integers.
 *
 * This is a type which can be used with String's << operator,
 * to format 64-bit integers in hexadecimal, with a fixed field width.
 */
struct Hex64 {
    /**
     * @brief Format 'value' using exactly 'width' characters.
     *
     * If 'leadingZeroes', extra leading characters will be '0',
     * otherwise they will be spaces.
     */
    Hex64(uint64_t value, unsigned width=16, bool leadingZeroes=true)
        : value(value), width(width), leadingZeroes(leadingZeroes) {}
    uint64_t value;
    unsigned width;
    bool leadingZeroes;
};


/**
 * @brief Compare two C-style strings.
 *
 * Returns a number less than, equal to, or greater than zero,
 * corresponding to a < b, a==b, and a > b respectively.
 *
 * The number returned is equal to the difference (a[i] - b[i])
 * where i is the index of the first non-matching character.
 */
inline int strncmp(const char *a, const char *b, unsigned count)
{
    return _SYS_strncmp(a, b, count);
}


/**
 * @brief A statically sized character buffer, with output formatting support.
 *
 * Method naming and conventions are STL-inspired, but designed for very
 * minimal runtime memory footprint.
 *
 * Strings are always 8-bit and NUL terminated. Encoding is assumed to
 * be ASCII or UTF-8.
 */

template <unsigned _capacity>
class String {
public:

    typedef char* iterator;
    typedef const char* const_iterator;

    /// Initialize a new empty string
    String() {
        clear();
    }

    /// Return a C-style character pointer
    char * c_str() {
        return buffer;
    }
    
    /// Return a C-style constant character pointer
    const char * c_str() const {
        return buffer;
    }
    
    /// Implicit conversion to a C-style character pointer
    operator char *() {
        return buffer;
    }

    /// Implicit conversion to a C-style constant character pointer
    operator const char *() const {
        return buffer;
    }

    /// Retrieve the size of the buffer, in bytes, including space for NUL termination.
    static unsigned capacity() {
        return _capacity;
    }

    /// Get the current size of the string, in characters, excluding NUL termination.
    unsigned size() const {
        return _SYS_strnlen(buffer, _capacity-1);
    }

    /// Return an iterator, pointing to the first character in the string.
    iterator begin() {
        return &buffer[0];
    }

    /// Return an iterator, pointing just past the end of the string (at the NUL)
    iterator end() {
        return &buffer[size()];
    }

    /// Const version of begin()
    const_iterator begin() const {
        return &buffer[0];
    }

    /// Const version of end()
    const_iterator end() const {
        return &buffer[size()];
    }

    /// Overwrite this with the empty string
    void clear() {
        buffer[0] = '\0';
    }

    /// Is the string empty? (Faster than size() == 0)
    bool empty() const {
        return buffer[0] == '\0';
    }

    /**
     * @brief Compare this string against another.
     *
     * Returns a number less than, equal to, or greater than zero,
     * corresponding to this < other, this==other, and this > other respectively.
     *
     * The number returned is equal to the difference (this[i] - other[i])
     * where i is the index of the first non-matching character.
     */
    template <class T> int compare(const T &other) const {
        return strncmp(*this, other, min(capacity(), other.capacity()));
    }

    /**
     * @brief Compare this string against a C-style string.
     *
     * Returns a number less than, equal to, or greater than zero,
     * corresponding to this < other, this==other, and this > other respectively.
     *
     * The number returned is equal to the difference (this[i] - other[i])
     * where i is the index of the first non-matching character.
     */
    int compare(const char *other) const {
        return strncmp(*this, other, capacity());
    }

    /// Overwrite this with another string
    String& operator=(const char *src) {
        _SYS_strlcpy(buffer, src, _capacity);
        return *this;
    }

    /// Append another string to this one
    String& operator+=(const char *src) {
        _SYS_strlcat(buffer, src, _capacity);
        return *this;
    }

    /// Indexing operator, to get or set one character
    char &operator[](unsigned index) {
        return buffer[index];
    }

    /// Indexing operator, to get or set one character
    char &operator[](int index) {
        return buffer[index];
    }

    /// Const indexing operator
    char operator[](unsigned index) const {
        return buffer[index];
    }

    /// Const indexing operator
    char operator[](int index) const {
        return buffer[index];
    }

    /// STL-style formatting operator, append a string
    String& operator<<(const char *src) {
        _SYS_strlcat(buffer, src, _capacity);
        return *this;
    }

    /// STL-style formatting operator, append a decimal integer
    String& operator<<(int src) {
        _SYS_strlcat_int(buffer, src, _capacity);
        return *this;
    }

    /// STL-style formatting operator, append a fixed-width decimal integer
    String& operator<<(const Fixed &src) {
        _SYS_strlcat_int_fixed(buffer, src.value, src.width, src.leadingZeroes, _capacity);
        return *this;
    }

    /// STL-style formatting operator, append a fixed-width hexadecimal integer
    String& operator<<(const Hex &src) {
        _SYS_strlcat_int_hex(buffer, src.value, src.width, src.leadingZeroes, _capacity);
        return *this;
    }

    /// STL-style formatting operator, append a fixed-width hexadecimal 64-bit integer
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

    /// Is this string equal to another?
    template <typename T> bool operator==(const T &other) {
        return compare(other) == 0;
    }

    /// Is this string different from another?
    template <typename T> bool operator!=(const T &other) {
        return compare(other) != 0;
    }

    /// Is this string before another, in ASCII order?
    template <typename T> bool operator<(const T &other) {
        return compare(other) < 0;
    }

    /// Is this string before another or equal, in ASCII order?
    template <typename T> bool operator<=(const T &other) {
        return compare(other) <= 0;
    }

    /// Is this string after another, in ASCII order?
    template <typename T> bool operator>(const T &other) {
        return compare(other) > 0;
    }

    /// Is this string after another or equal, in ASCII order?
    template <typename T> bool operator>=(const T &other) {
        return compare(other) >= 0;
    }

private:
    char buffer[_capacity];
};

/**
 * @} end defgroup string
 */

};  // namespace Sifteo
