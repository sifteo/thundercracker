#include <sifteo/array.h>
using namespace Sifteo;

// Optimization barrier
template <typename T> T ob(T x) {
    volatile T y = x;
    return y;
}

class C { 
  public:
    int i;
};

C c0 = { ob(0) };
C c1 = { ob(1) };
C c2 = { ob(2) };
C c3 = { ob(3) };
C c4 = { ob(4) };
C other = { ob(99) };

void arrayOfObjects()
{
    Array<C, 4> a;

    ASSERT((unsigned)0 == a.count());
    ASSERT(a.empty());

    a.append(c4);
    ASSERT(4 == a[0].i);

    a.clear();
    ASSERT(a.empty());

    a.append(c2);
    ASSERT(1 == a.count());
}

void arrayOfObjectPointers()
{
    Array<C*,4> a;
    a.append(&c0);
    a.append(&c1);
    a.append(&c2);
    ASSERT(3 == a.count());
    ASSERT(&c2 == a[2]);
}

void arrayOfInt()
{
    Array<int,4> a;
    a.append(ob(1));
    a.append(ob(2));
    ASSERT(2 == a.count());
}

void iterator()
{
    Array<int, 4> a, b;
    a.append(ob(1));
    a.append(ob(2));
    for(Array<int,4>::iterator it=a.begin(); it<a.end(); it++){
        b.append(*it);
    }
    ASSERT(1 == b[0]);
    ASSERT(2 == b[1]);
    ASSERT(2 == b.count());
}

void erase()
{
    Array<char, 4> a;
    a.append(ob('a'));
    a.append(ob('b'));
    a.append(ob('c'));
    a.append(ob('d'));
    a.erase(ob(1)); // 'b'
    ASSERT(3 == a.count());
    ASSERT('c' == a[1]);

    a.erase(ob(2));
    ASSERT(2 == a.count());
}

void findInArrayOfPointers()
{
    Array<C*, 4> a;
    a.append(&c1);
    a.append(&c2);
    a.append(&c3);

    ASSERT(0 == a.find(&c1));
    ASSERT(2 == a.find(&c3));
    ASSERT(-1 == a.find(&other));
}

void main()
{
    arrayOfObjects();
    arrayOfObjectPointers();
    arrayOfInt();
    iterator();
    erase();
    findInArrayOfPointers();
}
