/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * This file is part of the public interface to the Sifteo SDK.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#pragma once
#ifdef NOT_USERSPACE
#   error This is a userspace-only header, not allowed by the current build.
#endif

#include <sifteo/memory.h>
#include <sifteo/macros.h>

namespace Sifteo {

/**
 * @defgroup array Array
 *
 * @brief Container classes, Array and BitArray
 * 
 * @{
 */

/**
 * @brief A statically sized array
 *
 * Method naming and conventions are STL-inspired, but designed for very
 * minimal runtime memory footprint.
 */
template <typename T, unsigned tCapacity, typename sizeT = uint32_t>
class Array {
public:

    typedef T* iterator;
    typedef const T* const_iterator;
    static const unsigned NOT_FOUND = unsigned(-1);

    /// Initialize a new empty array.
    Array() {
        clear();
    }

    /// Retrieve the capacity of this array, always constant at compile-time.
    static unsigned capacity() {
        return tCapacity;
    }

    /// How many items does this array currently hold?
    unsigned count() const {
        return numItems;
    }

    /**
     * @brief Change the number of elements currently in the array.
     *
     * If this is less than the current count, the array is truncated. If
     * it is greater than the current count, the added elements will have
     * undefined values.
     */
    void setCount(unsigned c) {
        ASSERT(c <= tCapacity);
        numItems = c;
    }

    /// Is this array empty?
    bool empty() const {
        return count() == 0;
    }

    /// Accessor for array elements
    const T& operator[](unsigned index) const {
        ASSERT(index < count());
        return items[index];
    }

    /// Accessor for array elements
    const T& operator[](int index) const {
        ASSERT(0 <= index && index < (int)count());
        return items[index];
    }

    /// Accessor for array elements
    T& operator[](unsigned index) {
        ASSERT(index < count());
        return items[index];
    }

    /// Accessor for array elements
    T& operator[](int index) {
        ASSERT(0 <= index && index < (int)count());
        return items[index];
    }

    /// Synonym for push_back()
    T& append(const T &newItem) {
        return push_back(newItem);
    }

    /// Synonym for push_back()
    T& append() {
        return push_back();
    }

    /// Copy 'newItem' to the first unused slot in the array, and return a reference to the copy.
    T& push_back(const T &newItem) {
        ASSERT(count() < tCapacity);
        T &item = items[numItems++];
        item = newItem;
        return item;
    }

    /// Allocate the first unused slot in the array, and return an reference to this uninitialized slot.
    T& push_back() {
        ASSERT(count() < tCapacity);
        return items[numItems++];
    }

    /// Deallocate the last element and return a copy of it's value
    T pop_back() {
        ASSERT(count() > 0);
        numItems--;
        return items[numItems];
    }

    /// Pop-back, but avoid a potential unnecessary copy
    void pop_back(T* outValue) {
        ASSERT(outValue != 0);
        ASSERT(count() > 0);
        numItems--;
        *outValue = items[numItems];
    }
    
    /// Equivalent to setCount(0)
    void clear() {
        numItems = 0;
    }

    /// Remove a specific element, by index, and shift the others down.
    void erase(unsigned index) {
        // allow a.erase(a.find()) without checking for not-found.
        if (index == NOT_FOUND)
            return;

        erase(items + index);
    }

    /// Remove the element at iterator 'item' and shift the others down.
    void erase(iterator item) {
        ASSERT(item >= begin() && item < end());
        iterator next = item + 1;
        memcpy((uint8_t*)item, (uint8_t*)next, (uint8_t*)end() - (uint8_t*)next);
        numItems--;
    }

    /// Remove the last element, which involves no shifting
    void erase_tail() {
        ASSERT(count() > 0);
        numItems--;
    }
    
    /// Return an iterator pointing to the first slot in the array.
    iterator begin() {
        return &items[0];
    }

    /// Return an iterator pointing just past the last in-use slot in the array.
    iterator end() {
        return &items[count()];
    }

    /**
     * Locate an item by equality comparison.
     *
     * Using operator==(), tests 'itemToFind' against all items in
     * the array, returning the index of the first match, if any.
     * If no elements are found, returns the static member constant
     * NOT_FOUND.
     */
    unsigned find(T itemToFind)
    {
        int index = 0;
        for (iterator I=begin(), E=end(); I != E; ++I, ++index) {
            if (*I == itemToFind)
                return index;
        }
        return NOT_FOUND;
    }

private:
    T items[tCapacity];
    sizeT numItems;
};


/**
 * @brief A fixed-size array of bits, with compact storage and fast iteration.
 *
 * Supports arrays with any fixed-size number of bits. For sizes <= 32 bits,
 * this is just as efficient as using a bare uint32_t.
 *
 * This is a Plain Old Data type, with no constructor.
 *
 * The default value of the bits in a BitArray is undefined. Initialize the
 * BitArray prior to use, for example by calling mark() or clear().
 *
 * BitArrays support iteration, through explicit method calls or via C++11
 * range-based for loops.
 */

template <unsigned tSize>
class BitArray
{
    static unsigned clz(uint32_t word) {
        return __builtin_clz(word);
    }

    static uint32_t lz(unsigned bit) {
        return 0x80000000 >> bit;
    }

    // Return a range of bits from 'bit' to 31.
    static uint32_t range(int bit) {
        if (bit <= 0) return 0xffffffff;
        if (bit >= 32) return 0;
        return 0xffffffff >> bit;
    }

public:
    uint32_t words[(tSize + 31) / 32];
  
    /// Retrieve the size of this array in bits, always constant at compile-time.
    static unsigned size() {
        return tSize;
    }

    /// Mark (set to 1) a single bit
    void mark(unsigned index)
    {
        const unsigned NUM_WORDS = (tSize + 31) / 32;

        ASSERT(index < tSize);
        if (NUM_WORDS > 1) {
            unsigned word = index >> 5;
            unsigned bit = index & 31;
            words[word] |= lz(bit);
        } else {
            words[0] |= lz(index);
        }
    }

    /// Clear (set to 0) a single bit
    void clear(unsigned index)
    {
        const unsigned NUM_WORDS = (tSize + 31) / 32;

        ASSERT(index < tSize);
        if (NUM_WORDS > 1) {
            unsigned word = index >> 5;
            unsigned bit = index & 31;
            words[word] &= ~lz(bit);
        } else {
            words[0] &= ~lz(index);
        }
    }

    /// Mark (set to 1) all bits in the array
    void mark()
    {
        const unsigned NUM_WORDS = (tSize + 31) / 32;
        const unsigned NUM_FULL_WORDS = tSize / 32;
        const unsigned REMAINDER_BITS = tSize & 31;

        STATIC_ASSERT(NUM_FULL_WORDS + 1 == NUM_WORDS ||
                      NUM_FULL_WORDS == NUM_WORDS);

        // Set fully-utilized words only
        _SYS_memset32(words, -1, NUM_FULL_WORDS);

        if (NUM_FULL_WORDS != NUM_WORDS) {
            // Set only bits < tSize in the last word.
            uint32_t mask = ((uint32_t)-1) << ((32 - REMAINDER_BITS) & 31);
            words[NUM_FULL_WORDS] = mask;
        }
    }

    /// Clear (set to 0) all bits in the array
    void clear()
    {
        const unsigned NUM_WORDS = (tSize + 31) / 32;
        _SYS_memset32(words, 0, NUM_WORDS);
    }

    /// Is a particular bit marked?
    bool test(unsigned index) const
    {
        const unsigned NUM_WORDS = (tSize + 31) / 32;

        ASSERT(index < tSize);
        if (NUM_WORDS > 1) {
            unsigned word = index >> 5;
            unsigned bit = index & 31;
            return (words[word] & lz(bit)) != 0;
        } else {
            return (words[0] & lz(index)) != 0;
        }
    }

    /// Is every bit in this array set to zero?
    bool empty() const
    {
        const unsigned NUM_WORDS = (tSize + 31) / 32;

        if (NUM_WORDS > 1) {
            #pragma clang diagnostic push
            #pragma clang diagnostic ignored "-Wtautological-compare"
            for (unsigned w = 0; w < NUM_WORDS; w++)
                if (words[w])
                    return false;
            return true;
            #pragma clang diagnostic pop
        } else if (NUM_WORDS == 1) {
            return words[0] == 0;
        } else {
            return true;
        }
    }

    /// How many bits are marked in this vector?
    unsigned count() const
    {
        const unsigned NUM_WORDS = (tSize + 31) / 32;

        if (NUM_WORDS > 1) {
            #pragma clang diagnostic push
            #pragma clang diagnostic ignored "-Wtautological-compare"
            unsigned c = 0;
            for (unsigned w = 0; w < NUM_WORDS; w++)
                c += __builtin_popcount(words[w]);
            return c;
            #pragma clang diagnostic pop
        } else if (NUM_WORDS == 1) {
            return __builtin_popcount(words[0]);
        } else {
            return 0;
        }
    }

    /**
     * @brief Find the lowest index where there's a marked (1) bit.
     *
     * If any marked bits exist, returns true and puts the bit's index
     * in "index". Iff the entire array is zero, returns false.
     */
    bool findFirst(unsigned &index) const
    {
        const unsigned NUM_WORDS = (tSize + 31) / 32;

        if (NUM_WORDS > 1) {
            #pragma clang diagnostic push
            #pragma clang diagnostic ignored "-Wtautological-compare"
            for (unsigned w = 0; w < NUM_WORDS; w++) {
                uint32_t v = words[w];
                if (v) {
                    index = (w << 5) | clz(v);
                    ASSERT(index < tSize);
                    return true;
                }
            }
            #pragma clang diagnostic pop
        } else if (NUM_WORDS == 1) {
            uint32_t v = words[0];
            if (v) {
                index = clz(v);
                ASSERT(index < tSize);
                return true;
            }
        }
        return false;
    }

    /**
     * @brief Find and clear the lowest marked bit.
     *
     * If we find any marked bits,
     * returns true, sets "index" to that bit's index, and clears the
     * bit. This can be used as part of an iteration pattern:
     *
     *       unsigned index;
     *       while (vec.clearFirst(index)) {
     *           doStuff(index);
     *       }
     *
     * This is functionally equivalent to findFirst() followed by
     * clear(), but it's a tiny bit more efficient.
     */
    bool clearFirst(unsigned &index)
    {
        const unsigned NUM_WORDS = (tSize + 31) / 32;

        if (NUM_WORDS > 1) {
            #pragma clang diagnostic push
            #pragma clang diagnostic ignored "-Wtautological-compare"
            for (unsigned w = 0; w < NUM_WORDS; w++) {
                uint32_t v = words[w];
                if (v) {
                    unsigned bit = clz(v);
                    words[w] ^= lz(bit);
                    index = (w << 5) | bit;
                    ASSERT(index < tSize);
                    return true;
                }
            }
            #pragma clang diagnostic pop
        } else if (NUM_WORDS == 1) {
            uint32_t v = words[0];
            if (v) {
                unsigned bit = clz(v);
                words[0] ^= lz(bit);
                index = bit;
                ASSERT(index < tSize);
                return true;
            }
        }
        return false;
    }

    /**
     * @brief Find and clear the Nth lowest marked bit.
     *
     * For n=0, this is equivalent to clearFirst(). For n=1, we
     * skip the first marked bit and clear the second. And so on.
     * If there is no Nth marked bit, returns false.
     */
    bool clearN(unsigned &index, unsigned n)
    {
        for (unsigned i : *this) {
            if (n--)
                continue;

            clear(i);
            index = i;
            return true;
        }
        return false;
    }

    /// An STL-style iterator for the BitArray.
    struct iterator {
        unsigned index;
        BitArray<tSize> remaining;

        bool operator == (const iterator& other) const {
            return index == other.index;
        }

        bool operator != (const iterator& other) const {
            return index != other.index;
        }

        unsigned operator* () const {
            return index;
        }

        const iterator& operator++ () {
            if (!remaining.clearFirst(index))
                index = unsigned(-1);
            return *this;
        }
    };

    /// Return an STL-style iterator for this array.
    iterator begin() const {
        iterator i;
        i.remaining = *this;
        return ++i;
    }

    /// Return an STL-style iterator for this array
    iterator end() const {
        iterator i;
        i.index = unsigned(-1);
        return i;
    }

    /// Create a new empty BitArray
    BitArray() {
        clear();
    }

    /// Create a new BitArray with a single bit marked
    explicit BitArray(unsigned index) {
        const unsigned NUM_WORDS = (tSize + 31) / 32;

        ASSERT(index < tSize);
        if (NUM_WORDS > 1) {
            #pragma clang diagnostic push
            #pragma clang diagnostic ignored "-Wtautological-compare"
            for (unsigned w = 0; w < NUM_WORDS; w++) {
                words[w] = (w == (index >> 5)) ? lz(index & 31) : 0;
            }
            #pragma clang diagnostic pop
        } else {
            words[0] = lz(index);
        }
    }

    /**
     * @brief Create a new BitArray with a range of bits marked.
     *
     * This is a half-open interval. All bits >= 'begin' and < 'end' are marked.
     */
    explicit BitArray(unsigned begin, unsigned end) {
        const unsigned NUM_WORDS = (tSize + 31) / 32;

        ASSERT(begin < tSize);
        ASSERT(end <= tSize);
        ASSERT(begin <= end);

        if (NUM_WORDS > 1) {
            #pragma clang diagnostic push
            #pragma clang diagnostic ignored "-Wtautological-compare"
            for (unsigned w = 0; w < NUM_WORDS; w++) {
                int offset = w << 5;
                words[w] = range(begin - offset) & ~range(end - offset);
            }
            #pragma clang diagnostic pop
        } else {
            words[0] = range(begin) & ~range(end);
        }
    }

    /// Bitwise AND of two BitArrays of the same size
    BitArray<tSize> operator & (const BitArray<tSize> &other) const
    {
        const unsigned NUM_WORDS = (tSize + 31) / 32;
        BitArray<tSize> result;
        #pragma clang diagnostic push
        #pragma clang diagnostic ignored "-Wtautological-compare"
        for (unsigned w = 0; w < NUM_WORDS; w++)
            result.words[w] = words[w] & other.words[w];
        #pragma clang diagnostic pop
        return result;
    }

    /// Bitwise OR of two BitArrays of the same size
    BitArray<tSize> operator | (const BitArray<tSize> &other) const
    {
        const unsigned NUM_WORDS = (tSize + 31) / 32;
        BitArray<tSize> result;
        #pragma clang diagnostic push
        #pragma clang diagnostic ignored "-Wtautological-compare"
        for (unsigned w = 0; w < NUM_WORDS; w++)
            result.words[w] = words[w] | other.words[w];
        #pragma clang diagnostic pop
        return result;
    }

    /// Bitwise XOR of two BitArrays of the same size
    BitArray<tSize> operator ^ (const BitArray<tSize> &other) const
    {
        const unsigned NUM_WORDS = (tSize + 31) / 32;
        BitArray<tSize> result;
        #pragma clang diagnostic push
        #pragma clang diagnostic ignored "-Wtautological-compare"
        for (unsigned w = 0; w < NUM_WORDS; w++)
            result.words[w] = words[w] ^ other.words[w];
        #pragma clang diagnostic pop
        return result;
    }

    /// Negate a BitArray, returning a new array in which each bit is inverted
    BitArray<tSize> operator ~ () const
    {
        const unsigned NUM_WORDS = (tSize + 31) / 32;
        const unsigned NUM_FULL_WORDS = tSize / 32;
        const unsigned REMAINDER_BITS = tSize & 31;
        BitArray<tSize> result;

        STATIC_ASSERT(NUM_FULL_WORDS + 1 == NUM_WORDS ||
                      NUM_FULL_WORDS == NUM_WORDS);

        // Trivial inversion for fully-utilized words
        #pragma clang diagnostic push
        #pragma clang diagnostic ignored "-Wtautological-compare"
        for (unsigned w = 0; w < NUM_FULL_WORDS; w++)
            result.words[w] = ~words[w];
        #pragma clang diagnostic pop

        if (NUM_FULL_WORDS != NUM_WORDS) {
            // Set only bits < tSize in the last word.
            uint32_t mask = ((uint32_t)-1) << ((32 - REMAINDER_BITS) & 31);
            result.words[NUM_FULL_WORDS] = mask & ~words[NUM_FULL_WORDS];
        }

        return result;
    }
};


/**
 * @} end defgroup array
 */

};  // namespace Sifteo
