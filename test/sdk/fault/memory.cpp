#include "fault.h"

void NOINLINE testWriteNull()
{
    *(volatile int*)0 = 0;
}

void NOINLINE testReadNull()
{
    volatile int x = *(volatile int*)0;
}

void testMemoryFaults()
{
    testWriteNull();
    SCRIPT(LUA, assertFault(F_STORE_ADDRESS));

    testReadNull();
    SCRIPT(LUA, assertFault(F_LOAD_ADDRESS));
}
