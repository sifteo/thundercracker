#include <sifteo.h>
using namespace Sifteo;

float _ceilf(float f)
{
    float i = (float)(int)f;
    return (fmod(f, 1.f)) ?  i + 1.f : i;
}
