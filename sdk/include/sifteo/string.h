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
 * @brief String formatting classes
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
 * @brief Format a floating point number using fixed precision.
 *
 * This is a type which can be used with String's << operator,
 * to format floating point numbers with a fixed number of
 * digits to the left and to the right of the decimal point.
 *
 * Note that this operates by converting both the left and right
 * side into integers, and printing them separately.
 */
struct FixedFP {
    /**
     * @brief Format 'value' using exactly 'left' digits to the left of the decimal, and 'right' digits to the right.
     *
     * If 'leadingZeroes', extra leading characters will be '0',
     * otherwise they will be spaces.
     */
    FixedFP(float value, unsigned left, unsigned right, bool leadingZeroes=false)
        : widthL(left), widthR(right), leadingZeroes(leadingZeroes)
    {
        // Left side: truncate to integer
        valueL = int(value);

        /*
         * Right side: We want to convert the right side to an integer by
         * multiplying it by 10^right, then we can rely on String's
         * field width truncation to do the rest. There aren't very many
         * values of 'right' that won't overflow, so we can do this
         * with a lookup table. Typically the field width is constant
         * at compile-time, so this lookup table will optimize out.
         */
        switch (right) {
            default:   ASSERT(0);
            case 0:    valueR = value * 0; break;
            case 1:    valueR = value * 10; break;
            case 2:    valueR = value * 100; break;
            case 3:    valueR = value * 1000; break;
            case 4:    valueR = value * 10000; break;
            case 5:    valueR = value * 100000; break;
            case 6:    valueR = value * 1000000; break;
            case 7:    valueR = value * 10000000; break;
            case 8:    valueR = value * 100000000; break;
            case 9:    valueR = value * 1000000000; break;
        }
    }

    int valueL;
    int valueR;
    unsigned widthL, widthR;
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
 * @brief Get the length of a C-style string.
 *
 * 'maxLen' is required to limit the search for the end of the string. This
 * routine will fault if 'maxLen' extends beyond valid memory.
 */
inline unsigned strnlen(const char *str, uint32_t maxLen) {
    return _SYS_strnlen(str, maxLen);
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

template <unsigned tCapacity>
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
        return tCapacity;
    }

    /// Get the current size of the string, in characters, excluding NUL termination.
    unsigned size() const {
        return _SYS_strnlen(buffer, tCapacity-1);
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
        return strncmp(*this, other, MIN(capacity(), other.capacity()));
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
        _SYS_strlcpy(buffer, src, tCapacity);
        return *this;
    }

    /// Append another string to this one
    String& operator+=(const char *src) {
        _SYS_strlcat(buffer, src, tCapacity);
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
        _SYS_strlcat(buffer, src, tCapacity);
        return *this;
    }

    /// STL-style formatting operator, append a decimal integer
    String& operator<<(int src) {
        _SYS_strlcat_int(buffer, src, tCapacity);
        return *this;
    }

    /// STL-style formatting operator, append a fixed-width decimal integer
    String& operator<<(const Fixed &src) {
        _SYS_strlcat_int_fixed(buffer, src.value, src.width, src.leadingZeroes, tCapacity);
        return *this;
    }

    /// STL-style formatting operator, append a fixed-width floating point number
    String& operator<<(const FixedFP &src) {
        _SYS_strlcat_int_fixed(buffer, src.valueL, src.widthL, src.leadingZeroes, tCapacity);
        _SYS_strlcat(buffer, ".", tCapacity);
        _SYS_strlcat_int_fixed(buffer, src.valueR, src.widthR, true, tCapacity);
        return *this;
    }

    /// STL-style formatting operator, append a fixed-width hexadecimal integer
    String& operator<<(const Hex &src) {
        _SYS_strlcat_int_hex(buffer, src.value, src.width, src.leadingZeroes, tCapacity);
        return *this;
    }

    /// STL-style formatting operator, append a fixed-width hexadecimal 64-bit integer
    String& operator<<(const Hex64 &src) {
        uint32_t high = src.value >> 32;
        uint32_t low = src.value;
        if (src.width > 8 || high != 0) {
            _SYS_strlcat_int_hex(buffer, high, src.width - 8, src.leadingZeroes, tCapacity);
            _SYS_strlcat_int_hex(buffer, low, 8, true, tCapacity);
        } else {
            _SYS_strlcat_int_hex(buffer, low, src.width, src.leadingZeroes, tCapacity);
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
    char buffer[tCapacity];
};

/**
 * @} end defgroup string
 */

};  // namespace Sifteo
