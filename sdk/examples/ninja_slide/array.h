#ifndef _SIFTEO_ARRAY_H
#define _SIFTEO_ARRAY_H

#include <sifteo/macros.h>


/**
 * A statically sized array.
 *
 * Method naming and conventions are STL-inspired, but designed for very
 * minimal runtime memory footprint.
 */
template <class T, unsigned _capacity>
class Array {
public:


    Array() {
        clear();
    }

    static unsigned capacity() {
        return _capacity;
    }

    unsigned count() const {
        return numItems;
    }

    void clear() {
        numItems = 0;
    }
    
    bool empty() const {
        return numItems == 0;
    }

    T operator[](unsigned index) const {
        return items[index];
    }

    T operator[](int index) const {
        return items[index];
    }

    Array& append(const T &newItem){
        assert(count() < _capacity);
        items[numItems++] = newItem;
        return *this;
    }


//     Array& operator<<(const T &newItem) {
//         ASSERT(newItem < _capacity);
//         items[newItem++] = newItem;
//         return *this;
//     }

private:
    T items[_capacity];
    unsigned numItems;

};
#endif // _SIFTEO_ARRAY_H
