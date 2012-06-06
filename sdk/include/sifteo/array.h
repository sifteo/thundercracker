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
 * @{
 */

/**
 * @brief A statically sized array
 *
 * Method naming and conventions are STL-inspired, but designed for very
 * minimal runtime memory footprint.
 */
template <typename T, unsigned _capacity, typename sizeT = uint32_t>
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
        return _capacity;
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
        ASSERT(c <= _capacity);
        numItems = c;
    }

    /// Is this array empty?
    bool empty() const {
        return count() == 0;
    }

    /// Accessor for array elements
    T operator[](unsigned index) const {
        ASSERT(index < count());
        return items[index];
    }

    /// Accessor for array elements
    T operator[](int index) const {
        ASSERT(0 <= index && index < (int)count());
        return items[index];
    }

    /// Synonym for push_back()
    void append(const T &newItem) {
        push_back(newItem);
    }

    /// Copy 'newItem' to the first unused slot in the array.
    void push_back(const T &newItem) {
        ASSERT(count() < _capacity);
        items[numItems++] = newItem;
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
    T items[_capacity];
    sizeT numItems;
};

/**
 * @} end defgroup array
 */

};  // namespace Sifteo
