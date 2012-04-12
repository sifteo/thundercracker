#include "ShapeMask.h"

namespace TotalsGame
{
const ShapeMask ShapeMask::Zero = ShapeMask(vec(0,0), 0L);
const ShapeMask ShapeMask::Unity = ShapeMask(vec(1,1), 1L);



bool Connection::Matches(const Connection &c) {
    //return (dir - c.dir).Norm() == 0;
    return dir.x == c.dir.x && dir.y == c.dir.y;
}

bool Connection::IsFromOrigin() {
    return pos.x == 0 && pos.y == 0;
}

bool Connection::IsBottom() {
    return dir.x == 0 && dir.y == 1;
}

bool Connection::IsRight() {
    return dir.x == 1 && dir.y == 0;
}

ShapeMask::ShapeMask(Int2 size, bool *flags, size_t numFlags) {
    this->size = size;
    this->bits = 0L;
    for(int x=0; x<size.x; ++x) {
        for(int y=0; y<size.y; ++y) {
            size_t index = (y * size.x + x);
            ASSERT(index < numFlags);
            if (flags[index]) {
                bits |= (1L<<index);
            }
        }
    }
}

ShapeMask::ShapeMask(Int2 size, long bits) {
    this->size = size;
    this->bits = bits;
}

ShapeMask::ShapeMask()
{
    size = vec(0,0);
    bits = 0;
}

ShapeMask ShapeMask::GetRotation() {
    Int2 s = vec(size.y, size.x);
    long bts = 0L;
    for(int x=0; x<s.x; ++x) {
        for(int y=0; y<s.y; ++y) {
            if (BitAt(vec(y, size.y-1-x))) {
                int shift = y * s.x + x;
                bts |= (1L<<shift);
            }
        }
    }
    return ShapeMask(s, bts);
}

ShapeMask ShapeMask::GetReflection() {
    Int2 s = vec(size.x, size.y);
    long bts = 0L;
    for(int x=0; x<s.x; ++x) {
        for(int y=0; y<s.y; ++y) {
            if (BitAt(vec(x, size.y-1-y))) {
                int shift = y * s.x + x;
                bts |= (1L<<shift);
            }
        }
    }
    return ShapeMask(s, bts);
}

bool ShapeMask::BitAt(Int2 p) const
{
    if (p.x < 0 || p.x >= size.x || p.y < 0 || p.y >= size.y) { return false; }
    int shift = p.y * size.x + p.x;
    return (bits & (1L<<shift))!= 0L;
}

ShapeMask ShapeMask::SubMask(Int2 p, Int2 s) {
    long bts = 0L;
    for(int x=0; x<s.x; ++x) {
        for(int y=0; y<s.y; ++y) {
            if (BitAt(p+vec(x,y))) {
                int shift = y * s.x + x;
                bts |= (1L<<shift);
            }
        }
    }
    return ShapeMask(s, bts);
}

bool ShapeMask::Matches(const ShapeMask &mask) {
    return size.x == mask.size.x && size.y == mask.size.y && bits == mask.bits;
}

void ShapeMask::ListOutConnections(Connection *connections, int *numConnections, int maxConnections)
{
    *numConnections = 0;

    Int2 p;
    for(p.x=0; p.x<size.x; ++p.x)
    {
        for(p.y=0; p.y<size.y; ++p.y)
        {
            if (BitAt(p))
            {
                if (!BitAt(p+vec(1,0)))
                {
                    ASSERT(*numConnections < maxConnections);
                    connections[*numConnections].pos = p;
                    connections[*numConnections].dir.set(1,0);
                    (*numConnections)++;
                }
                if (!BitAt(p+vec(0,1)))
                {
                    ASSERT(*numConnections < maxConnections);
                    connections[*numConnections].pos = p;
                    connections[*numConnections].dir.set(0,1);
                    (*numConnections)++;
                }
            }
        }
    }
}

void ShapeMask::ListInConnections(Connection *connections, int *numConnections, int maxConnections)
{
    *numConnections = 0;

    Int2 p;
    for(p.x=0; p.x<size.x; ++p.x)
    {
        for(p.y=0; p.y<size.y; ++p.y)
        {
            if (BitAt(p))
            {
                if (!BitAt(p-vec(1,0)))
                {
                    ASSERT(*numConnections < maxConnections);
                    connections[*numConnections].pos = p;
                    connections[*numConnections].dir.set(1,0);
                    (*numConnections)++;
                }
                if (!BitAt(p-vec(0,1)))
                {
                    ASSERT(*numConnections < maxConnections);
                    connections[*numConnections].pos = p;
                    connections[*numConnections].dir.set(0,1);
                    (*numConnections)++;
                }
            }
        }
    }
}

#if NO_STACK_PARAMS_HACK
ShapeMask ShapeMask::m1;
ShapeMask ShapeMask::m2;
bool ShapeMask::TryConcat(
        Int2 offset,
        ShapeMask *result,Int2 *d1, Int2 *d2
        )
#else
bool ShapeMask::TryConcat(
        const ShapeMask &m1, const ShapeMask &m2, Int2 offset,
        ShapeMask *result,Int2 *d1, Int2 *d2
        )
#endif
{
    Int2 min = vec(
                MIN(offset.x, 0),
                MIN(offset.y, 0)
                );
    Int2 max = vec(
                MAX(m1.size.x, offset.x + m2.size.x),
                MAX(m1.size.y, offset.y + m2.size.y)
                );
    long newbits = 0L;
    Int2 newsize = max - min;
    Int2 p;
    for(p.x = min.x; p.x < max.x; ++p.x)
    {
        for(p.y = min.y; p.y < max.y; ++p.y)
        {
            bool b1 = m1.BitAt(p);
            bool b2 = m2.BitAt(p - offset);
            if (b1 && b2)
            {
                *result = Zero;
                d1->set(0,0);
                d2->set(0,0);
                return false;
            }
            if (b1 || b2)
            {
                Int2 d = p - min;
                int shift = d.y * newsize.x + d.x;
                newbits |= (1L<<shift);
            }
        }
    }
    *result = ShapeMask(newsize, newbits);
    //d1 = min - Int2.Zero;
    *d1 = vec(0,0) - min;

    *d2 = offset - min;
    return true;
}


}
