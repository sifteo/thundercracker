#include <sifteo/abi.h>

void siftmain()
{
    static volatile int x[5];
    x[0] = 1;
    x[0] = 2;
    x[1] = 3;
}
