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

void eraseChar()
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

void eraseInt()
{
    Array<uint16_t, 4, uint8_t> a;
    a.append(ob(10));
    a.append(ob(11));
    a.append(ob(12));
    a.append(ob(13));
    a.erase(ob(1)); // 11
    ASSERT(3 == a.count());
    ASSERT(10 == a[0]);
    ASSERT(12 == a[1]);
    ASSERT(13 == a[2]);

    a.erase(ob(2)); // 13
    ASSERT(2 == a.count());
    ASSERT(10 == a[0]);
    ASSERT(12 == a[1]);
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

void bitArraySizeEdgeCases()
{
    BitArray<0> a0;
    BitArray<1> a1;
    BitArray<31> a31;
    BitArray<32> a32;
    BitArray<33> a33;
    BitArray<63> a63;
    BitArray<64> a64;
    BitArray<65> a65;

    ASSERT(sizeof a0 == 0);
    ASSERT(sizeof a1 == 4);
    ASSERT(sizeof a31 == 4);
    ASSERT(sizeof a32 == 4);
    ASSERT(sizeof a33 == 8);
    ASSERT(sizeof a63 == 8);
    ASSERT(sizeof a64 == 8);
    ASSERT(sizeof a65 == 12);

    a0.mark();
    a1.mark();
    a31.mark();
    a32.mark();
    a33.mark();
    a63.mark();
    a64.mark();
    a65.mark();

    ASSERT(a0.empty() == true);
    ASSERT(a1.empty() == false);
    ASSERT(a31.empty() == false);
    ASSERT(a32.empty() == false);
    ASSERT(a33.empty() == false);
    ASSERT(a63.empty() == false);
    ASSERT(a64.empty() == false);
    ASSERT(a65.empty() == false);

    unsigned index = 100;
    ASSERT(a0.findFirst(index) == false && index == 100);
    ASSERT(a0.clearFirst(index) == false && index == 100);
    ASSERT(a0.findFirst(index) == false && index == 100);

    ASSERT(a1.findFirst(index) == true && index == 0);
    ASSERT(a31.findFirst(index) == true && index == 0);
    ASSERT(a32.findFirst(index) == true && index == 0);
    ASSERT(a33.findFirst(index) == true && index == 0);
    ASSERT(a63.findFirst(index) == true && index == 0);
    ASSERT(a64.findFirst(index) == true && index == 0);
    ASSERT(a65.findFirst(index) == true && index == 0);

    ASSERT(a1.clearFirst(index) == true && index == 0);
    ASSERT(a1.clearFirst(index) == false && index == 0);
    ASSERT(a1.findFirst(index) == false && index == 0);

    for (unsigned x = 0; x < 31; x++)
        ASSERT(a31.clearFirst(index) == true && index == x);
    ASSERT(a31.clearFirst(index) == false && index == 30);
    ASSERT(a31.findFirst(index) == false && index == 30);

    for (unsigned x = 0; x < 32; x++)
        ASSERT(a32.clearFirst(index) == true && index == x);
    ASSERT(a32.clearFirst(index) == false && index == 31);
    ASSERT(a32.findFirst(index) == false && index == 31);

    for (unsigned x = 0; x < 33; x++)
        ASSERT(a33.clearFirst(index) == true && index == x);
    ASSERT(a33.clearFirst(index) == false && index == 32);
    ASSERT(a33.findFirst(index) == false && index == 32);

    for (unsigned x = 0; x < 63; x++)
        ASSERT(a63.clearFirst(index) == true && index == x);
    ASSERT(a63.clearFirst(index) == false && index == 62);
    ASSERT(a63.findFirst(index) == false && index == 62);

    for (unsigned x = 0; x < 64; x++)
        ASSERT(a64.clearFirst(index) == true && index == x);
    ASSERT(a64.clearFirst(index) == false && index == 63);
    ASSERT(a64.findFirst(index) == false && index == 63);

    for (unsigned x = 0; x < 65; x++)
        ASSERT(a65.clearFirst(index) == true && index == x);
    ASSERT(a65.clearFirst(index) == false && index == 64);
    ASSERT(a65.findFirst(index) == false && index == 64);
}

void bitArrayIter()
{
    unsigned index = 100;
    BitArray<4> a4;
    BitArray<48> a48;

    a4.clear();
    a4.mark(2);
    ASSERT(a4.clearFirst(index) == true && index == 2);
    ASSERT(a4.clearFirst(index) == false && index == 2);

    a4.mark();
    a4.clear(2);
    ASSERT(a4.clearFirst(index) == true && index == 0);
    ASSERT(a4.clearFirst(index) == true && index == 1);
    ASSERT(a4.clearFirst(index) == true && index == 3);
    ASSERT(a4.clearFirst(index) == false && index == 3);

    a48.clear();
    a48.mark(20);
    a48.mark(1);
    a48.mark(0);
    a48.mark(30);
    a48.mark(3);
    a48.mark(5);
    a48.mark(40);
    ASSERT(a48.clearFirst(index) == true && index == 0);
    ASSERT(a48.clearFirst(index) == true && index == 1);
    ASSERT(a48.clearFirst(index) == true && index == 3);
    ASSERT(a48.clearFirst(index) == true && index == 5);
    ASSERT(a48.clearFirst(index) == true && index == 20);
    ASSERT(a48.clearFirst(index) == true && index == 30);
    ASSERT(a48.clearFirst(index) == true && index == 40);
    ASSERT(a48.clearFirst(index) == false && index == 40);
}

void main()
{
    arrayOfObjects();
    arrayOfObjectPointers();
    arrayOfInt();
    iterator();
    eraseChar();
    eraseInt();
    findInArrayOfPointers();
    bitArraySizeEdgeCases();
    bitArrayIter();
    
    LOG("Success.\n");
}
