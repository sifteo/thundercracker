#include "fault.h"

void NOINLINE returnFunc() {}

void main()
{
    SCRIPT_FMT(LUA, "pReturnFunc = 0x%x", &returnFunc);
    SCRIPT(LUA,
        package.path = package.path .. ";../../lib/?.lua"
        require('test-fault')
        assertFault(nil)
    );

    testMemoryFaults();
    testSvmValidator();

    SCRIPT(LUA, assertFault(nil));
    LOG("Success.\n");
}
