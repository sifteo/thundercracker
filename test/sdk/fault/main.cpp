#include <sifteo.h>
using namespace Sifteo;

void NOINLINE returnFunc() {}

void NOINLINE testWriteNull()
{
    *(volatile int*)0 = 0;
}

void NOINLINE testReadNull()
{
    volatile int x = *(volatile int*)0;
}

void main()
{
    SCRIPT_FMT(LUA, "pReturnFunc = 0x%x", &returnFunc);
    SCRIPT(LUA,
        package.path = package.path .. ";../../lib/?.lua"
        require('test-fault')
        assertFault(nil)
    );

    testWriteNull();
    SCRIPT(LUA, assertFault(0x06));

    testReadNull();
    SCRIPT(LUA, assertFault(0x05));

    SCRIPT(LUA, assertFault(nil));
    LOG("Success.\n");
}
