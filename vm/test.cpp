#include <sifteo/abi.h>

void __attribute__ ((noinline)) foo(int *x)
{
    x[0] = 1;
    x[0] = 2;
    x[1] = 3;    
}

void siftmain()
{
    int x[5];
    foo(x);
}
