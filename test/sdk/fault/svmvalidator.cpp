#include "fault.h"

CODE_BLOCK(testReturnOnly) {
    0xdf00,     // svc 0 (return)
    0xffff,     // invalid
    0xffff,     // invalid
    0xffff,     // invalid
}; 

void testSvmValidator()
{
	// Plain call, always works
    call(testReturnOnly);
    SCRIPT(LUA, assertFault(nil));

    // Reserved LSBs, currently have no effect. Function still returns.
    call(testReturnOnly, 1);
    SCRIPT(LUA, assertFault(nil));
    call(testReturnOnly, 2);
    SCRIPT(LUA, assertFault(nil));
    call(testReturnOnly, 3);
    SCRIPT(LUA, assertFault(nil));

    // Jumping to invalid bundle
    call(testReturnOnly, 4);
    SCRIPT(LUA, assertFault(F_BAD_CODE_ADDRESS));
}
